#include "kernel_loader.h"
#include "common/align.h"
#include "common/bigendianview.h"
#include "common/decaf_assert.h"
#include "common/teenyheap.h"
#include "common/strutils.h"
#include "decaf_config.h"
#include "elf.h"
#include "filesystem/filesystem.h"
#include "kernel_filesystem.h"
#include "kernel_hle.h"
#include "kernel_hlemodule.h"
#include "kernel_hlefunction.h"
#include "kernel_memory.h"
#include "libcpu/mem.h"
#include "modules/coreinit/coreinit_memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include <atomic>
#include <map>
#include <unordered_map>

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
      decaf_check(align_up(ptrOut, alignment) == ptrOut);

      // Make sure we have room
      decaf_check(mPtr + size <= mEnd);

      mPtr += size;
      return ptrOut;
   }

protected:
   uint8_t *mStart;
   uint8_t *mEnd;
   uint8_t *mPtr;
};

namespace kernel
{

namespace loader
{

using TrampolineMap = std::map<ppcaddr_t, ppcaddr_t>;
using SectionList = std::vector<elf::XSection>;
using AddressRange = std::pair<ppcaddr_t, ppcaddr_t>;

static std::atomic<uint32_t>
sLoaderLock { 0 };

static std::map<std::string, LoadedModule*>
sLoadedModules;

static std::map<ppcaddr_t, std::string, std::greater<ppcaddr_t>>
sGlobalSymbolLookup;

static ppcaddr_t
sSyscallAddress = 0;

static TeenyHeap *
sLoaderHeap = nullptr;

static uint32_t
sLoaderHeapRefs = 0;

static uint32_t
sModuleIndex = 0u;

static void *
loaderAlloc(uint32_t size, uint32_t alignment)
{
   sLoaderHeapRefs++;

   if (!sLoaderHeap) {
      if (!mem::commit(mem::LoaderBase, mem::LoaderSize)) {
         decaf_abort("Failed to allocate loader temporary memory");
      }

      sLoaderHeap = new TeenyHeap(mem::translate(mem::LoaderBase), mem::LoaderSize);
   }

   return sLoaderHeap->alloc(size, alignment);
}

static void
loaderFree(void *mem)
{
   sLoaderHeapRefs--;

   if (sLoaderHeapRefs == 0) {
      delete sLoaderHeap;
      sLoaderHeap = nullptr;
      mem::uncommit(mem::LoaderBase, mem::LoaderSize);
   }
}

void
lockLoader()
{
   uint32_t expected = 0;
   auto core = 1 << cpu::this_core::id();

   while (!sLoaderLock.compare_exchange_weak(expected, core, std::memory_order_acquire)) {
      expected = 0;
   }
}

void
unlockLoader()
{
   auto core = 1 << cpu::this_core::id();
   auto oldCore = sLoaderLock.exchange(0, std::memory_order_release);
   decaf_check(oldCore == core);
}

std::map<std::string, LoadedModule*>
getLoadedModules()
{
   return sLoadedModules;
}

static LoadedModule *
loadRPLNoLock(const std::string& name);

static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod,
                SequentialMemoryTracker &codeSeg,
                TrampolineMap &trampolines,
                void *target,
                const std::string& symbolName);

static bool
readFileInfo(BigEndianView &in,
             const SectionList &sections,
             elf::FileInfo &info);

static const elf::XSection *
findSection(const SectionList &sections,
            const char *shStrTab,
            const std::string &name);

static elf::Symbol
getSymbol(BigEndianView &symSecView,
          uint32_t index);

static ppcaddr_t
calculateRelocatedAddress(ppcaddr_t address, const SectionList &sections);

// Find and read the SHT_RPL_FILEINFO section
static bool
readFileInfo(BigEndianView &in,
             const SectionList &sections,
             elf::FileInfo &info)
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
findSection(const SectionList &sections,
            const char *shStrTab,
            const std::string &name)
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
getSymbol(BigEndianView &symSecView,
          uint32_t index)
{
   elf::Symbol sym;
   symSecView.seek(index * sizeof(elf::Symbol));
   elf::readSymbol(symSecView, sym);
   return sym;
}


// Returns address of trampoline for target
static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod,
                SequentialMemoryTracker &codeSeg,
                TrampolineMap &trampolines,
                void *target,
                const std::string& symbolName)
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
      decaf_check(0);
   }

   loadedMod->symbols.emplace(symbolName + "#thunk", Symbol{ trampAddr, SymbolType::Function });
   trampolines.emplace(targetAddr, trampAddr);
   return trampAddr;
}


