#define ZLIB_CONST
#include <algorithm>
#include <cassert>
#include <gsl.h>
#include <limits>
#include <string>
#include <string_view.h>
#include <vector>
#include <zlib.h>
#include "bigendianview.h"
#include "elf.h"
#include "filesystem/filesystem.h"
#include "cpu/instructiondata.h"
#include "kernelmodule.h"
#include "loader.h"
#include "log.h"
#include "mem/mem.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "system.h"
#include "types.h"
#include "usermodule.h"
#include "util.h"
#include "teenyheap.h"

Loader gLoader;
using TrampolineMap = std::map<ppcaddr_t, ppcaddr_t>;

class SequentialMemoryTracker
{
public:
   SequentialMemoryTracker(void *ptr, size_t size)
      : mStart(static_cast<uint8_t*>(ptr)), mEnd(mStart + size), mPtr(mStart)
   {
   }

   ppcaddr_t
      getCurrentAddr() const
   {
      return mem::untranslate(mPtr);
   }

   void *
      get(size_t size, uint32_t alignment = 4)
   {
      // Ensure section alignment
      auto alignOffset = alignUp(mPtr, alignment) - mPtr;
      size += alignOffset;

      // Double-check alignment
      auto ptrOut = mPtr + alignOffset;
      assert(alignUp(ptrOut, alignment) == ptrOut);

      // Make sure we have room
      assert(mPtr + size <= mEnd);

      mPtr += size;
      return ptrOut;
   }

protected:
   uint8_t *mStart;
   uint8_t *mEnd;
   uint8_t *mPtr;
};

static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod, SequentialMemoryTracker &codeSeg, TrampolineMap &trampolines, void *target, const std::string& symbolName);

static bool
readFileInfo(BigEndianView &in, const SectionList &sections, elf::FileInfo &info);

static const elf::XSection *
findSection(const SectionList &sections, const char *shStrTab, const std::string &name);

static uint32_t
calculateSdaBase(const elf::XSection *sdata, const elf::XSection *sbss);

static elf::Symbol
getSymbol(BigEndianView &symSecView, uint32_t index);

static uint32_t
getSymbolAddress(const elf::Symbol &sym, const SectionList &sections);


// Find and read the SHT_RPL_FILEINFO section
static bool
readFileInfo(BigEndianView &in, const SectionList &sections, elf::FileInfo &info)
{
   std::vector<uint8_t> data;

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_FILEINFO) {
         continue;
      }

      elf::readSectionData(in, section.header, data);
      elf::readFileInfo(BigEndianView { data }, info);
      return true;
   }

   gLog->error("Failed to find RPLFileInfo section");
   return false;
}


// Find a section with a matching name
static const elf::XSection *
findSection(const SectionList &sections, const char *shStrTab, const std::string &name)
{
   for (auto &section : sections) {
      auto sectionName = shStrTab + section.header.name;

      if (sectionName == name) {
         return &section;
      }
   }

   return nullptr;
}


// Calculate the SDA base address based off sdata and sbss sections
static uint32_t
calculateSdaBase(const elf::XSection *sdata, const elf::XSection *sbss)
{
   uint32_t start, end;
   auto sdataAddr = sdata ? sdata->virtAddress : 0u;
   auto sbssAddr = sbss ? sbss->virtAddress : 0u;

   if (sdata && sbss) {
      start = std::min(sdataAddr, sbssAddr);
      end = std::max(sdataAddr + sdata->virtSize, sbssAddr + sbss->virtSize);
   } else if (sdata) {
      start = sdataAddr;
      end = sdataAddr + sdata->virtSize;
   } else if (sbss) {
      start = sbssAddr;
      end = sbssAddr + sbss->virtSize;
   } else {
      return 0;
   }

   if ((end - start) > 0xffff) {
      gLog->error("Small data and bss do not fit in a 16 bit range");
   }

   return ((end - start) / 2) + start;
}


// Get symbol at index
static elf::Symbol
getSymbol(BigEndianView &symSecView, uint32_t index)
{
   elf::Symbol sym;
   symSecView.seek(index * sizeof(elf::Symbol));
   elf::readSymbol(symSecView, sym);
   return sym;
}


// Get address of symbol
static uint32_t
getSymbolAddress(const elf::Symbol &sym, const SectionList &sections)
{
   auto &symSec = sections[sym.shndx];
   return (sym.value - symSec.header.addr) + symSec.virtAddress;
}


