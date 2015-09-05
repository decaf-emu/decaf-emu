#define ZLIB_CONST
#include <cassert>
#include <limits>
#include <algorithm>
#include <string>
#include <vector>
#include <zlib.h>
#include "bigendianview.h"
#include "elf.h"
#include "filesystem/filesystem.h"
#include "instructiondata.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "system.h"
#include "systemtypes.h"
#include "kernelmodule.h"
#include "usermodule.h"
#include "util.h"
#include "teenyheap.h"

Loader gLoader;

static bool
readFileInfo(BigEndianView &in, std::vector<elf::XSection> &sections, elf::FileInfo &info)
{
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_FILEINFO) {
         continue;
      }

      std::vector<uint8_t> fileInfoData;
      elf::readSectionData(in, section.header, fileInfoData);

      auto fileInfoIn = BigEndianView{ fileInfoData.data(), fileInfoData.size() };
      elf::readFileInfo(fileInfoIn, info);
      return true;
   }

   gLog->error("Failed to find RPLFileInfo section");
   return false;
}

void
Loader::initialise(ppcsize_t maxCodeSize)
{
   // Get the MEM2 Region
   be_val<uint32_t> mem2start, mem2size;
   OSGetMemBound(OSMemoryType::MEM2, &mem2start, &mem2size);

   // Allocate MEM2 Region
   gMemory.alloc(mem2start, mem2size);

   // Steal some space for code heap
   mCodeHeap = new TeenyHeap(gMemory.translate(mem2start), maxCodeSize);

   // Update MEM2 to ignore the code heap region
   OSSetMemBound(OSMemoryType::MEM2, mem2start + maxCodeSize, mem2size - maxCodeSize);
}

template<typename... Args>
static void
debugPrint(fmt::StringRef msg, Args... args) {
   auto out = fmt::format(msg, args...);
   out += "\n";
   OutputDebugStringA(out.c_str());
}

void
Loader::debugPrint()
{
   struct MySection {
      LoadedModule *module;
      LoadedSection *section;
   };
   std::vector<MySection> sections;

   for (auto &module : mModules) {
      for (auto &section : module.second->mSections) {
         sections.emplace_back(MySection{ module.second, &section });
      }
   }

   std::sort(sections.begin(), sections.end(), [](const MySection& i, const MySection& j) {
      return i.section->start < j.section->start;
   });

   ::debugPrint("Sections:");
   for (auto &section : sections) {
      ::debugPrint("[{:08x}:{:08x}] {:<16} {}",
         gMemory.untranslate(section.section->start),
         gMemory.untranslate(section.section->end),
         section.module->getName().c_str(),
         section.section->name.c_str());
   }
}

LoadedModule*
Loader::loadRPL(const std::string& name)
{
   // Strip the .rpl extension
   std::string moduleName = name;
   if (moduleName.size() < 4 || 
      (moduleName.substr(moduleName.size() - 4) != ".rpl" && 
         moduleName.substr(moduleName.size() - 4) != ".rpx")) {
      moduleName = moduleName + ".rpl";
   }

   // Check if we already have this module loaded
   auto moduleIter = mModules.find(moduleName);
   if (moduleIter != mModules.end()) {
      return moduleIter->second;
   }

   // Try to load the module from somewhere
   LoadedModule * loadedModule = nullptr;

   // Try to find module in system kernel library list
   if (!loadedModule) {
      KernelModule *kernelModule = gSystem.findModule(moduleName.c_str());
      if (kernelModule) {
         // Found a kernel module with this name.
         loadedModule = loadKernelModule(moduleName, kernelModule);
      }
   }

   // Try to find module in the game code directory
   if (!loadedModule) {
      // Read rpx file
      auto fs = gSystem.getFileSystem();
      auto fh = fs->openFile("/vol/code/" + moduleName, fs::File::Read);

      if (fh) {
         auto buffer = std::vector<uint8_t>(fh->size());
         fh->read(reinterpret_cast<char *>(buffer.data()), buffer.size());
         fh->close();

         loadedModule = loadRPL(moduleName, buffer);
      }
   }

   if (!loadedModule) {
      gLog->error("Failed to load module {}", moduleName);
      return nullptr;
   }

   mModules.emplace(moduleName, loadedModule);

   gLog->info("Loaded module {}", moduleName);
   return loadedModule;
}

