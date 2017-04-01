#include "kernel_loader.h"
#include "decaf_config.h"
#include "elf.h"
#include "filesystem/filesystem.h"
#include "kernel_filesystem.h"
#include "kernel_hle.h"
#include "kernel_hlemodule.h"
#include "kernel_hlefunction.h"
#include "kernel_memory.h"
#include "modules/coreinit/coreinit_internal_idlock.h"
#include "modules/coreinit/coreinit_memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_scheduler.h"

#include <atomic>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <common/frameallocator.h>
#include <common/teenyheap.h>
#include <common/strutils.h>
#include <gsl.h>
#include <libcpu/cpu.h>
#include <libcpu/mem.h>
#include <map>
#include <unordered_map>
#include <zlib.h>

const unsigned elf::Header::Magic;

namespace kernel
{

namespace loader
{

using TrampolineMap = std::map<ppcaddr_t, ppcaddr_t>;
using SectionList = std::vector<elf::Section>;
using AddressRange = std::pair<ppcaddr_t, ppcaddr_t>;

static coreinit::internal::IdLock
sLoaderLock;

static std::map<std::string, LoadedModule *>
sLoadedModules;

static std::map<ppcaddr_t, std::string, std::greater<ppcaddr_t>>
sGlobalSymbolLookup;

static ppcaddr_t
sSyscallAddress = 0;

static uint32_t
sModuleIndex = 0u;

static TeenyHeap *
sLoaderHeap = nullptr;

static uint32_t
sLoaderHeapRefs = 0;

static void *
loaderAlloc(uint32_t size,
            uint32_t alignment)
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

static LoadedModule *
loadRPLNoLock(const std::string& name);

// Read and decompress section data
static bool
readSectionData(const gsl::span<uint8_t> &file,
                const elf::SectionHeader &header,
                std::vector<uint8_t> &data)
{
   if (header.type == elf::SHT_NOBITS || header.size == 0) {
      data.clear();
      return true;
   }

   if (header.flags & elf::SHF_DEFLATED) {
      auto stream = z_stream {};
      auto ret = Z_OK;

      // Read the deflated header
      auto deflatedHeader = reinterpret_cast<elf::DeflatedHeader *>(file.data() + header.offset);
      auto deflatedData = file.data() + header.offset + sizeof(elf::DeflatedHeader);
      data.resize(deflatedHeader->inflatedSize);

      // Inflate
      memset(&stream, 0, sizeof(stream));
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;

      ret = inflateInit(&stream);

      if (ret != Z_OK) {
         gLog->error("Couldn't decompress .rpx section because inflateInit returned {}", ret);
         data.clear();
      } else {
         stream.avail_in = header.size;
         stream.next_in = const_cast<Bytef *>(deflatedData);
         stream.avail_out = static_cast<uInt>(data.size());
         stream.next_out = reinterpret_cast<Bytef *>(data.data());

         ret = inflate(&stream, Z_FINISH);

         if (ret != Z_OK && ret != Z_STREAM_END) {
            gLog->error("Couldn't decompress .rpx section because inflate returned {}", ret);
            data.clear();
         }

         inflateEnd(&stream);
      }
   } else {
      data.resize(header.size);
      std::memcpy(data.data(), file.data() + header.offset, header.size);
   }

   return data.size() > 0;
}

// Find and read the SHT_RPL_FILEINFO section
static bool
readFileInfo(const gsl::span<uint8_t> &file,
             const SectionList &sections,
             elf::FileInfo &info)
{
   std::vector<uint8_t> data;

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_FILEINFO) {
         continue;
      }

      readSectionData(file, section.header, data);

      info = *reinterpret_cast<elf::FileInfo *>(data.data());
      return true;
   }

   gLog->error("Failed to find RPLFileInfo section");
   return false;
}


// Find a section with a matching name
static const elf::Section *
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