static ppcaddr_t
generateUnimplementedDataThunk(const std::string &module,
                               const std::string &name)
{
   static std::map<std::string, int> sUnimplementedData;
   auto itr = sUnimplementedData.find(name);

   if (itr != sUnimplementedData.end()) {
      return itr->second | 0x800;
   }

   auto id = gsl::narrow_cast<uint32_t>(sUnimplementedData.size());
   auto fakeAddr = 0xFFF00000 | (id << 12);
   decaf_check(id <= 0xFF);

   gLog->info("Unimplemented data symbol {}::{} at {:08x}", module, name, fakeAddr);
   sUnimplementedData.emplace(name, fakeAddr);
   return fakeAddr | 0x800;
}


static ppcaddr_t
generateUnimplementedFunctionThunk(const std::string &module,
                                   const std::string &name)
{
   static std::map<std::string, ppcaddr_t> sUnimplementedFunctions;
   auto itr = sUnimplementedFunctions.find(name);

   if (itr != sUnimplementedFunctions.end()) {
      return itr->second;
   }

   auto id = kernel::registerUnimplementedHleFunc(module, name);
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

   gLog->info("Unimplemented function {}::{} at {:08x}", module, name, addr);
   sUnimplementedFunctions.emplace(name, addr);
   return addr;
}


static void
processKernelTraceFilters(const std::string &module,
                          std::vector<kernel::HleFunction *> &functions)
{
   struct TraceFilter
   {
      bool enable = false;
      gsl::span<const char> name;
      bool wildcard = false;
   };

   // Create list of filters appropriate for this module
   std::vector<TraceFilter> filters;
   filters.reserve(decaf::config::log::kernel_trace_filters.size());

   for (auto &filterText : decaf::config::log::kernel_trace_filters) {
      TraceFilter filter;

      // First character is + enable or - disable
      if (filterText[0] == '+') {
         filter.enable = true;
      } else if (filterText[0] == '-') {
         filter.enable = false;
      } else {
         decaf_abort(fmt::format("Invalid kernel trace filter {}", filterText));
      }

      // Find module::name
      auto nameSeparator = filterText.find("::");
      auto functionName = gsl::span<const char>{};
      auto moduleName = gsl::span<const char>{};
      auto moduleWildcard = false;

      if (nameSeparator != std::string::npos) {
         moduleName = gsl::as_span(filterText.c_str() + 1, nameSeparator - 1);
         functionName = gsl::as_span(filterText.c_str() + nameSeparator + 2, filterText.size() - nameSeparator - 2);
      } else {
         moduleName = gsl::as_span(filterText.c_str() + 1, filterText.size() - 1);
         filter.wildcard = true;
      }

      if (!moduleName.empty() && *(moduleName.end() - 1) == '*') {
         moduleWildcard = true;
         moduleName = moduleName.subspan(0, moduleName.size() - 1);
      }

      if (moduleWildcard) {
         // Check this module matches the module filter partially
         if (!moduleName.empty() && module.compare(0, moduleName.size(), moduleName.data(), moduleName.size()) != 0) {
            continue;
         }
      } else {
         // Check this module matches the module filter exactly
         if (module.compare(0, std::string::npos, moduleName.data(), moduleName.size()) != 0) {
            continue;
         }
      }

      if (!functionName.empty() && *(functionName.end() - 1) == '*') {
         filter.wildcard = true;
         functionName = functionName.subspan(0, functionName.size() - 1);
      }

      filter.name = functionName;
      filters.emplace_back(filter);
   }

   // Now check filters against all functions
   for (auto &func : functions) {
      for (auto &filter : filters) {
         if (filter.wildcard) {
            // Check this function name matches the filter partially
            if (!filter.name.empty() && func->name.compare(0, filter.name.size(), filter.name.data(), filter.name.size()) != 0) {
               continue;
            }
         } else {
            // Check this function name matches the filter exactly
            if (func->name.compare(0, std::string::npos, filter.name.data(), filter.name.size()) != 0) {
               continue;
            }
         }

         func->traceEnabled = filter.enable;
      }
   }
}