// Returns address of trampoline for target
static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod, SequentialMemoryTracker &codeSeg, TrampolineMap &trampolines, void *target, const std::string& symbolName)
{
   auto trampAddr = codeSeg.getCurrentAddr();
   auto targetAddr = mem::untranslate(target);
   auto trampIter = trampolines.find(targetAddr);

   if (trampIter != trampolines.end()) {
      return trampIter->second;
   }

   auto tramp = mem::translate<uint32_t>(trampAddr);
   auto delta = static_cast<ptrdiff_t>(targetAddr) - static_cast<ptrdiff_t>(trampAddr);

   if (delta > -0x1FFFFFCll && delta < 0x1FFFFFCll) {
      tramp = static_cast<uint32_t*>(codeSeg.get(4));

      // Short jump using b
      auto b = gInstructionTable.encode(InstructionID::b);
      b.li = delta >> 2;
      b.lk = 0;
      b.aa = 0;
      *tramp = byte_swap(b.value);
   } else if (targetAddr < 0x03fffffc) {
      tramp = static_cast<uint32_t*>(codeSeg.get(4));

      // Absolute jump using b
      auto b = gInstructionTable.encode(InstructionID::b);
      b.li = targetAddr >> 2;
      b.lk = 0;
      b.aa = 1;
      *tramp = byte_swap(b.value);
   } else {
      // Need to implement 16-byte long-jumping here...
      assert(0);
   }

   loadedMod->symbols.emplace(symbolName + "#thunk", trampAddr);
   trampolines.emplace(targetAddr, trampAddr);
   return trampAddr;
}


// Allocates the loader heap from the MEM2 region
void
Loader::initialise(ppcsize_t maxCodeSize)
{
   // Get the MEM2 Region
   be_val<uint32_t> mem2start, mem2size;
   OSGetMemBound(OSMemoryType::MEM2, &mem2start, &mem2size);

   // Allocate MEM2 Region
   mem::alloc(mem2start, mem2size);
   mem::protect(0xFFF00000, 0x000FFFFF);

   // Steal some space for code heap
   mCodeHeap = std::make_unique<TeenyHeap>(mem::translate(mem2start), maxCodeSize);

   // Update MEM2 to ignore the code heap region
   OSSetMemBound(OSMemoryType::MEM2, mem2start + maxCodeSize, mem2size - maxCodeSize);
}


// Load RPL by name, could load a kernel module or a user module
LoadedModule *
Loader::loadRPL(std::string name)
{
   std::unique_ptr<LoadedModule> module;

   // Ensure moduleName has an extension
   if (!ends_with(name, ".rpl") && !ends_with(name, ".rpx")) {
      name = name + ".rpl";
   }

   // Check if we already have this module loaded
   auto itr = mModules.find(name);

   if (itr != mModules.end()) {
      return itr->second.get();
   }

   // Try to find module in system kernel library list
   if (!module) {
      auto kernelModule = gSystem.findModule(name.c_str());

      if (kernelModule) {
         module = loadKernelModule(name, kernelModule);
      }
   }

   // Try to find module in the game code directory
   if (!module) {
      auto fs = gSystem.getFileSystem();
      auto fh = fs->openFile("/vol/code/" + name, fs::File::Read);

      if (fh) {
         auto buffer = std::vector<uint8_t>(fh->size());
         fh->read(reinterpret_cast<char *>(buffer.data()), buffer.size());
         fh->close();

         module = loadRPL(name, buffer);
      }
   }

   if (!module) {
      gLog->error("Failed to load module {}", name);
      return nullptr;
   } else {
      auto result = module.get();
      gLog->info("Loaded module {}", name);
      mModules.emplace(name, std::move(module));
      return result;
   }
}


