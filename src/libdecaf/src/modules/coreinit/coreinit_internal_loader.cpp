#include "coreinit_internal_loader.h"
#include "common/strutils.h"
#include "kernel/kernel_hle.h"
#include "kernel/kernel_hlemodule.h"
#include "kernel/kernel_hlefunction.h"
#include "kernel/kernel_filesystem.h"
#include "modules/coreinit/coreinit_memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "filesystem/filesystem.h"
#include "common/align.h"
#include "common/bigendianview.h"
#include "common/teenyheap.h"
#include "elf.h"

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
      auto alignOffset = align_up(mPtr, alignment) - mPtr;
      size += alignOffset;

      // Double-check alignment
      auto ptrOut = mPtr + alignOffset;
      assert(align_up(ptrOut, alignment) == ptrOut);

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

namespace coreinit
{

namespace internal
{

using TrampolineMap = std::map<ppcaddr_t, ppcaddr_t>;
using SectionList = std::vector<elf::XSection>;
using AddressRange = std::pair<ppcaddr_t, ppcaddr_t>;

static std::map<std::string, LoadedModule*>
gLoadedModules;

static LoadedModule *
gUserModule;

static TeenyHeap*
gCodeHeap;

static std::map<std::string, ppcaddr_t>
gUnimplementedFunctions;

static std::map<std::string, int>
gUnimplementedData;

void initialiseCodeHeap(ppcsize_t maxCodeSize)
{
   // Get the MEM2 Region
   be_val<uint32_t> mem2start, mem2size;
   coreinit::OSGetMemBound(OSMemoryType::MEM2, &mem2start, &mem2size);

   // Steal some space for code heap
   gCodeHeap = new TeenyHeap(mem::translate(mem2start), maxCodeSize);

   // Update MEM2 to ignore the code heap region
   coreinit::internal::setMemBound(OSMemoryType::MEM2, mem2start + maxCodeSize, mem2size - maxCodeSize);
}

std::map<std::string, LoadedModule*> getLoadedModules()
{
   return gLoadedModules;
}

LoadedModule *loadRPLNoLock(const std::string& name);

static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod, SequentialMemoryTracker &codeSeg, TrampolineMap &trampolines, void *target, const std::string& symbolName);

static bool
readFileInfo(BigEndianView &in, const SectionList &sections, elf::FileInfo &info);

static const elf::XSection *
findSection(const SectionList &sections, const char *shStrTab, const std::string &name);

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