/**
 * Load a kernel module into virtual memory space by creating thunks
 */
static LoadedModule *
loadHleModule(const std::string &moduleName,
              const std::string &name,
              kernel::HleModule *module)
{
   std::vector<kernel::HleFunction *> funcSymbols;
   std::vector<kernel::HleData *> dataSymbols;
   uint32_t dataSize = 0, codeSize = 0;
   auto &symbols = module->getSymbols();

   // Get all function and data symbols exported from module
   for (auto &symbol : symbols) {
      if (symbol->type == kernel::HleSymbol::Function) {
         auto funcSymbol = static_cast<kernel::HleFunction *>(symbol);
         codeSize += 8;
         funcSymbols.emplace_back(funcSymbol);
      } else if (symbol->type == kernel::HleSymbol::Data) {
         auto dataSymbol = static_cast<kernel::HleData *>(symbol);
         dataSize += dataSymbol->size;
         dataSymbols.emplace_back(dataSymbol);
      } else {
         gLog->error("Unexpected HleSymbol type");
         return nullptr;
      }
   }

   // Process kernel_trace_filters
   processKernelTraceFilters(moduleName, funcSymbols);

   // Create module
   auto loadedMod = new LoadedModule {};
   sLoadedModules.emplace(moduleName, loadedMod);
   loadedMod->name = name;
   loadedMod->handle = coreinit::internal::sysAlloc<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod;

   // Load code section
   if (codeSize > 0) {
      auto codeRegion = static_cast<uint8_t*>(coreinit::internal::sysAlloc(codeSize, 4));
      auto start = mem::untranslate(codeRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection { ".text", LoadedSectionType::Code, start, end });

      for (auto &func : funcSymbols) {
         // Allocate some space for the thunk
         auto thunk = reinterpret_cast<uint32_t *>(codeRegion);
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
         loadedMod->symbols.emplace(func->name, Symbol{ addr, SymbolType::Function });

         // Save the PPC ptr for internal lookups
         func->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         if (func->hostPtr) {
            *reinterpret_cast<ppcaddr_t *>(func->hostPtr) = addr;
         }
      }
   }

   // Load data section
   if (dataSize > 0) {
      auto dataRegion = static_cast<uint8_t *>(coreinit::internal::sysAlloc(dataSize, 4));
      auto start = mem::untranslate(dataRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection { ".data", LoadedSectionType::Data, start, end });

      for (auto &data : dataSymbols) {
         // Allocate the same for this export
         auto thunk = dataRegion;
         auto addr = mem::untranslate(thunk);
         dataRegion += data->size;

         // Add to exports list
         loadedMod->symbols.emplace(data->name, Symbol{ addr, SymbolType::Data });

         // Save the PPC ptr for internal lookups
         data->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         if (data->hostPtr) {
            for (uint32_t off = 0, i = 0; off < data->size; off += data->itemSize, i++) {
               reinterpret_cast<void **>(data->hostPtr)[i] = thunk + off;
            }
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

static bool
processRelocations(LoadedModule *loadedMod,
                   const SectionList &sections,
                   BigEndianView &in,
                   const char *shStrTab,
                   SequentialMemoryTracker &codeSeg,
                   AddressRange &trampSeg)
{
   auto trampolines = TrampolineMap{};
   auto buffer = std::vector<uint8_t>{};
   trampSeg.first = codeSeg.getCurrentAddr();

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      elf::readSectionData(in, section.header, buffer);
      auto secIn = BigEndianView{ gsl::as_span(buffer) };

      auto &symSec = sections[section.header.link];
      auto &targetSec = sections[section.header.info];
      auto symSecView = BigEndianView{ symSec.memory, symSec.virtSize };
      auto &symStrTab = sections[symSec.header.link];

      auto targetBaseAddr = targetSec.header.addr;
      auto targetVirtAddr = targetSec.virtAddress;

      while (!secIn.eof()) {
         elf::Rela rela;
         elf::readRelocationAddend(secIn, rela);

         auto index = rela.info >> 8;
         auto type = rela.info & 0xff;
         auto reloAddr = rela.offset - targetBaseAddr + targetVirtAddr;

         auto symbol = getSymbol(symSecView, index);
         auto symbolName = reinterpret_cast<const char*>(symStrTab.memory) + symbol.name;
         const elf::XSection *symbolSection = nullptr;

         if (symbol.shndx == elf::SHN_UNDEF) {
            continue;
         } else if (symbol.shndx < elf::SHN_LORESERVE) {
            symbolSection = &sections[symbol.shndx];
         } else {
            // ABS is the only supported special section index
            decaf_check(symbol.shndx == elf::SHN_ABS);
         }

         // Get symbol address
         auto symAddr = symbol.value + rela.addend;

         // Calculate relocated symbol address except for TLS which are NOT rpl imports
         if (symbolSection) {
            if (symbolSection->header.type == elf::SHT_RPL_IMPORTS ||
               (type != elf::R_PPC_DTPREL32 && type != elf::R_PPC_DTPMOD32)) {
               symAddr = calculateRelocatedAddress(symbol.value, sections);

               if (symbolSection->header.type == elf::SHT_RPL_IMPORTS) {
                  decaf_check(symAddr);
                  symAddr = mem::read<uint32_t>(symAddr);
               }

               if (type != elf::R_PPC_DTPREL32 && type != elf::R_PPC_DTPMOD32) {
                  decaf_check(symAddr);
               }

               symAddr += rela.addend;
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
            decaf_check(data->id == espresso::InstructionID::b);

            auto delta = static_cast<ptrdiff_t>(symAddr) - static_cast<ptrdiff_t>(reloAddr);

            if (delta < -0x01FFFFFC || delta > 0x01FFFFFC) {
               auto trampAddr = getTrampAddress(loadedMod, codeSeg, trampolines, mem::translate(symAddr), symbolName);
               decaf_check(trampAddr);

               // Ensure valid trampoline delta
               delta = static_cast<ptrdiff_t>(trampAddr) - static_cast<ptrdiff_t>(reloAddr);
               decaf_check(delta >= -0x01FFFFFC && delta <= 0x01FFFFFC);

               symAddr = trampAddr;
            }

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
               decaf_check(0);
            }

            if (offset < std::numeric_limits<int16_t>::min() || offset > std::numeric_limits<int16_t>::max()) {
               gLog->error("Expected SDA relocation {:x} to be within signed 16 bit offset of base {}", symAddr, ins.rA);
               break;
            }

            ins.simm = offset;
            *ptr32 = byte_swap(ins.value);
            break;
         }
         case elf::R_PPC_DTPREL32:
         {
            *ptr32 = byte_swap(symAddr);
            break;
         }
         case elf::R_PPC_DTPMOD32:
         {
            decaf_check(symbolSection);
            auto moduleIndex = loadedMod->tlsModuleIndex;

            // If this is an import, we must find the correct module index
            if (symbolSection->header.type == elf::SHT_RPL_IMPORTS) {
               auto module = loadRPLNoLock(symbolSection->name);
               moduleIndex = module->tlsModuleIndex;
            }

            *ptr32 = byte_swap(moduleIndex);
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
processImports(LoadedModule *loadedMod,
               SectionList &sections)
{
   std::map<std::string, ppcaddr_t> symbolTable;

   // Process import sections
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_IMPORTS) {
         continue;
      }

      // Load library
      auto libraryName = reinterpret_cast<const char *>(section.memory + 8);
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
            auto symbolAddr = 0u;

            if (symbolTargetIter == symbolTable.end()) {
               if (type == elf::STT_FUNC) {
                  symbolAddr = generateUnimplementedFunctionThunk(impsec.name, name);
               } else if (type == elf::STT_OBJECT) {
                  symbolAddr = generateUnimplementedDataThunk(impsec.name, name);
               } else {
                  decaf_abort("Unexpected import symbol type");
               }
            } else {
               decaf_check(type == elf::STT_FUNC || type == elf::STT_OBJECT || type == elf::STT_TLS);
               symbolAddr = symbolTargetIter->second;
            }

            // Write the symbol address into .fimport or .dimport
            decaf_check(type == elf::STT_TLS || symbolAddr);
            mem::write(virtAddress, symbolAddr);
         }
      }
   }

   return true;
}

bool
processSymbols(LoadedModule *loadedMod,
               SectionList &sections)
{
   // Process symbols
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

         if (sym.shndx == elf::SHN_ABS && sym.value == 0) {
            // This is an no-value absolute symbol, probably for a FILE object.
            continue;
         }

         if (sym.shndx == elf::SHN_HIRESERVE && sym.value == 0) {
            // Not actually sure what this is, but it doens't point to anything.
            continue;
         }

         if (sym.shndx >= elf::SHN_LORESERVE) {
            gLog->warn("Unexpected symbol definition: shndx {:04x}, info {:02x}, size {:08x}, other {:02x}, value {:08x}, name {}",
               sym.shndx, sym.info, sym.size, sym.other, sym.value, name);
            continue;
         }

         // Calculate relocated address
         auto &symsec = sections[sym.shndx];
         auto offset = sym.value - symsec.header.addr;

         if (!symsec.virtSize) {
            // Ignore symbols in unused sections...
            continue;
         }

         auto symType = SymbolType::Unknown;

         if (type == elf::STT_OBJECT) {
            symType = SymbolType::Data;
         } else if (type == elf::STT_FUNC) {
            symType = SymbolType::Function;
         }

         auto baseAddress = symsec.virtAddress;
         auto virtAddress = baseAddress + offset;

         if (type == elf::STT_TLS) {
            // TLS symbols should not be relocated, use the original sym.value
            symType = SymbolType::TLS;
            virtAddress = sym.value;
         }

         loadedMod->symbols.emplace(name, Symbol { virtAddress, symType });
      }
   }

   return true;
}

static ppcaddr_t
calculateRelocatedAddress(ppcaddr_t address, const SectionList &sections)
{
   for (auto &section : sections) {
      if (section.header.addr <= address && section.header.addr + section.virtSize > address) {
         if (section.virtAddress >= section.header.addr) {
            address += section.virtAddress - section.header.addr;
         } else {
            address -= section.header.addr - section.virtAddress;
         }

         return address;
      }
   }

   decaf_abort("Cannot relocate addresses which don't exist in any section");
}

static bool
processExports(LoadedModule *loadedMod,
               const SectionList &sections)
{
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_EXPORTS) {
         continue;
      }

      auto secData = reinterpret_cast<uint32_t *>(section.memory);
      auto strTab = reinterpret_cast<const char *>(section.memory);
      auto numExports = byte_swap(*secData++);
      auto exportsCrc = byte_swap(*secData++);

      for (auto i = 0u; i < numExports; ++i) {
         auto addr = byte_swap(*secData++);
         auto nameOffset = byte_swap(*secData++);

         if (nameOffset & 0x80000000) {
            // TLS exports have the high bit set
            nameOffset = nameOffset & ~0x80000000;
         } else {
            addr = calculateRelocatedAddress(addr, sections);
         }

         loadedMod->exports.emplace(strTab + nameOffset, addr);
      }
   }

   return true;
}