std::unique_ptr<LoadedModule>
Loader::loadKernelModule(const std::string &name, KernelModule *module)
{
   std::vector<KernelFunction*> funcExports;
   std::vector<KernelData*> dataExports;
   uint32_t dataSize = 0, codeSize = 0;
   auto &exports = module->getExportMap();

   for (auto &pair : exports) {
      auto exportInfo = pair.second;

      if (exportInfo->type == KernelExport::Function) {
         auto funcExport = static_cast<KernelFunction*>(exportInfo);
         codeSize += 8;
         funcExports.emplace_back(funcExport);
      } else if (exportInfo->type == KernelExport::Data) {
         auto dataExport = static_cast<KernelData*>(exportInfo);
         dataSize += dataExport->size;
         dataExports.emplace_back(dataExport);
      } else {
         gLog->error("Unexpected KernelExport type");
         return nullptr;
      }
   }

   // Create module
   auto loadedMod = std::make_unique<LoadedModule>();
   loadedMod->name = name;
   loadedMod->handle = OSAllocFromSystem<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod.get();

   // Load code section
   if (codeSize > 0) {
      auto codeRegion = static_cast<uint8_t*>(OSAllocFromSystem(codeSize, 4));
      auto start = mem::untranslate(codeRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection { ".text", start, end });

      for (auto &func : funcExports) {
         // Allocate some space for the thunk
         auto thunk = reinterpret_cast<uint32_t*>(codeRegion);
         auto addr = mem::untranslate(thunk);
         codeRegion += 8;

         // Write syscall thunk
         auto kc = gInstructionTable.encode(InstructionID::kc);
         kc.kcn = func->syscallID;
         *(thunk + 0) = byte_swap(kc.value);

         auto bclr = gInstructionTable.encode(InstructionID::bclr);
         bclr.bo = 0x1f;
         *(thunk + 1) = byte_swap(bclr.value);

         // Add to exports list
         loadedMod->exports.emplace(func->name, addr);
         loadedMod->symbols.emplace(func->name, addr);

         // Save the PPC ptr for internal lookups
         func->ppcPtr = thunk;
      }
   }

   // Load data section
   if (dataSize > 0) {
      auto dataRegion = static_cast<uint8_t*>(OSAllocFromSystem(dataSize, 4));
      auto start = mem::untranslate(dataRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection { ".data", start, end });

      for (auto &data : dataExports) {
         // Allocate the same for this export
         auto thunk = dataRegion;
         auto addr = mem::untranslate(thunk);
         dataRegion += data->size;

         // Add to exports list
         loadedMod->exports.emplace(data->name, addr);
         loadedMod->symbols.emplace(data->name, addr);

         // Save the PPC ptr for internal lookups
         data->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         *data->hostPtr = thunk;
      }
   }

   module->initialise();
   return loadedMod;
}


ppcaddr_t
Loader::registerUnimplementedData(const std::string& name)
{
   auto itr = mUnimplementedData.find(name);

   if (itr != mUnimplementedData.end()) {
      return itr->second | 0x800;
   }

   auto id = gsl::narrow_cast<uint32_t>(mUnimplementedData.size());
   auto fakeAddr = 0xFFF00000 | (id << 12);
   assert(id <= 0xFF);

   mUnimplementedData.emplace(name, fakeAddr);
   return fakeAddr | 0x800;
}


const char *
Loader::getUnimplementedData(ppcaddr_t addr)
{
   static char tempStr[2048];

   for (auto &data : mUnimplementedData) {
      if (data.second == (addr & 0xFFFFF000)) {
         int offset = static_cast<int>(addr & 0xFFF) - 0x800;

         if (offset > 0) {
            sprintf_s(tempStr, "%s + %d", data.first.c_str(), offset);
         } else if (offset < 0) {
            sprintf_s(tempStr, "%s - %d", data.first.c_str(), -offset);
         } else {
            sprintf_s(tempStr, "%s", data.first.c_str());
         }

         return tempStr;
      }
   }

   return "UNKNOWN";
}


ppcaddr_t
Loader::registerUnimplementedFunction(const std::string& name)
{
   auto itr = mUnimplementedFunctions.find(name);

   if (itr != mUnimplementedFunctions.end()) {
      return itr->second;
   }

   auto id = gSystem.registerUnimplementedFunction(name.c_str());
   auto thunk = static_cast<uint32_t*>(OSAllocFromSystem(8, 4));
   auto addr = mem::untranslate(thunk);

   // Write syscall thunk
   auto kc = gInstructionTable.encode(InstructionID::kc);
   kc.kcn = id;
   *(thunk + 0) = byte_swap(kc.value);

   auto bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   *(thunk + 1) = byte_swap(bclr.value);

   mUnimplementedFunctions.emplace(name, addr);
   return addr;
}