const elf::XSection* findSection(const std::vector<elf::XSection>& sections, const char *shStrTab, const std::string& name) {
   for (auto &section : sections) {
      auto sectionName = shStrTab + section.header.name;
      if (sectionName == name) {
         return &section;
      }
   }
   return nullptr;
}

void * calculateSdaBase(const elf::XSection *sdata, const elf::XSection *sbss) {
   uint8_t * start;
   uint8_t * end;

   if (sdata && sbss) {
      auto sdataAddr = static_cast<uint8_t*>(sdata->virtAddress);
      auto sbssAddr = static_cast<uint8_t*>(sbss->virtAddress);
      start = std::min(sdataAddr, sbssAddr);
      end = std::max(sdataAddr + sdata->virtSize, sbssAddr + sbss->virtSize);
   } else if (sdata) {
      auto sdataAddr = static_cast<uint8_t*>(sdata->virtAddress);
      start = sdataAddr;
      end = sdataAddr + sdata->virtSize;
   } else if (sbss) {
      auto sbssAddr = static_cast<uint8_t*>(sbss->virtAddress);
      start = sbssAddr;
      end = sbssAddr + sbss->virtSize;
   } else {
      return nullptr;
   }

   if (end - start > 0xffff) {
      gLog->error("small data and bss do not fit in a 16 bit range");
   }

   return ((end - start) / 2) + start;
}

static elf::Symbol
getSymbol(BigEndianView &symSecView, uint32_t index)
{
   elf::Symbol sym;
   symSecView.seek(index * sizeof(elf::Symbol));
   elf::readSymbol(symSecView, sym);
   return sym;
}

static uint32_t
getSymbolAddress(const elf::Symbol& sym, std::vector<elf::XSection>& sections)
{
   auto symSec = sections[sym.shndx];
   uint32_t virtAddr = gMemory.untranslate(symSec.virtAddress);
   return sym.value - symSec.header.addr + virtAddr;
}