      BigEndianView be_view{ gsl::as_span(data) };
      elf::readFileInfo(be_view, info);
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
      auto b = espresso::encodeInstruction(espresso::InstructionID::b);
      b.li = delta >> 2;
      b.lk = 0;
      b.aa = 0;
      *tramp = byte_swap(b.value);
   } else if (targetAddr < 0x03fffffc) {
      tramp = static_cast<uint32_t*>(codeSeg.get(4));

      // Absolute jump using b
      auto b = espresso::encodeInstruction(espresso::InstructionID::b);
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


ppcaddr_t
generateUnimplementedDataThunk(const std::string &module, const std::string& name)
{
   auto itr = gUnimplementedData.find(name);

   if (itr != gUnimplementedData.end()) {
      return itr->second | 0x800;
   }

   auto id = gsl::narrow_cast<uint32_t>(gUnimplementedData.size());
   auto fakeAddr = 0xFFF00000 | (id << 12);
   assert(id <= 0xFF);

   gLog->info("Unimplemented data symbol {}::{} at {:08x}", module, name, fakeAddr);

   gUnimplementedData.emplace(name, fakeAddr);
   return fakeAddr | 0x800;
}


ppcaddr_t
generateUnimplementedFunctionThunk(const std::string &module, const std::string &func)
{
   auto itr = gUnimplementedFunctions.find(func);

   if (itr != gUnimplementedFunctions.end()) {
      return itr->second;
   }

   auto id = kernel::registerUnimplementedHleFunc(module, func);
   auto thunk = static_cast<uint32_t*>(coreinit::internal::sysAlloc(8, 4));
   auto addr = mem::untranslate(thunk);

   // Write syscall thunk
   auto kc = espresso::encodeInstruction(espresso::InstructionID::kc);
   kc.kcn = id;
   *(thunk + 0) = byte_swap(kc.value);

   auto bclr = espresso::encodeInstruction(espresso::InstructionID::bclr);
   bclr.bo = 20;
   bclr.bi = 0;
   *(thunk + 1) = byte_swap(bclr.value);

   gLog->info("Unimplemented function {}::{} at {:08x}", module, func, addr);
   gUnimplementedFunctions.emplace(func, addr);
   return addr;
}


// Load a kernel module into virtual memory space by creating thunks
LoadedModule *
loadHleModule(const std::string &moduleName,
   const std::string &name,
   kernel::HleModule *module)
{
   std::vector<kernel::HleFunction*> funcSymbols;
   std::vector<kernel::HleData*> dataSymbols;
   uint32_t dataSize = 0, codeSize = 0;
   auto &symbols = module->getSymbols();

   for (auto &symbol : symbols) {
      if (symbol->type == kernel::HleSymbol::Function) {
         auto funcSymbol = static_cast<kernel::HleFunction*>(symbol);
         codeSize += 8;
         funcSymbols.emplace_back(funcSymbol);
      } else if (symbol->type == kernel::HleSymbol::Data) {
         auto dataSymbol = static_cast<kernel::HleData*>(symbol);
         dataSize += dataSymbol->size;
         dataSymbols.emplace_back(dataSymbol);
      } else {
         gLog->error("Unexpected HleSymbol type");
         return nullptr;
      }
   }

   // Create module
   auto loadedMod = new LoadedModule{};
   gLoadedModules.emplace(moduleName, loadedMod);
   loadedMod->name = name;
   loadedMod->handle = coreinit::internal::sysAlloc<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod;

   // Load code section
   if (codeSize > 0) {
      auto codeRegion = static_cast<uint8_t*>(coreinit::internal::sysAlloc(codeSize, 4));
      auto start = mem::untranslate(codeRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection{ ".text", start, end });

      for (auto &func : funcSymbols) {
         // Allocate some space for the thunk
         auto thunk = reinterpret_cast<uint32_t*>(codeRegion);
         auto addr = mem::untranslate(thunk);
         codeRegion += 8;

         // Write syscall thunk
         auto kc = espresso::encodeInstruction(espresso::InstructionID::kc);
         kc.kcn = func->syscallID;
         *(thunk + 0) = byte_swap(kc.value);

         auto bclr = espresso::encodeInstruction(espresso::InstructionID::bclr);
         bclr.bo = 20;
         bclr.bi = 0;
         *(thunk + 1) = byte_swap(bclr.value);

         // Add to symbols list
         loadedMod->symbols.emplace(func->name, addr);

         // Save the PPC ptr for internal lookups
         func->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         if (func->hostPtr) {
            *reinterpret_cast<ppcaddr_t*>(func->hostPtr) = addr;
         }
      }
   }

   // Load data section
   if (dataSize > 0) {
      auto dataRegion = static_cast<uint8_t*>(coreinit::internal::sysAlloc(dataSize, 4));
      auto start = mem::untranslate(dataRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection{ ".data", start, end });

      for (auto &data : dataSymbols) {
         // Allocate the same for this export
         auto thunk = dataRegion;
         auto addr = mem::untranslate(thunk);
         dataRegion += data->size;

         // Add to exports list
         loadedMod->symbols.emplace(data->name, addr);

         // Save the PPC ptr for internal lookups
         data->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         if (data->hostPtr) {
            *reinterpret_cast<void**>(data->hostPtr) = thunk;
         }
      }
   }

   // Export everything which is an export
   auto &exports = module->getExports();
   for (auto &exp : exports) {
      loadedMod->exports.emplace(exp->name, mem::untranslate(exp->ppcPtr));
   }

   module->initialise();
   return loadedMod;
}

bool
processRelocations(LoadedModule *loadedMod, const SectionList &sections, BigEndianView &in, const char *shStrTab, SequentialMemoryTracker &codeSeg, AddressRange &trampSeg)
{
   auto trampolines = TrampolineMap{};
   auto buffer = std::vector<uint8_t>{};
   trampSeg.first = codeSeg.getCurrentAddr();

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      elf::readSectionData(in, section.header, buffer);
      auto in = BigEndianView{ gsl::as_span(buffer) };

      auto &symSec = sections[section.header.link];
      auto &targetSec = sections[section.header.info];
      auto symSecView = BigEndianView{ symSec.memory, symSec.virtSize };
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
         auto symbolName = reinterpret_cast<const char*>(symStrTab.memory) + symbol.name;

         auto symAddr = getSymbolAddress(symbol, sections);
         symAddr += rela.addend;

         if (symbolSection.header.type == elf::SHT_RPL_IMPORTS) {
            symAddr = mem::read<uint32_t>(symAddr);

            if (symAddr == 0) {
               symAddr = generateUnimplementedDataThunk(symbolSection.name, symbolName);
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
            auto ins = espresso::Instruction{ byte_swap(*ptr32) };
            auto data = espresso::decodeInstruction(ins);

            // Our REL24 trampolines only work for a branch instruction...
            assert(data->id == espresso::InstructionID::b);

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
            auto ins = espresso::Instruction{ byte_swap(*ptr32) };
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
processImports(LoadedModule *loadedMod, SectionList &sections)
{
   std::map<std::string, ppcaddr_t> symbolTable;

   // Process import sections
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_IMPORTS) {
         continue;
      }

      // Load library
      auto libraryName = reinterpret_cast<const char*>(section.memory + 8);
      auto linkedModule = loadRPLNoLock(libraryName);

      // Zero the whole section after we have used the name
      section.name = libraryName;
      std::memset(section.memory + 8, 0, section.virtSize - 8);

      if (!linkedModule) {
         // Ignore missing modules for now
         gLog->debug("Missing library {}", section.name);
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
      auto symIn = BigEndianView{ section.memory, section.virtSize };

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

         if (sym.shndx >= elf::SHN_LORESERVE) {
            gLog->warn("Symbol {} in invalid section 0x{:X}", name, sym.shndx);
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
                  symbolAddr = generateUnimplementedFunctionThunk(impsec.name, name);
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
processExports(LoadedModule *loadedMod, const SectionList &sections)
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


LoadedModule *
loadRPL(const std::string &moduleName, const std::string &name, const gsl::span<uint8_t> &data)
{
   std::vector<uint8_t> buffer;
   auto in = BigEndianView{ data };
   auto loadedMod = new LoadedModule();
   gLoadedModules.emplace(moduleName, loadedMod);

   // Read header
   auto header = elf::Header{};

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
   auto sections = std::vector<elf::XSection>{};

   if (!elf::readSectionHeaders(in, header, sections)) {
      gLog->error("Failed elf::readSectionHeaders");
      return nullptr;
   }

   // Allocate memory segments for load / code / data
   void *codeSegAddr, *loadSegAddr, *dataSegAddr;
   elf::FileInfo info;

   readFileInfo(in, sections, info);
   codeSegAddr = gCodeHeap->alloc(info.textSize, info.textAlign);
   loadSegAddr = coreinit::internal::sysAlloc(info.loadSize, info.loadAlign);

   if (coreinit::internal::dynLoadMemAlloc(info.dataSize, info.dataAlign, &dataSegAddr) != 0) {
      dataSegAddr = nullptr;
   }

   assert(dataSegAddr);
   assert(loadSegAddr);
   assert(codeSegAddr);

   auto codeSeg = SequentialMemoryTracker{ codeSegAddr, info.textSize };
   auto dataSeg = SequentialMemoryTracker{ dataSegAddr, info.dataSize };
   auto loadSeg = SequentialMemoryTracker{ loadSegAddr, info.loadSize };

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
   if (!shStrTab) {
      gLog->error("Section name table missing");
      return nullptr;
   }

   // Calculate SDA Bases
   auto sdata = findSection(sections, shStrTab, ".sdata");

   if (sdata) {
      loadedMod->sdaBase = sdata->virtAddress + 0x8000;
   }

   auto sdata2 = findSection(sections, shStrTab, ".sdata2");

   if (sdata2) {
      loadedMod->sda2Base = sdata2->virtAddress + 0x8000;
   }

   // Process exports
   if (!processExports(loadedMod, sections)) {
      gLog->error("Error loading exports");
      return nullptr;
   }

   // Process Imports
   if (!processImports(loadedMod, sections)) {
      gLog->error("Error loading imports");
      return nullptr;
   }

   // Process relocations
   auto trampSeg = AddressRange{};

   if (!processRelocations(loadedMod, sections, in, shStrTab, codeSeg, trampSeg)) {
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

   // Create sections list
   for (auto &section : sections) {
      if (section.header.flags & elf::SHF_ALLOC) {
         if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
            auto sectionName = shStrTab + section.header.name;
            auto start = section.virtAddress;
            auto end = section.virtAddress + section.virtSize;
            loadedMod->sections.emplace_back(LoadedSection{ sectionName, start, end });
         }
      }
   }

   if (trampSeg.second > trampSeg.first) {
      loadedMod->sections.emplace_back(LoadedSection{ "loader_thunks", trampSeg.first, trampSeg.second });
   }

   // Free the load segment
   coreinit::internal::sysFree(loadSegAddr);
   //mCodeHeap->free(loadSegAddr);

   loadedMod->name = name;
   loadedMod->defaultStackSize = info.stackSize;
   loadedMod->entryPoint = entryPoint;
   loadedMod->handle = coreinit::internal::sysAlloc<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod;
   return loadedMod;
}

void normalizeModuleName(const std::string& name, std::string& moduleName, std::string& fileName)
{
   if (!ends_with(name, ".rpl") && !ends_with(name, ".rpx")) {
      moduleName = name;
      fileName = name + ".rpl";
   } else {
      moduleName = name.substr(0, name.size() - 4);
      fileName = name;
   }
}

LoadedModule *loadRPLNoLock(const std::string& name)
{
   LoadedModule *module = nullptr;
   std::string moduleName;
   std::string fileName;

   normalizeModuleName(name, moduleName, fileName);

   // Check if we already have this module loaded
   auto itr = gLoadedModules.find(moduleName);

   if (itr != gLoadedModules.end()) {
      return itr->second;
   }

   // Try to find module in system kernel library list
   if (!module) {
      auto kernelModule = kernel::findHleModule(fileName);

      if (kernelModule) {
         module = loadHleModule(moduleName, fileName, kernelModule);
      }
   }

   // Try to find module in the game code directory
   if (!module) {
      auto fs = kernel::getFileSystem();
      auto fh = fs->openFile("/vol/code/" + fileName, fs::File::Read);

      if (fh) {
         auto buffer = std::vector<uint8_t>(fh->size());
         fh->read(buffer.data(), buffer.size(), 1);
         fh->close();

         module = loadRPL(moduleName, fileName, buffer);
      }
   }

   if (!module) {
      gLog->error("Failed to load module {}", fileName);
      gLoadedModules.erase(moduleName);
      return nullptr;
   } else {
      gLog->info("Loaded module {}", fileName);
      return module;
   }
}

LoadedModule *loadRPL(const std::string& name)
{
   // Use the scheduler lock to protect access to the loaders memory
   internal::lockScheduler();
   auto res = loadRPLNoLock(name);
   internal::unlockScheduler();
   return res;
}

LoadedModule * loadRPX(ppcsize_t maxCodeSize, const std::string& name)
{
   // Initialise the code-heap information
   initialiseCodeHeap(maxCodeSize);

   // Force-load coreinit which is a default system-loaded library
   // Note that this needs to be here as its DefaultHeapInit method
   // makes assumptions about the MEM bounds which are changed by the
   // initialiseCodeHeap call above.
   loadRPL("coreinit");

   // Load the RPX as a normal module
   gUserModule = loadRPL(name);
   return gUserModule;
}

LoadedModule * findModule(const std::string& name)
{
   std::string moduleName;
   std::string fileName;
   normalizeModuleName(name, moduleName, fileName);

   // Check if we already have this module loaded
   auto itr = gLoadedModules.find(moduleName);

   if (itr == gLoadedModules.end()) {
      return nullptr;
   }

   return itr->second;
}

LoadedModule * getUserModule()
{
   return gUserModule;
}

} // namespace internal

} // namespace coreinit