bool
Loader::processRelocations(LoadedModule *loadedMod, const SectionList &sections, BigEndianView &in, const char *shStrTab, SequentialMemoryTracker &codeSeg, AddressRange &trampSeg)
{
   auto trampolines = TrampolineMap {};
   auto buffer = std::vector<uint8_t> {};
   trampSeg.first = codeSeg.getCurrentAddr();

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      elf::readSectionData(in, section.header, buffer);
      auto in = BigEndianView { buffer };

      auto &symSec = sections[section.header.link];
      auto &targetSec = sections[section.header.info];
      auto symSecView = BigEndianView { symSec.memory, symSec.virtSize };
      auto &symStrTab = sections[symSec.header.link];

      auto targetBaseAddr = targetSec.header.addr;
      auto targetVirtAddr = targetSec.virtAddress;

      while (!in.eof()) {
         elf::Rela rela;
         elf::readRelocationAddend(in, rela);

         auto index = rela.info >> 8;
         auto type = rela.info & 0xff;
         auto reloAddr = rela.offset - targetBaseAddr + targetVirtAddr;

         auto symbol = getSymbol(symSecView, index);
         auto &symbolSection = sections[symbol.shndx];
         auto symbolSectionName = shStrTab + symbolSection.header.name;
         auto symbolName = reinterpret_cast<const char*>(symStrTab.memory) + symbol.name;

         auto symAddr = getSymbolAddress(symbol, sections);
         symAddr += rela.addend;

         if (symbolSection.header.type == elf::SHT_RPL_IMPORTS) {
            symAddr = mem::read<uint32_t>(symAddr);

            if (symAddr == 0) {
               symAddr = registerUnimplementedData(symbolName);
            }
         }

         auto ptr8 = mem::translate(reloAddr);
         auto ptr16 = reinterpret_cast<uint16_t*>(ptr8);
         auto ptr32 = reinterpret_cast<uint32_t*>(ptr8);

         switch (type) {
         case elf::R_PPC_ADDR32:
            *ptr32 = byte_swap(symAddr);
            break;
         case elf::R_PPC_ADDR16_LO:
            *ptr16 = byte_swap<uint16_t>(symAddr & 0xffff);
            break;
         case elf::R_PPC_ADDR16_HI:
            *ptr16 = byte_swap<uint16_t>(symAddr >> 16);
            break;
         case elf::R_PPC_ADDR16_HA:
            *ptr16 = byte_swap<uint16_t>((symAddr + 0x8000) >> 16);
            break;
         case elf::R_PPC_REL24:
         {
            auto ins = Instruction { byte_swap(*ptr32) };
            auto data = gInstructionTable.decode(ins);

            // Our REL24 trampolines only work for a branch instruction...
            assert(data->id == InstructionID::b);

            auto delta = static_cast<ptrdiff_t>(symAddr) - static_cast<ptrdiff_t>(reloAddr);

            if (delta < -0x01FFFFFC || delta > 0x01FFFFFC) {
               auto trampAddr = getTrampAddress(loadedMod, codeSeg, trampolines, mem::translate(symAddr), symbolName);

               // Ensure valid trampoline delta
               assert(trampAddr);
               delta = static_cast<ptrdiff_t>(trampAddr) - static_cast<ptrdiff_t>(reloAddr);
               symAddr = trampAddr;
            }

            assert(delta >= -0x01FFFFFC && delta <= 0x01FFFFFC);
            *ptr32 = byte_swap((byte_swap(*ptr32) & ~0x03FFFFFC) | (gsl::narrow_cast<uint32_t>(delta & 0x03FFFFFC)));
            break;
         }
         case elf::R_PPC_EMB_SDA21:
         {
            auto ins = Instruction { byte_swap(*ptr32) };
            ptrdiff_t offset = 0;

            if (ins.rA == 0) {
               offset = 0;
            } else if (ins.rA == 2) {
               // sda2Base
               offset = static_cast<ptrdiff_t>(symAddr) - static_cast<ptrdiff_t>(loadedMod->sda2Base);
            } else if (ins.rA == 13) {
               // sdaBase
               offset = static_cast<ptrdiff_t>(symAddr) - static_cast<ptrdiff_t>(loadedMod->sdaBase);
            } else {
               assert(0);
            }

            if (offset < std::numeric_limits<int16_t>::min() || offset > std::numeric_limits<int16_t>::max()) {
               gLog->error("Expected SDA relocation {:x} to be within signed 16 bit offset of base {}", symAddr, ins.rA);
               break;
            }

            ins.simm = offset;
            *ptr32 = byte_swap(ins.value);
            break;
         }
         default:
            gLog->error("Unknown relocation type {}", type);
         }
      }
   }

   trampSeg.second = codeSeg.getCurrentAddr();
   return true;
}