LoadedModule *
loadRPL(const std::string &moduleName,
        const std::string &name,
        const gsl::span<uint8_t> &data)
{
   std::vector<uint8_t> buffer;
   auto in = BigEndianView{ data };
   auto loadedMod = new LoadedModule();
   loadedMod->name = name;
   sLoadedModules.emplace(moduleName, loadedMod);

   gLog->debug("Loading module {}", moduleName);

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

   // Allocate all our memory chunks which will be used
   codeSegAddr = getCodeHeap()->alloc(info.textSize, info.textAlign);

   if (coreinit::internal::dynLoadMemAlloc(info.dataSize, info.dataAlign, &dataSegAddr) != 0) {
      dataSegAddr = nullptr;
   }

   loadSegAddr = loaderAlloc(info.loadSize, info.loadAlign);

   decaf_check(codeSegAddr);
   decaf_check(dataSegAddr);
   decaf_check(loadSegAddr);

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

   for (auto &section : sections) {
      auto sectionName = shStrTab + section.header.name;
      gLog->debug("  found section: type {:08x}, flags {:08x}, info {:08x}, addr {:08x}, size {:>8x}, name {}",
         section.header.type,
         section.header.flags,
         section.header.info,
         section.header.addr,
         section.header.size,
         sectionName);
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

   // Calculate TLS base
   auto thrdata = findSection(sections, shStrTab, ".thrdata");

   if (thrdata) {
      loadedMod->tlsBase = thrdata->virtAddress;

      // Calculate TLS size
      auto start = thrdata->virtAddress;
      auto end = thrdata->virtAddress + thrdata->virtSize;

      for (auto &section : sections) {
         if (section.header.flags & elf::SHF_TLS) {
            decaf_check(section.virtAddress >= start);
            end = std::max(end, section.virtAddress + section.virtSize);
         }
      }

      loadedMod->tlsSize = end - start;
   }

   // Apparently info.tlsModuleIndex is always zero... so let's create our own!
   decaf_check(info.tlsModuleIndex == 0);
   loadedMod->tlsModuleIndex = sModuleIndex++;
   loadedMod->tlsAlignShift = info.tlsAlignShift;

   // Process exports
   if (!processExports(loadedMod, sections)) {
      gLog->error("Error loading exports");
      return nullptr;
   }

   // Process imports
   if (!processImports(loadedMod, sections)) {
      gLog->error("Error loading imports");
      return nullptr;
   }

   // Process symbols
   if (!processSymbols(loadedMod, sections)) {
      gLog->error("Error loading symbols");
      return nullptr;
   }

   // Process relocations
   auto trampSeg = AddressRange{};

   if (!processRelocations(loadedMod, sections, in, shStrTab, codeSeg, trampSeg)) {
      gLog->error("Error loading relocations");
      return nullptr;
   }

   // Process dot syscall
   for (auto &section : sections) {
      auto sectionName = shStrTab + section.header.name;

      if (strcmp(sectionName, ".syscall") == 0) {
         decaf_check(sSyscallAddress);

         // Write a branch to the syscall location
         auto b = espresso::encodeInstruction(espresso::InstructionID::b);
         b.li = sSyscallAddress >> 2;
         b.lk = 0;
         b.aa = 1;
         mem::write(section.virtAddress, b.value);
      }
   }

   // Relocate entry point
   auto entryPoint = calculateRelocatedAddress(header.entry, sections);

   // Create sections list
   for (auto &section : sections) {
      if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
         auto sectionName = shStrTab + section.header.name;
         auto start = section.virtAddress;
         auto end = section.virtAddress + section.virtSize;
         auto type = LoadedSectionType::Data;

         if (section.header.flags & elf::SHF_EXECINSTR) {
            type = LoadedSectionType::Code;
         }

         loadedMod->sections.emplace_back(LoadedSection { sectionName, type, start, end });
      }
   }

   if (trampSeg.second > trampSeg.first) {
      loadedMod->sections.emplace_back(LoadedSection { "loader_thunks", LoadedSectionType::Code, trampSeg.first, trampSeg.second });
   }

   // Add the modules entry point as an symbol called 'start'
   loadedMod->symbols.emplace("__start", Symbol{ entryPoint, SymbolType::Function });

   // Free the load segment
   loaderFree(loadSegAddr);

   // Add all the modules symbols to the Global Symbol Map
   for (auto &i : loadedMod->symbols) {
      // TODO: Modify symbol lookup functions to support picking the type of
      //  symbol you want so that we can have our global symbol lookup table
      //  include information about which symbol, and relatedly, display data
      //  symbol names in the debugger.
      if (i.second.type == SymbolType::Function) {
         sGlobalSymbolLookup.emplace(i.second.address, fmt::format("{}:{}", loadedMod->name, i.first));
      }
   }

   loadedMod->defaultStackSize = info.stackSize;
   loadedMod->entryPoint = entryPoint;
   loadedMod->handle = coreinit::internal::sysAlloc<LoadedModuleHandleData>();
   loadedMod->handle->ptr = loadedMod;

   return loadedMod;
}