// Calculate relocated address
static ppcaddr_t
calculateRelocatedAddress(ppcaddr_t address,
                          const SectionList &sections)
{
   bool foundSectionEnd = true;
   ppcaddr_t sectionEndAddress;

   for (auto &section : sections) {
      if (section.header.addr <= address && section.header.addr + section.virtSize > address) {
         if (section.virtAddress >= section.header.addr) {
            address += section.virtAddress - section.header.addr;
         } else {
            address -= section.header.addr - section.virtAddress;
         }

         return address;

      } else if (section.header.addr + section.virtSize == address) {

         foundSectionEnd = true;
         if (section.virtAddress >= section.header.addr) {
            sectionEndAddress = address + section.virtAddress - section.header.addr;
         } else {
            sectionEndAddress = address - section.header.addr - section.virtAddress;
         }

      }
   }

   if (foundSectionEnd) {
      return sectionEndAddress;
   }

   decaf_abort("Cannot relocate addresses which don't exist in any section");
}


// Returns address of trampoline for target
static ppcaddr_t
getTrampAddress(LoadedModule *loadedMod,
                FrameAllocator &codeSeg,
                TrampolineMap &trampolines,
                void *target,
                const std::string& symbolName)
{
   auto targetAddr = mem::untranslate(target);
   auto trampIter = trampolines.find(targetAddr);

   if (trampIter != trampolines.end()) {
      return trampIter->second;
   }

   auto tramp = reinterpret_cast<uint32_t *>(codeSeg.top());
   auto trampAddr = mem::untranslate(tramp);
   auto delta = static_cast<ptrdiff_t>(targetAddr) - static_cast<ptrdiff_t>(trampAddr);

   if (delta > -0x1FFFFFCll && delta < 0x1FFFFFCll) {
      tramp = static_cast<uint32_t *>(codeSeg.allocate(4));

      // Short jump using b
      auto b = espresso::encodeInstruction(espresso::InstructionID::b);
      b.li = delta >> 2;
      b.lk = 0;
      b.aa = 0;
      *tramp = byte_swap(b.value);
   } else if (targetAddr < 0x03fffffc) {
      tramp = static_cast<uint32_t *>(codeSeg.allocate(4));

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

   loadedMod->symbols.emplace(symbolName + "#thunk", Symbol { trampAddr, SymbolType::Function });
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
         moduleName = gsl::make_span(filterText.c_str() + 1, nameSeparator - 1);
         functionName = gsl::make_span(filterText.c_str() + nameSeparator + 2, filterText.size() - nameSeparator - 2);
      } else {
         moduleName = gsl::make_span(filterText.c_str() + 1, filterText.size() - 1);
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
            if (!filter.name.empty()
             && func->name.compare(0, filter.name.size(), filter.name.data(), filter.name.size()) != 0) {
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
   auto &symbolMap = module->getSymbolMap();

   // Get all function and data symbols
   for (auto &pair : symbolMap) {
      auto symbol = pair.second;

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
      auto codeRegion = static_cast<uint8_t *>(coreinit::internal::sysAlloc(codeSize));
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
         loadedMod->symbols.emplace(func->name, Symbol { addr, SymbolType::Function });

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
      auto dataRegion = static_cast<uint8_t *>(coreinit::internal::sysAlloc(dataSize));
      auto start = mem::untranslate(dataRegion);
      auto end = start + codeSize;
      loadedMod->sections.emplace_back(LoadedSection { ".data", LoadedSectionType::Data, start, end });

      for (auto &data : dataSymbols) {
         // Allocate some space for the data
         auto thunk = dataRegion;
         auto addr = mem::untranslate(thunk);
         dataRegion += data->size;

         // Add to symbol list
         loadedMod->symbols.emplace(data->name, Symbol { addr, SymbolType::Data });

         // Save the PPC ptr for internal lookups
         data->ppcPtr = thunk;

         // Map host memory pointer to PPC region
         if (data->hostPtr) {
            for (auto off = 0u, i = 0u; off < data->size; off += data->itemSize, i++) {
               reinterpret_cast<void **>(data->hostPtr)[i] = thunk + off;
            }
         }
      }
   }

   // Export everything which is an export
   for (auto &pair : symbolMap) {
      auto symbol = pair.second;

      if (symbol->exported) {
         loadedMod->exports.emplace(symbol->name, mem::untranslate(symbol->ppcPtr));
      }
   }

   module->initialise();
   return loadedMod;
}

static bool
processRelocations(LoadedModule *loadedMod,
                   const SectionList &sections,
                   const gsl::span<uint8_t> &file,
                   const char *shStrTab,
                   FrameAllocator &codeSeg,
                   AddressRange &trampSeg)
{
   auto trampolines = TrampolineMap{};
   auto buffer = std::vector<uint8_t> {};
   trampSeg.first = mem::untranslate(codeSeg.top());

   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      readSectionData(file, section.header, buffer);

      auto &symSec = sections[section.header.link];
      auto &targetSec = sections[section.header.info];
      auto &symStrTab = sections[symSec.header.link];

      auto targetBaseAddr = targetSec.header.addr;
      auto targetVirtAddr = targetSec.virtAddress;

      auto symbols = gsl::make_span(reinterpret_cast<elf::Symbol *>(symSec.memory), symSec.virtSize / sizeof(elf::Symbol));
      auto relocations = gsl::make_span(reinterpret_cast<elf::Rela *>(buffer.data()), buffer.size() / sizeof(elf::Rela));

      for (auto &rela : relocations) {
         auto index = rela.info >> 8;
         auto type = rela.info & 0xff;
         auto reloAddr = rela.offset - targetBaseAddr + targetVirtAddr;

         auto &symbol = symbols[index];
         auto symbolName = reinterpret_cast<const char*>(symStrTab.memory) + symbol.name;
         const elf::Section *symbolSection = nullptr;

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

   trampSeg.second = mem::untranslate(codeSeg.top());
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

      auto stringTable = reinterpret_cast<const char*>(sections[section.header.link].memory);
      auto symbols = gsl::make_span(reinterpret_cast<elf::Symbol *>(section.memory), section.virtSize / sizeof(elf::Symbol));

      for (auto &symbol : symbols) {
         auto name = stringTable + symbol.name;
         auto binding = symbol.info >> 4;
         auto type = symbol.info & 0xf;

         if (binding != elf::STB_GLOBAL) {
            // No need to scan LOCAL symbols for imports
            //  In fact, this crashes due to NULL symbols
            continue;
         }

         if (symbol.shndx >= elf::SHN_LORESERVE) {
            gLog->warn("Symbol {} in invalid section 0x{:X}", name, symbol.shndx);
            continue;
         }

         auto &importSection = sections[symbol.shndx];

         if (importSection.header.type != elf::SHT_RPL_IMPORTS) {
            continue;
         }

         // Find the symbol address
         auto symbolTableItr = symbolTable.find(name);
         auto symbolAddress = 0u;

         if (symbolTableItr == symbolTable.end()) {
            if (type == elf::STT_FUNC) {
               symbolAddress = generateUnimplementedFunctionThunk(importSection.name, name);
            } else if (type == elf::STT_OBJECT) {
               symbolAddress = generateUnimplementedDataThunk(importSection.name, name);
            } else {
               decaf_abort("Unexpected import symbol type");
            }
         } else {
            decaf_check(type == elf::STT_FUNC || type == elf::STT_OBJECT || type == elf::STT_TLS);
            symbolAddress = symbolTableItr->second;
         }

         decaf_check(type == elf::STT_TLS || symbolAddress);

         // Write the symbol address into the .fimport or .dimport section
         auto offset = symbol.value - importSection.header.addr;
         auto importAddress = importSection.virtAddress + offset;
         mem::write(importAddress, symbolAddress);
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

      auto stringTable = reinterpret_cast<const char*>(sections[section.header.link].memory);
      auto symbols = gsl::make_span(reinterpret_cast<elf::Symbol *>(section.memory), section.virtSize / sizeof(elf::Symbol));

      for (auto &symbol : symbols) {
         // Create symbol data
         auto name = stringTable + symbol.name;
         auto binding = symbol.info >> 4;
         auto type = symbol.info & 0xf;

         if (symbol.shndx == elf::SHN_ABS && symbol.value == 0) {
            // This is an no-value absolute symbol, probably for a FILE object.
            continue;
         }

         if (symbol.shndx == elf::SHN_HIRESERVE && symbol.value == 0) {
            // Not actually sure what this is, but it doens't point to anything.
            continue;
         }

         if (symbol.shndx >= elf::SHN_LORESERVE) {
            gLog->warn("Unexpected symbol definition: shndx {:04x}, info {:02x}, size {:08x}, other {:02x}, value {:08x}, name {}",
                       symbol.shndx, symbol.info, symbol.size, symbol.other, symbol.value, name);
            continue;
         }

         // Calculate relocated address
         auto &parentSection = sections[symbol.shndx];

         if (!parentSection.virtSize) {
            // Ignore symbols in unused sections...
            continue;
         }

         auto symbolAddress = symbol.value;

         if (type != elf::STT_TLS) {
            // Relocate non-TLS symbols
            auto offset = symbol.value - parentSection.header.addr;
            symbolAddress = parentSection.virtAddress + offset;
         }

         auto symbolType = SymbolType::Unknown;

         if (type == elf::STT_OBJECT) {
            symbolType = SymbolType::Data;
         } else if (type == elf::STT_FUNC) {
            symbolType = SymbolType::Function;
         } else if (type == elf::STT_TLS) {
            symbolType = SymbolType::TLS;
         }

         loadedMod->symbols.emplace(name, Symbol { symbolAddress, symbolType });
      }
   }

   return true;
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
   auto loadedMod = new LoadedModule();
   loadedMod->name = name;
   sLoadedModules.emplace(moduleName, loadedMod);

   gLog->debug("Loading module {}", moduleName);

   // Read header
   auto header = reinterpret_cast<elf::Header *>(data.data());

   if (header->magic != elf::Header::Magic) {
      gLog->error("Unexpected elf magic, found {:08x} expected {:08x}", header->magic, elf::Header::Magic);
      return nullptr;
   }

   if (header->fileClass != elf::ELFCLASS32) {
      gLog->error("Unexpected elf file class, found {:02x} expected {:02x}", header->fileClass, elf::ELFCLASS32);
      return nullptr;
   }

   if (header->encoding != elf::ELFDATA2MSB) {
      gLog->error("Unexpected elf encoding, found {:02x} expected {:02x}", header->encoding, elf::ELFDATA2MSB);
      return nullptr;
   }

   if (header->machine != elf::EM_PPC) {
      gLog->error("Unexpected elf machine, found {:04x} expected {:04x}", header->machine, elf::EM_PPC);
      return nullptr;
   }

   if (header->elfVersion != elf::EV_CURRENT) {
      gLog->error("Unexpected elf version, found {:02x} expected {:02x}", header->elfVersion, elf::EV_CURRENT);
      return nullptr;
   }

   if (header->ehsize != sizeof(elf::Header)) {
      gLog->error("Unexpected elf ehsize, found 0x{:X} expected 0x{:X}", header->ehsize, sizeof(elf::Header));
      return nullptr;
   }

   if (header->shentsize != sizeof(elf::SectionHeader)) {
      gLog->error("Unexpected elf shentsize, found 0x{:X} expected 0x{:X}", header->shentsize, sizeof(elf::SectionHeader));
      return nullptr;
   }

   if (header->abi != elf::EABI_CAFE) {
      gLog->error("Unexpected elf abi found {:02x} expected {:02x}", header->abi, elf::EABI_CAFE);
      return nullptr;
   }

   // Read sections
   auto sectionHeaders = reinterpret_cast<elf::SectionHeader *>(data.data() + header->shoff);
   auto sections = std::vector<elf::Section> { header->shnum };

   for (auto i = 0u; i < sections.size(); ++i) {
      sections[i].header = sectionHeaders[i];
   }

   // Read RPL file info
   elf::FileInfo info;
   readFileInfo(data, sections, info);

   // Allocate all our memory chunks which will be used
   auto codeSegment = getCodeHeap()->alloc(info.textSize, info.textAlign);
   auto loadSegment = loaderAlloc(info.loadSize, info.loadAlign);
   void *dataSegment = nullptr;

   if (coreinit::internal::dynLoadMemAlloc(info.dataSize, info.dataAlign, &dataSegment) != 0) {
      dataSegment = nullptr;
   }

   decaf_check(codeSegment);
   decaf_check(loadSegment);
   decaf_check(loadSegment);

   auto codeAllocator = FrameAllocator { codeSegment, info.textSize };
   auto dataAllocator = FrameAllocator { dataSegment, info.dataSize };
   auto loadAllocator = FrameAllocator { loadSegment, info.loadSize };

   // Allocate sections from our memory segments
   for (auto &section : sections) {
      if (section.header.flags & elf::SHF_ALLOC) {
         void *allocData = nullptr;

         // Read the section data
         if (section.header.type == elf::SHT_NOBITS) {
            buffer.clear();
            buffer.resize(section.header.size, 0);
         } else {
            if (!readSectionData(data, section.header, buffer)) {
               gLog->error("Failed to read section data");
               return nullptr;
            }
         }

         // Allocate from correct memory segment
         if (section.header.type == elf::SHT_PROGBITS || section.header.type == elf::SHT_NOBITS) {
            if (section.header.flags & elf::SHF_EXECINSTR) {
               allocData = codeAllocator.allocate(buffer.size(), section.header.addralign);
            } else {
               allocData = dataAllocator.allocate(buffer.size(), section.header.addralign);
            }
         } else {
            allocData = loadAllocator.allocate(buffer.size(), section.header.addralign);
         }

         memcpy(allocData, buffer.data(), buffer.size());
         section.memory = reinterpret_cast<uint8_t*>(allocData);
         section.virtAddress = mem::untranslate(allocData);
         section.virtSize = static_cast<uint32_t>(buffer.size());
      }
   }

   // Read strtab
   auto shStrTab = reinterpret_cast<const char*>(sections[header->shstrndx].memory);

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
   auto thrbss = findSection(sections, shStrTab, ".thrbss");

   if (thrdata || thrbss) {
      // Calculate TLS size
      uint32_t start, end;
      if (thrdata) {
         start = thrdata->virtAddress;
         end = thrdata->virtAddress + thrdata->virtSize;
         if (thrbss) {
            decaf_assert(thrbss->virtAddress >= end, fmt::format(
                            "Module {} has strange TLS section order: .thrdata = 0x{:08X}+0x{:X}, .thrbss = 0x{:08X}+0x{:X}",
                            moduleName,
                            thrdata->virtAddress, thrdata->virtAddress + thrdata->virtSize,
                            thrbss->virtAddress, thrbss->virtAddress + thrbss->virtSize));
            end = thrbss->virtAddress + thrbss->virtSize;
         }
      } else {
         start = thrbss->virtAddress;
         end = thrbss->virtAddress + thrbss->virtSize;
      }

      loadedMod->tlsBase = start;

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
   auto trampSeg = AddressRange { };

   if (!processRelocations(loadedMod, sections, data, shStrTab, codeAllocator, trampSeg)) {
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
   auto entryPoint = calculateRelocatedAddress(header->entry, sections);

   // Mark read-only segments as read-only
   for (auto &section : sections) {
      if (section.header.type == elf::SHT_PROGBITS) {
         bool isReadOnly;
         auto sectionName = shStrTab + section.header.name;
         if (decaf::config::jit::rodata_read_only
          && strcmp(sectionName, ".rodata") == 0) {
            isReadOnly = true;
         } else {
            isReadOnly = !(section.header.flags & elf::SHF_WRITE);
         }

         if (isReadOnly) {
            cpu::addJitReadOnlyRange(section.virtAddress, section.virtSize);
         }
      }
   }

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
   loaderFree(loadSegment);

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
      auto result = fs->openFile("/vol/code/" + fileName, fs::File::Read);

      if (result) {
         auto fh = result.value();
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
loadRPL(const std::string &name)
{
   // Use the scheduler lock to protect access to the loaders memory
   lockLoader();
   auto res = loadRPLNoLock(name);
   unlockLoader();
   return res;
}

void
lockLoader()
{
   coreinit::internal::acquireIdLock(sLoaderLock, cpu::this_core::id());
}

void
unlockLoader()
{
   coreinit::internal::releaseIdLock(sLoaderLock, cpu::this_core::id());
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

std::map<std::string, LoadedModule *>
getLoadedModules()
{
   return sLoadedModules;
}

void
setSyscallAddress(ppcaddr_t address)
{
   sSyscallAddress = address;
}

} // namespace loader

} // namespace kernel