bool
Loader::processImports(LoadedModule *loadedMod, const SectionList &sections)
{
   std::map<std::string, ppcaddr_t> symbolTable;

   // Process import sections
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_IMPORTS) {
         continue;
      }

      // Load library
      auto libraryName = reinterpret_cast<const char*>(section.memory + 8);
      auto linkedModule = loadRPL(libraryName);

      // Zero the whole section after we have used the name
      memset(section.memory + 8, section.virtSize - 8, 0);

      if (!linkedModule) {
         // Ignore missing modules for now
         gLog->debug("Missing library {}", libraryName);
         continue;
      }

      // Add all these symbols to the symbol table
      for (auto &linkedExport : linkedModule->exports) {
         symbolTable.insert(linkedExport);
      }
   }

   // Process import symbols
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_SYMTAB) {
         continue;
      }

      auto strTab = reinterpret_cast<const char*>(sections[section.header.link].memory);
      auto symIn = BigEndianView { section.memory, section.virtSize };

      while (!symIn.eof()) {
         elf::Symbol sym;
         elf::readSymbol(symIn, sym);

         // Create symbol data
         auto name = strTab + sym.name;
         auto binding = sym.info >> 4;
         auto type = sym.info & 0xf;

         if (binding != elf::STB_GLOBAL) {
            // No need to scan LOCAL symbols for imports
            //  In fact, this crashes due to NULL symbols
            continue;
         }

         // Calculate relocated address
         auto &impsec = sections[sym.shndx];
         auto offset = sym.value - impsec.header.addr;
         auto baseAddress = impsec.virtAddress;
         auto virtAddress = baseAddress + offset;

         if (impsec.header.type == elf::SHT_NULL) {
            continue;
         }

         if (impsec.header.type == elf::SHT_RPL_IMPORTS) {
            auto symbolTargetIter = symbolTable.find(name);
            ppcaddr_t symbolAddr = 0u;

            if (symbolTargetIter == symbolTable.end()) {
               if (type == elf::STT_FUNC) {
                  symbolAddr = registerUnimplementedFunction(name);
               }
            } else {
               if (type != elf::STT_FUNC && type != elf::STT_OBJECT) {
                  assert(0);
               }

               symbolAddr = symbolTargetIter->second;
            }

            // Write the symbol address into .fimport or .dimport
            mem::write(virtAddress, symbolAddr);
         }
      }
   }

   return true;
}

bool
Loader::processExports(LoadedModule *loadedMod, const SectionList &sections)
{
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_EXPORTS) {
         continue;
      }

      auto secData = reinterpret_cast<uint32_t*>(section.memory);
      auto secNames = reinterpret_cast<const char*>(section.memory);
      auto numExports = byte_swap(*secData++);
      auto exportsCrc = byte_swap(*secData++);

      for (auto i = 0u; i < numExports; ++i) {
         auto exportsAddr = byte_swap(*secData++);
         auto exportNameOff = byte_swap(*secData++);
         auto exportsName = secNames + exportNameOff;

         loadedMod->exports.emplace(exportsName, exportsAddr);
         loadedMod->symbols.emplace(exportsName, exportsAddr);
      }
   }

   return true;
}