LoadedModule *
Loader::loadKernelModule(const std::string& name, KernelModule *module)
{
   KernelExportMap exports = module->getExportMap();

   std::vector<KernelFunction*> funcExports;
   std::vector<KernelData*> dataExports;
   std::map<std::string, void*> exportsMap;
   std::map<std::string, void*> symbolsMap;
   uint32_t dataSize = 0;

   for (auto &i : exports) {
      auto exportInfo = i.second;
      if (exportInfo->type == KernelExport::Function) {
         auto funcExport = static_cast<KernelFunction*>(exportInfo);
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

   std::vector<LoadedSection> loadedSections;
   
   uint32_t codeSize = (uint32_t)funcExports.size() * 8;
   uint8_t *codeRegion = nullptr;
   if (codeSize > 0) {
      codeRegion = static_cast<uint8_t*>(OSAllocFromSystem(codeSize, 4));

      loadedSections.emplace_back(LoadedSection{ ".text", codeRegion, codeRegion + codeSize });

      for (auto &func : funcExports) {
         // Allocate some space for the thunk
         uint32_t *thunkAddr = reinterpret_cast<uint32_t*>(codeRegion);
         codeRegion += 8;

         // Write syscall thunk
         auto kc = gInstructionTable.encode(InstructionID::kc);
         kc.li = func->syscallID;
         kc.aa = 1;
         *(thunkAddr + 0) = byte_swap(kc.value);

         auto bclr = gInstructionTable.encode(InstructionID::bclr);
         bclr.bo = 0x1f;
         *(thunkAddr + 1) = byte_swap(bclr.value);

         // Add to exports list
         exportsMap.emplace(func->name, thunkAddr);
         symbolsMap.emplace(func->name, thunkAddr);

         // Save the PPC ptr for internal lookups
         func->ppcPtr = thunkAddr;
      }
   }

   uint8_t *dataRegion = nullptr;
   if (dataSize > 0) {
      dataRegion = static_cast<uint8_t*>(OSAllocFromSystem(dataSize, 4));

      loadedSections.emplace_back(LoadedSection{ ".data", dataRegion, dataRegion + dataSize });

      for (auto &data : dataExports) {
         // Allocate the same for this export
         uint8_t *dataAddr = dataRegion;
         dataRegion += data->size;

         // Add to exports list
         exportsMap.emplace(data->name, dataAddr);
         symbolsMap.emplace(data->name, dataAddr);

         // Save the PPC ptr for internal lookups
         data->ppcPtr = dataAddr;

         // Map host memory pointer to PPC region
         *data->hostPtr = dataAddr;
      }
   }

   module->initialise();

   LoadedModule *loadedMod = new LoadedModule();
   loadedMod->mName = name;
   loadedMod->mSdaBase = 0;
   loadedMod->mSda2Base = 0;
   loadedMod->mDefaultStackSize = 0;
   loadedMod->mEntryPoint = 0;
   loadedMod->mExports = exportsMap;
   loadedMod->mSymbols = symbolsMap;
   loadedMod->mSections = loadedSections;
   loadedMod->mHandle = OSAllocFromSystem<LoadedModuleHandleData>();
   loadedMod->mHandle->ptr = loadedMod;
   return loadedMod;
}

uint32_t
Loader::registerUnimplementedData(const std::string& name)
{
   auto thunkIter = mUnimplementedData.find(name);
   if (thunkIter != mUnimplementedData.end()) {
      return thunkIter->second | 0x800;
   }

   int newId = (int)mUnimplementedData.size();
   assert(newId <= 0xFF);
   uint32_t fakeAddr = 0xFFF00000 | (newId << 12);

   mUnimplementedData.emplace(name, fakeAddr);
   return fakeAddr | 0x800;
}

const char *
Loader::getUnimplementedData(uint32_t addr)
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

void *
Loader::registerUnimplementedFunction(const std::string& name)
{
   auto thunkIter = mUnimplementedFunctions.find(name);
   if (thunkIter != mUnimplementedFunctions.end()) {
      return thunkIter->second;
   }

   uint32_t syscallId = gSystem.registerUnimplementedFunction(name.c_str());

   uint32_t *thunkAddr = static_cast<uint32_t*>(OSAllocFromSystem(8, 4));

   // Write syscall thunk
   auto kc = gInstructionTable.encode(InstructionID::kc);
   kc.li = syscallId;
   kc.aa = 0;
   *(thunkAddr + 0) = byte_swap(kc.value);

   auto bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   *(thunkAddr + 1) = byte_swap(bclr.value);

   mUnimplementedFunctions.emplace(name, thunkAddr);
   return thunkAddr;
}

class SequentialMemoryTracker {
public:
   SequentialMemoryTracker(void *ptr, size_t size)
      : mStart(static_cast<uint8_t*>(ptr)), mEnd(mStart + size), mPtr(mStart)
   {
   }

   void * getCurrentAddr() const {
      return mPtr;
   }

   void * get(size_t size, uint32_t alignment = 4) {
      // Ensure section alignment
      auto alignOffset = alignUp(mPtr, alignment) - mPtr;
      size += alignOffset;

      // Double-check alignment
      void * ptrOut = mPtr + alignOffset;
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

LoadedModule*
Loader::loadRPL(const std::string& name, const std::vector<uint8_t> data)
{
   auto in = BigEndianView{ data.data(), data.size() };

   std::map<std::string, void*> symbolsMap;

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
   
   // Read FileInfo data
   elf::FileInfo info;
   readFileInfo(in, sections, info);

   void *codeSegAddr = mCodeHeap->alloc(info.textSize, info.textAlign);
   assert(codeSegAddr);
   SequentialMemoryTracker codeSeg(codeSegAddr, info.textSize);

   void *dataSegAddr = nullptr;
   if (OSDynLoad_MemAlloc(info.dataSize, info.dataAlign, &dataSegAddr) != 0) {
      dataSegAddr = nullptr;
   }
   assert(dataSegAddr);
   SequentialMemoryTracker dataSeg(dataSegAddr, info.dataSize);

   void *loadSegAddr = mCodeHeap->alloc(info.loadSize, info.loadAlign);
   assert(loadSegAddr);
   SequentialMemoryTracker loadSeg(loadSegAddr, info.loadSize);

   // Allocate
   {
      std::vector<uint8_t> sectionData;

      for (auto& section : sections) {
         if (section.header.flags & elf::SHF_ALLOC) {
            if (section.header.type == elf::SHT_NOBITS) {
               sectionData.clear();
               sectionData.resize(section.header.size, 0);
            } else {
               if (!elf::readSectionData(in, section.header, sectionData)) {
                  gLog->error("Failed to decompressed allocatable section");
                  return nullptr;
               }
            }

            void *allocData = nullptr;
            if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
               if (section.header.flags & elf::SHF_EXECINSTR) {
                  allocData = codeSeg.get(sectionData.size(), section.header.addralign);
               } else {
                  allocData = dataSeg.get(sectionData.size(), section.header.addralign);
               }
            } else {
               allocData = loadSeg.get(sectionData.size(), section.header.addralign);
            }

            memcpy(allocData, sectionData.data(), sectionData.size());
            section.virtAddress = allocData;
            section.virtSize = static_cast<uint32_t>(sectionData.size());
         }
      }
   }

   // I am a bad person and I should feel bad
   std::map<void*, void*> trampolines;
   void * trampSegStart = codeSeg.getCurrentAddr();
   auto getTramp = [&](void *target, const std::string& symbolName) {
      auto trampIter = trampolines.find(target);
      if (trampIter != trampolines.end()) {
         return trampIter->second;
      }

      uint32_t *trampAddr = static_cast<uint32_t*>(codeSeg.getCurrentAddr());
      uint32_t *targetAddr = static_cast<uint32_t*>(target);

      intptr_t delta = reinterpret_cast<uint8_t*>(targetAddr) - reinterpret_cast<uint8_t*>(trampAddr);
      if (delta > -0x1fffffc && delta < 0x1fffffc) {
         trampAddr = static_cast<uint32_t*>(codeSeg.get(4));
         
         // Short jump using b
         auto b = gInstructionTable.encode(InstructionID::b);
         b.li = delta >> 2;
         b.lk = 0;
         b.aa = 0;
         *trampAddr = byte_swap(b.value);
      } else if (gMemory.untranslate(targetAddr) < 0x03fffffc) {
         trampAddr = static_cast<uint32_t*>(codeSeg.get(4));

         // Absolute jump using b
         auto b = gInstructionTable.encode(InstructionID::b);
         b.li = gMemory.untranslate(targetAddr) >> 2;
         b.lk = 0;
         b.aa = 1;
         *trampAddr = byte_swap(b.value);
      } else {
         // need to implement 16-byte long-jumping here...
         assert(0);
      }

      symbolsMap.emplace(symbolName + "#thunk", trampAddr);

      trampolines.emplace(targetAddr, trampAddr);
      return static_cast<void*>(trampAddr);
   };

   // Read strtab
   const char *shStrTab = static_cast<const char*>(sections[header.shstrndx].virtAddress);

   // Calculate SDA Bases
   auto sdata = findSection(sections, shStrTab, ".sdata");
   auto sbss = findSection(sections, shStrTab, ".sbss");
   auto sdata2 = findSection(sections, shStrTab, ".sdata2");
   auto sbss2 = findSection(sections, shStrTab, ".sbss2");
   void * sdaBase = calculateSdaBase(sdata, sbss);
   void * sda2Base = calculateSdaBase(sdata2, sbss2);

   // Process Imports
   std::map<std::string, void*> symbolTable;
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_IMPORTS) {
         continue;
      }

      // Grab the name of the library
      const char *libraryName = static_cast<const char*>(section.virtAddress) + 8;

      // Try to acquire this module
      LoadedModule *linkedModule = loadRPL(libraryName);
      
      // Zero the whole section after we have used the name
      memset(static_cast<uint8_t*>(section.virtAddress) + 8, section.virtSize - 8, 0);
      
      if (!linkedModule) {
         // Ignore missing modules for now
         continue;
      }

      // Add all these symbols to the symbol table
      for (auto &linkedExport : linkedModule->mExports) {
         symbolTable.insert(linkedExport);
      }
   }

   // Process Import Symbols
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_SYMTAB) {
         continue;
      }

      const char *strTab = static_cast<const char*>(sections[section.header.link].virtAddress);

      auto symIn = BigEndianView{ section.virtAddress, section.virtSize };
      elf::Symbol sym;
      while (!symIn.eof()) {
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
         auto baseAddress = gMemory.untranslate(impsec.virtAddress);
         auto virtAddress = baseAddress + offset;

         if (impsec.header.type == elf::SHT_NULL) {
            continue;
         }

         if (impsec.header.type == elf::SHT_RPL_IMPORTS) {
            auto symbolTargetIter = symbolTable.find(name);
            void * symbolAddr = nullptr;
            if (symbolTargetIter == symbolTable.end()) {
               if (type == elf::STT_FUNC) {
                  symbolAddr = registerUnimplementedFunction(name);
               } else {
                  symbolAddr = nullptr;
               }
            } else {
               if (type != elf::STT_FUNC && type != elf::STT_OBJECT) {
                  assert(0);
               }

               symbolAddr = symbolTargetIter->second;
            }

            // Write the symbol addres into .fimport or .dimport
            gMemory.write(virtAddress, gMemory.untranslate(symbolAddr));
         }
      }
   }

   // Process relocations
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      auto &symSec = sections[section.header.link];
      auto &targetSec = sections[section.header.info];
      auto symSecView = BigEndianView{ symSec.virtAddress, symSec.virtSize };
      auto &symStrTab = sections[symSec.header.link];

      uint32_t sdaBaseAddr = gMemory.untranslate(sdaBase);
      uint32_t sda2BaseAddr = gMemory.untranslate(sda2Base);

      std::vector<uint8_t> data;
      elf::readSectionData(in, section.header, data);
      auto reloIn = BigEndianView{ data.data(), data.size() };

      auto targetBaseAddr = targetSec.header.addr;
      auto targetVirtAddr = gMemory.untranslate(targetSec.virtAddress);

      elf::Rela rela;
      while (!reloIn.eof()) {
         elf::readRelocationAddend(reloIn, rela);

         auto index = rela.info >> 8;
         auto type = rela.info & 0xff;

         auto reloAddr = rela.offset - targetBaseAddr + targetVirtAddr;

         auto symbol = getSymbol(symSecView, index);
         auto &symbolSection = sections[symbol.shndx];
         auto symbolSectionName = shStrTab + symbolSection.header.name;

         auto symbolName = static_cast<const char*>(symStrTab.virtAddress) + symbol.name;

         uint32_t symAddr = getSymbolAddress(symbol, sections);
         symAddr += rela.addend;

         if (symbolSection.header.type == elf::SHT_RPL_IMPORTS) {
            symAddr = gMemory.read<uint32_t>(symAddr);

            if (symAddr == 0) {
               symAddr = registerUnimplementedData(symbolName);
            }
         }

         auto ptr8 = gMemory.translate(reloAddr);
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
         case elf::R_PPC_REL24: {
            auto ins = Instruction{ byte_swap(*ptr32) };
            auto data = gInstructionTable.decode(ins);

            // Our REL24 trampolines only work for a branch instruction...
            assert(data->id == InstructionID::b);

            intptr_t delta = symAddr - reloAddr;
            if (delta < -0x01fffffc || delta > 0x01fffffc) {
               void *trampPtr = getTramp(gMemory.translate(symAddr), symbolName);
               assert(trampPtr);
               uint32_t trampAddr = gMemory.untranslate(trampPtr);

               delta = trampAddr - reloAddr;
               if (delta < -0x01fffffc || delta > 0x01fffffc) {
                  // Trampoline is still too far...
                  assert(0);
               }
               symAddr = trampAddr;
            }

            *ptr32 = byte_swap((byte_swap(*ptr32) & ~0x03fffffc) | ((symAddr - reloAddr) & 0x03fffffc));
            break;
         }
         case elf::R_PPC_EMB_SDA21: {
            auto ins = Instruction{ byte_swap(*ptr32) };

            int32_t offset = 0;

            if (ins.rA == 0) {
               // 0
               offset = 0;
            } else if (ins.rA == 2) {
               // sda2Base
               offset = static_cast<int32_t>(symAddr) - static_cast<int32_t>(sda2BaseAddr);
            } else if (ins.rA == 13) {
               // sdaBase
               offset = static_cast<int32_t>(symAddr) - static_cast<int32_t>(sdaBaseAddr);
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

   // Relocate entry point
   uint32_t entryPoint = header.entry;
   for (auto &section : sections) {
      uint32_t virtAddr = gMemory.untranslate(section.virtAddress);

      if (section.header.addr <= entryPoint && section.header.addr + section.virtSize > entryPoint) {
         if (virtAddr >= section.header.addr) {
            entryPoint += virtAddr - section.header.addr;
         } else {
            entryPoint -= section.header.addr - virtAddr;
         }
         break;
      }
   }

   // Process exports
   std::map<std::string, void*> exports;

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_EXPORTS) {
         continue;
      }

      uint32_t *secData = static_cast<uint32_t*>(section.virtAddress);
      const char *secNames = static_cast<const char*>(section.virtAddress);
      uint32_t numExports = byte_swap(*secData++);
      uint32_t exportsCrc = byte_swap(*secData++);

      for (auto i = 0u; i < numExports; ++i) {
         uint32_t exportsAddr = byte_swap(*secData++);
         uint32_t exportNameOff = byte_swap(*secData++);
         const char * exportsName = secNames + exportNameOff;

         exports.emplace(exportsName, gMemory.translate(exportsAddr));
         symbolsMap.emplace(exportsName, gMemory.translate(exportsAddr));
      }

   }

   // Create sections list
   std::vector<LoadedSection> loadedSections;
   for (auto& section : sections) {
      if (section.header.flags & elf::SHF_ALLOC) {
         if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
            const char * sectionName = shStrTab + section.header.name;
            uint8_t * sectionAddr = static_cast<uint8_t*>(section.virtAddress);
            loadedSections.emplace_back(LoadedSection{ sectionName, sectionAddr, sectionAddr + section.virtSize });
         }
      }
   }

   void * trampSegEnd = codeSeg.getCurrentAddr();
   if (trampSegEnd > trampSegStart) {
      loadedSections.emplace_back(LoadedSection{ "loader_thunks", trampSegStart, trampSegEnd });
   }

   // Free the load segment
   mCodeHeap->free(loadSegAddr);

   LoadedModule *loadedMod = new LoadedModule();
   loadedMod->mName = name;
   loadedMod->mSdaBase = gMemory.untranslate(sdaBase);
   loadedMod->mSda2Base = gMemory.untranslate(sda2Base);
   loadedMod->mDefaultStackSize = info.stackSize;
   loadedMod->mEntryPoint = entryPoint;
   loadedMod->mExports = exports;
   loadedMod->mSymbols = symbolsMap;
   loadedMod->mSections = loadedSections;
   loadedMod->mHandle = OSAllocFromSystem<LoadedModuleHandleData>();
   loadedMod->mHandle->ptr = loadedMod;
   return loadedMod;
}