static void
normalizeModuleName(const std::string &name,
      std::string &moduleName,
      std::string &fileName)
{
   if (!ends_with(name, ".rpl") && !ends_with(name, ".rpx")) {
      moduleName = name;
      fileName = name + ".rpl";
   } else {
      moduleName = name.substr(0, name.size() - 4);
      fileName = name;
   }
}

LoadedModule *
loadRPLNoLock(const std::string &name)
{
   LoadedModule *module = nullptr;
   std::string moduleName;
   std::string fileName;

   normalizeModuleName(name, moduleName, fileName);

   // Check if we already have this module loaded
   auto itr = sLoadedModules.find(moduleName);

   if (itr != sLoadedModules.end()) {
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
      sLoadedModules.erase(moduleName);
      return nullptr;
   } else {
      gLog->info("Loaded module {}", fileName);
      return module;
   }
}

LoadedModule *
loadRPL(const std::string& name)
{
   // Use the scheduler lock to protect access to the loaders memory
   lockLoader();
   auto res = loadRPLNoLock(name);
   unlockLoader();
   return res;
}

void
setSyscallAddress(ppcaddr_t address)
{
   sSyscallAddress = address;
}

LoadedModule *
findModule(const std::string& name)
{
   std::string moduleName;
   std::string fileName;
   normalizeModuleName(name, moduleName, fileName);

   // Check if we already have this module loaded
   auto itr = sLoadedModules.find(moduleName);

   if (itr == sLoadedModules.end()) {
      return nullptr;
   }

   return itr->second;
}

LoadedSection *
findSectionForAddress(ppcaddr_t address)
{
   for (auto &mod : sLoadedModules) {
      for (auto &sec : mod.second->sections) {
         if (address >= sec.start && address < sec.end) {
            return &sec;
         }
      }
   }
   return nullptr;
}

std::string *
findSymbolNameForAddress(ppcaddr_t address)
{
   auto symIter = sGlobalSymbolLookup.find(address);

   if (symIter == sGlobalSymbolLookup.end()) {
      return nullptr;
   }

   return &symIter->second;
}

std::string
findNearestSymbolNameForAddress(ppcaddr_t address)
{
   auto symIter = sGlobalSymbolLookup.lower_bound(address);

   if (symIter == sGlobalSymbolLookup.end()) {
      return "?";
   }

   auto delta = address - symIter->first;

   if (delta == 0) {
      return symIter->second;
   } else {
      return fmt::format("{} + 0x{:x}", symIter->second, delta);
   }
}

} // namespace loader

} // namespace kernel