std::unique_ptr<LoadedModule>
Loader::loadRPL(const std::string& name, const gsl::array_view<uint8_t> &data)
{
   std::vector<uint8_t> buffer;
   auto in = BigEndianView { data };
   auto loadedMod = std::make_unique<LoadedModule>();

   // Read header
   auto header = elf::Header {};

   if (!elf::readHeader(in, header)) {
      gLog->error("Failed elf::readHeader");
      return nullptr;
   }

   // Check it is a CAFE abi rpl
   if (header.abi != elf::EABI_CAFE) {
      gLog->error("Unexpected elf abi found {:02x} expected {:02x}", header.abi, elf::EABI_CAFE);
      return nullptr;
   }

   // Read sections
   auto sections = std::vector<elf::XSection> {};

   if (!elf::readSectionHeaders(in, header, sections)) {
      gLog->error("Failed elf::readSectionHeaders");
      return nullptr;
   }

   // Allocate memory segments for load / code / data
   void *codeSegAddr, *loadSegAddr, *dataSegAddr;
   elf::FileInfo info;

   readFileInfo(in, sections, info);

   codeSegAddr = mCodeHeap->alloc(info.textSize, info.textAlign);
   loadSegAddr = OSAllocFromSystem(info.loadSize, info.loadAlign);

   if (OSDynLoad_MemAlloc(info.dataSize, info.dataAlign, &dataSegAddr) != 0) {
      dataSegAddr = nullptr;
   }

   assert(dataSegAddr);
   assert(loadSegAddr);
   assert(codeSegAddr);

   auto codeSeg = SequentialMemoryTracker { codeSegAddr, info.textSize };
   auto dataSeg = SequentialMemoryTracker { dataSegAddr, info.dataSize };
   auto loadSeg = SequentialMemoryTracker { loadSegAddr, info.loadSize };

   // Allocate sections from our memory segments
   for (auto &section : sections) {
      if (section.header.flags & elf::SHF_ALLOC) {
         void *allocData = nullptr;

         // Read the section data
         if (section.header.type == elf::SHT_NOBITS) {
            buffer.clear();
            buffer.resize(section.header.size, 0);
         } else {
            if (!elf::readSectionData(in, section.header, buffer)) {
               gLog->error("Failed to read section data");
               return nullptr;
            }
         }

         // Allocate from correct memory segment
         if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
            if (section.header.flags & elf::SHF_EXECINSTR) {
               allocData = codeSeg.get(buffer.size(), section.header.addralign);
            } else {
               allocData = dataSeg.get(buffer.size(), section.header.addralign);
            }
         } else {
            allocData = loadSeg.get(buffer.size(), section.header.addralign);
         }

         memcpy(allocData, buffer.data(), buffer.size());
         section.memory = reinterpret_cast<uint8_t*>(allocData);
         section.virtAddress = mem::untranslate(allocData);
         section.virtSize = static_cast<uint32_t>(buffer.size());
      }
   }

   // Read strtab
   auto shStrTab = reinterpret_cast<const char*>(sections[header.shstrndx].memory);

   // Calculate SDA Bases
   auto sdata = findSection(sections, shStrTab, ".sdata");
   auto sbss = findSection(sections, shStrTab, ".sbss");
   auto sdata2 = findSection(sections, shStrTab, ".sdata2");
   auto sbss2 = findSection(sections, shStrTab, ".sbss2");
   loadedMod->sdaBase = calculateSdaBase(sdata, sbss);
   loadedMod->sda2Base = calculateSdaBase(sdata2, sbss2);

   // Process Imports
   if (!processImports(loadedMod.get(), sections)) {
      gLog->error("Error loading imports");
      return nullptr;
   }

   // Process relocations
   auto trampSeg = AddressRange {};

   if (!processRelocations(loadedMod.get(), sections, in, shStrTab, codeSeg, trampSeg)) {
      gLog->error("Error loading relocations");
      return nullptr;
   }

   // Relocate entry point
   auto entryPoint = header.entry;

   for (auto &section : sections) {
      if (section.header.addr <= entryPoint && section.header.addr + section.virtSize > entryPoint) {
         if (section.virtAddress >= section.header.addr) {
            entryPoint += section.virtAddress - section.header.addr;
         } else {
            entryPoint -= section.header.addr - section.virtAddress;
         }

         break;
      }
   }

   // Process exports
   if (!processExports(loadedMod.get(), sections)) {
      gLog->error("Error loading exports");
      return nullptr;
   }

   // Create sections list
   for (auto &section : sections) {
      if (section.header.flags & elf::SHF_ALLOC) {
         if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
            auto sectionName = shStrTab + section.header.name;
            auto start = section.virtAddress;
            auto end = section.virtAddress + section.virtSize;
            loadedMod->sections.emplace_back(LoadedSection { sectionName, start, end });
         }
      }
   }

   if (trampSeg.second > trampSeg.first) {
      loadedMod->sections.emplace_back(LoadedSection { "loader_thunks", trampSeg.first, trampSeg.second });
   }

   // Free the load segment
   OSFreeToSystem(loadSegAddr);
   //mCodeHeap->free(loadSegAddr);

   loadedMod->name = name;
   loadedMod->defaultStackSize = info.stackSize;
   loadedMod->entryPoint = entryPoint;
   loadedMod->handle = OSAllocFromSystem<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod.get();
   return loadedMod;
}
