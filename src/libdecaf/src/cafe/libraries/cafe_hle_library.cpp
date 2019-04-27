#include "cafe_hle.h"
#include "cafe_hle_library.h"

#include "cafe/loader/cafe_loader_rpl.h"
#include "decaf_config.h"

#include <common/log.h>
#include <fstream>
#include <libcpu/cpu.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <zlib.h>

namespace rpl = cafe::loader::rpl;

namespace cafe::hle
{

static std::unordered_map<uint32_t, UnimplementedLibraryFunction*> sUnimplementedSystemCalls;
static virt_ptr<void> sUnimplementedFunctionStubMemory = nullptr;
static uint32_t sUnimplementedFunctionStubPos = 0u;
static uint32_t sUnimplementedFunctionStubSize = 0u;

virt_addr
registerUnimplementedSymbol(std::string_view module,
                            std::string_view name)
{
   auto libraryName = std::string { module } + ".rpl";
   auto library = getLibrary(libraryName);
   if (!library) {
      return virt_addr { 0u };
   }

   // Check if we already created an unimplemented function stub.
   if (auto unimpl = library->findUnimplementedFunctionExport(name)) {
      return unimpl->value;
   }

   // Ensure we have sufficient stub memory
   if (sUnimplementedFunctionStubPos + 8 >= sUnimplementedFunctionStubSize) {
      gLog->error("Out of stub memory for unimplemented function {}::{}",
                  module, name);
      return virt_addr { 0u };
   }

   // Create a new unimplemented function
   auto unimpl = new UnimplementedLibraryFunction { };
   unimpl->library = library;
   unimpl->syscallID = cpu::registerIllegalSystemCall();
   unimpl->name = name;
   unimpl->value = virt_cast<virt_addr>(sUnimplementedFunctionStubMemory)
                   + sUnimplementedFunctionStubPos;

   // Generate a kc, blr stub
   auto stub = virt_cast<uint32_t *>(unimpl->value);
   auto kc = espresso::encodeInstruction(espresso::InstructionID::kc);
   kc.kcn = unimpl->syscallID;
   stub[0] = kc.value;

   auto bclr = espresso::encodeInstruction(espresso::InstructionID::bclr);
   bclr.bo = 0b10100;
   stub[1] = bclr.value;
   sUnimplementedFunctionStubPos += 8;

   // Add to the list
   library->addUnimplementedFunctionExport(unimpl);
   sUnimplementedSystemCalls[unimpl->syscallID] = unimpl;
   gLog->debug("Unimplemented function import {}::{} added at {}",
               module, name, unimpl->value);
   return unimpl->value;
}

void
setUnimplementedFunctionStubMemory(virt_ptr<void> base,
                                   uint32_t size)
{
   sUnimplementedFunctionStubPos = 0;
   sUnimplementedFunctionStubSize = size;
   sUnimplementedFunctionStubMemory = base;
}

cpu::Core *
Library::handleUnknownSystemCall(cpu::Core *state,
                                 uint32_t id)
{
   // Because we register explicit handlers for all of the valid kernel
   // calls, if we get an unknown kernel call it must either be one of
   // the unimplemented functions we registered, or something has gone
   // awry and we can't continue anyways...

   auto unimplIter = sUnimplementedSystemCalls.find(id);
   if (unimplIter != sUnimplementedSystemCalls.end()) {
      auto &unimpl = unimplIter->second;

      gLog->warn("Unimplemented function call {}::{} from 0x{:08X}",
                 unimpl->library ? unimpl->library->name().c_str() : "<unknown>",
                 unimpl->name,
                 state->lr);

      // Set r3 to some nonsense value to try and catch errors from
      // unimplemented functions sooner.
      state->gpr[3] = 0xC5C5C5C5u;

      return state;
   }

   decaf_abort(fmt::format("Unexpected kernel call {} from 0x{:08X}",
                           id, state->lr));
}

void
Library::registerSystemCalls()
{
   for (auto const &[name, symbol] : mSymbolMap) {
      if (symbol->type == LibrarySymbol::Function) {
         auto funcSymbol = static_cast<LibraryFunction *>(symbol.get());
         auto newKcId = cpu::registerSystemCallHandler(funcSymbol->invokeHandler);
         funcSymbol->syscallID = newKcId;
      }
   }
}

void
Library::relocate(virt_addr textBaseAddress,
                  virt_addr dataBaseAddress)
{
   for (auto const &[name, symbol] : mSymbolMap) {
      if (symbol->type == LibrarySymbol::Function) {
         auto funcSymbol = static_cast<LibraryFunction *>(symbol.get());
         funcSymbol->address = textBaseAddress + funcSymbol->offset;

         if (funcSymbol->hostPtr) {
            *(funcSymbol->hostPtr) = virt_cast<void *>(funcSymbol->address);
         }
      } else if (symbol->type == LibrarySymbol::Data) {
         auto dataSymbol = static_cast<LibraryData *>(symbol.get());
         dataSymbol->address = dataBaseAddress + dataSymbol->offset;

         if (dataSymbol->hostPointer) {
            *(dataSymbol->hostPointer) = virt_cast<void *>(dataSymbol->address);
         }

         // TODO: When we relocate for a process switch we should not call this
         // constructor - must differentiate between relocate for load and
         // relocate for process switch.
         if (dataSymbol->constructor) {
            (*dataSymbol->constructor)(virt_cast<void *>(dataSymbol->address).get());
         }
      }
   }
}

static uint32_t
addSectionString(std::vector<uint8_t> &data,
                 std::string_view name)
{
   auto pos = data.size();
   data.insert(data.end(), name.begin(), name.end());
   data.push_back(0);
   return static_cast<uint32_t>(pos);
}

static void
generateTypeDescriptors(Library *library,
                        std::vector<LibraryTypeInfo> &types,
                        const uint32_t dataBaseAddr,
                        std::vector<uint8_t> &data,
                        std::vector<uint8_t> &relocations)
{
   auto stdTypeInfoOffset = uint32_t { 0u };

   auto addRelocation =
      [&relocations](const int32_t srcOffset,
                     const uint32_t dstOffset,
                     const uint32_t symbol)
      {
         auto rela = rpl::Rela { };
         rela.info = rpl::R_PPC_ADDR32 | (symbol << 8);
         rela.addend = srcOffset;
         rela.offset = dstOffset;
         relocations.insert(relocations.end(),
                            reinterpret_cast<uint8_t *>(&rela),
                            reinterpret_cast<uint8_t *>(&rela) + sizeof(rpl::Rela));
      };

   // Add a relocation against symbol 1 ($TEXT)
   auto addTextRelocation =
      [&addRelocation, dataBaseAddr](const uint32_t srcOffset,
                                     const uint32_t dstOffset)
      {
         addRelocation(srcOffset,
                       dstOffset + dataBaseAddr, 1);
      };

   // Add a relocation against symbol 2 ($DATA)
   auto addDataRelocation =
      [&addRelocation, dataBaseAddr](const uint32_t srcOffset,
                                     const uint32_t dstOffset)
      {
         addRelocation(srcOffset,
                       dstOffset + dataBaseAddr, 2);
      };

   auto addTypeDescriptor =
      [&](LibraryTypeInfo &typeInfo)
      {
         typeInfo.nameOffset = addSectionString(data, typeInfo.name);

         // Allocate some memory so the address acts as a unique type id
         typeInfo.typeIdOffset = static_cast<uint32_t>(data.size());
         data.resize(data.size() + 4);

         if (!typeInfo.baseTypes.empty()) {
            // Reserve space for base types
            typeInfo.baseTypeOffset = static_cast<uint32_t>(data.size());
            data.resize(data.size() + sizeof(ghs::BaseTypeDescriptor) * typeInfo.baseTypes.size());

            // Last base type flags = 0x1600
            *reinterpret_cast<be2_val<uint32_t> *>(&data[data.size() - 4]) = 0x1600u;
         }

         // Insert type descriptor, all the values are filled via relocations
         typeInfo.typeDescriptorOffset = static_cast<uint32_t>(data.size());
         data.resize(data.size() + sizeof(ghs::TypeDescriptor));
         addDataRelocation(typeInfo.typeDescriptorOffset + 0x00, stdTypeInfoOffset);
         addDataRelocation(typeInfo.typeDescriptorOffset + 0x04, typeInfo.nameOffset);
         addDataRelocation(typeInfo.typeDescriptorOffset + 0x08, typeInfo.typeIdOffset);
         if (!typeInfo.baseTypes.empty()) {
            addDataRelocation(typeInfo.typeDescriptorOffset + 0x0C, typeInfo.baseTypeOffset);
         }

         // Create virtual table
         auto virtualTableOffset = static_cast<uint32_t>(data.size());

         {
            // First entry points to the type descriptor
            auto entryOffset = static_cast<uint32_t>(data.size());
            data.resize(data.size() + sizeof(ghs::VirtualTable));
            addDataRelocation(entryOffset + 4, typeInfo.typeDescriptorOffset);
         }

         for (auto entry : typeInfo.virtualTable) {
            auto entryOffset = static_cast<uint32_t>(data.size());
            auto symbol = library->findSymbol(entry);
            decaf_assert(symbol,
                         fmt::format("Could not find vtable function {}", entry));
            data.resize(data.size() + sizeof(ghs::VirtualTable));
            addTextRelocation(entryOffset + 4, symbol->offset);
         }

         return typeInfo.typeDescriptorOffset;
      };

   // Generate type descriptors
   LibraryTypeInfo stdTypeInfo;
   stdTypeInfo.name = "std::type_info";
   stdTypeInfoOffset = addTypeDescriptor(stdTypeInfo);

   for (auto &type : types) {
      addTypeDescriptor(type);
   }

   /*
    * Add relocations for base types.
    *
    * We do this here instead of in addTypeDescriptor so we do not restrict
    * ourselves to defining base types before the types that inherit them.
    */
   auto findTypeInfo =
      [&types](const char *name) -> LibraryTypeInfo *
      {
         for (auto &type : types) {
            if (strcmp(type.name, name) == 0) {
               return &type;
            }
         }

         return nullptr;
      };

   for (auto &type : types) {
      auto baseTypeOffset = type.baseTypeOffset;

      for (auto baseType : type.baseTypes) {
         auto baseTypeInfo = findTypeInfo(baseType);
         decaf_assert(baseTypeInfo,
                      fmt::format("Could not find base type {}", baseType));
         addDataRelocation(baseTypeOffset,
                           baseTypeInfo->typeDescriptorOffset);
         baseTypeOffset += 8;
      }
   }
}

constexpr auto LibraryFunctionStubSize = 8u;
constexpr auto CodeBaseAddress = 0x02000000u;
constexpr auto DataBaseAddress = 0x10000000u;
constexpr auto LoadBaseAddress = 0xC0000000u;

// NULL, .text, .fexports, .data, .dexports, .rela.fexports, .rela.dexports,
// .symtab, .strtab, .shstrtab, SHT_RPL_CRCS, SHT_RPL_FILEINFO
constexpr auto NumSections = 12;

struct Section
{
   rpl::SectionHeader header;
   std::vector<uint8_t> data;
};

void
Library::generateRpl()
{
   // Build up our symbol information
   auto funcSymbols = std::vector<LibraryFunction *> { };
   auto dataSymbols = std::vector<LibraryData *> { };
   auto numDataExports = 0u;
   auto numCodeExports = 0u;
   auto dataSymbolsSize = 0u;
   auto textSymbolSize = 0u;

   for (auto const &[name, symbol] : mSymbolMap) {
      if (symbol->type == LibrarySymbol::Function) {
         auto funcSymbol = static_cast<LibraryFunction *>(symbol.get());
         textSymbolSize += LibraryFunctionStubSize;

         if (symbol->exported) {
            numCodeExports++;
         }

         funcSymbols.push_back(funcSymbol);
      } else if (symbol->type == LibrarySymbol::Data) {
         auto dataSymbol = static_cast<LibraryData *>(symbol.get());
         dataSymbolsSize = align_up(dataSymbolsSize, dataSymbol->align);
         dataSymbolsSize += dataSymbol->size;

         if (symbol->exported) {
            numDataExports++;
         }

         dataSymbols.push_back(dataSymbol);
      }
   }

   // Calculate required number of sections
   auto numSections = 1u;
   auto nullSectionIndex = 0u;
   auto textSectionIndex = 0u;
   auto fexportSectionIndex = 0u;
   auto fexportRelaSectionIndex = 0u;
   auto dataSectionIndex = 0u;
   auto dataRelaSectionIndex = 0u;
   auto dexportSectionIndex = 0u;
   auto dexportRelaSectionIndex = 0u;
   auto firstImportSectionIndex = 0u;

   if (textSymbolSize) {
      textSectionIndex = numSections++;
   }

   if (numCodeExports) {
      fexportSectionIndex = numSections++;
      fexportRelaSectionIndex = numSections++;
   }

   if (dataSymbolsSize || !mTypeInfo.empty()) {
      dataSectionIndex = numSections++;
   }

   if (!mTypeInfo.empty()) {
      dataRelaSectionIndex = numSections++;
   }

   if (numDataExports) {
      dexportSectionIndex = numSections++;
      dexportRelaSectionIndex = numSections++;
   }

   if (mLibraryDependencies.size()) {
      firstImportSectionIndex = numSections;
      numSections += static_cast<unsigned int>(mLibraryDependencies.size());
   }

   auto symTabSectionIndex = numSections++;
   auto strTabSectionIndex = numSections++;
   auto shStrTabSectionIndex = numSections++;
   auto crcSectionIndex = numSections++;
   auto fileInfoSectionIndex = numSections++;

   auto sections = std::vector<Section> { };
   sections.resize(numSections);

   auto nullSection = sections.begin() + nullSectionIndex;
   auto textSection = sections.begin() + textSectionIndex;
   auto fexportSection = sections.begin() + fexportSectionIndex;
   auto fexportRelaSection = sections.begin() + fexportRelaSectionIndex;
   auto dataSection = sections.begin() + dataSectionIndex;
   auto dataRelaSection = sections.begin() + dataRelaSectionIndex;
   auto dexportSection = sections.begin() + dexportSectionIndex;
   auto dexportRelaSection = sections.begin() + dexportRelaSectionIndex;
   auto firstImportSection = sections.begin() + firstImportSectionIndex;
   auto symTabSection = sections.begin() + symTabSectionIndex;
   auto strTabSection = sections.begin() + strTabSectionIndex;
   auto shStrTabSection = sections.begin() + shStrTabSectionIndex;
   auto crcSection = sections.begin() + crcSectionIndex;
   auto fileInfoSection = sections.begin() + fileInfoSectionIndex;
   auto loadAddr = LoadBaseAddress;

   // Add empty string to string sections
   shStrTabSection->data.push_back(0);
   strTabSection->data.push_back(0);

   // Generate .text
   if (textSectionIndex) {
      textSection->header.name = addSectionString(shStrTabSection->data, ".text");
      textSection->header.type = rpl::SHT_PROGBITS;
      textSection->header.flags = rpl::SHF_EXECINSTR | rpl::SHF_ALLOC;
      textSection->header.addralign = 32u;
      textSection->header.addr = CodeBaseAddress;
      textSection->header.offset = 0u;
      textSection->header.size = 0u;
      textSection->header.link = 0u;
      textSection->header.info = 0u;
      textSection->header.entsize = 0u;
   }

   // Generate .fexports
   if (fexportSectionIndex) {
      fexportSection->header.name = addSectionString(shStrTabSection->data, ".fexports");
      fexportSection->header.type = rpl::SHT_RPL_EXPORTS;
      fexportSection->header.flags = rpl::SHF_EXECINSTR | rpl::SHF_ALLOC;
      fexportSection->header.addralign = 4u;
      fexportSection->header.offset = 0u;
      fexportSection->header.addr = align_up(loadAddr, fexportSection->header.addralign);
      fexportSection->header.size = 8u + static_cast<uint32_t>(sizeof(rpl::Export) * numCodeExports);
      fexportSection->header.link = 0u;
      fexportSection->header.info = 0u;
      fexportSection->header.entsize = 0u;
      fexportSection->data.resize(fexportSection->header.size);

      auto exports = rpl::Exports { };
      exports.count = numCodeExports;
      exports.signature = 0u;
      std::memcpy(fexportSection->data.data(), &exports, sizeof(rpl::Exports));
   }

   // Generate .rela.fexports
   if (fexportRelaSectionIndex) {
      fexportRelaSection->header.name = addSectionString(shStrTabSection->data, ".rela.fexports");
      fexportRelaSection->header.type = rpl::SHT_RELA;
      fexportRelaSection->header.flags = 0u;
      fexportRelaSection->header.addralign = 4u;
      fexportRelaSection->header.addr = 0u;
      fexportRelaSection->header.offset = 0u;
      fexportRelaSection->header.size = static_cast<uint32_t>(sizeof(rpl::Rela) * numCodeExports);
      fexportRelaSection->header.link = symTabSectionIndex;
      fexportRelaSection->header.info = fexportSectionIndex;
      fexportRelaSection->header.entsize = static_cast<uint32_t>(sizeof(rpl::Rela));
      fexportRelaSection->data.resize(fexportRelaSection->header.size);
   }

   if (textSymbolSize) {
      auto exportIdx = 0u;
      auto textOffset = static_cast<uint32_t>(textSection->data.size());
      textSection->data.resize(textSection->data.size() + textSymbolSize);

      for (auto &symbol : funcSymbols) {
         // Write syscall thunk
         auto kc = espresso::encodeInstruction(espresso::InstructionID::kc);
         kc.kcn = symbol->syscallID;

         auto bclr = espresso::encodeInstruction(espresso::InstructionID::bclr);
         bclr.bo = 20;
         bclr.bi = 0;

         auto thunk = reinterpret_cast<be2_val<uint32_t> *>(textSection->data.data() + textOffset);
         *(thunk + 0) = kc.value;
         *(thunk + 1) = bclr.value;

         if (symbol->exported) {
            auto exportOffset = 8u + (sizeof(rpl::Export) * exportIdx);
            auto relaOffset = sizeof(rpl::Rela) * exportIdx;

            // Write to .fexport
            auto fexport = rpl::Export { };
            fexport.name = addSectionString(fexportSection->data, symbol->name);
            fexport.value = textSection->header.addr + textOffset;
            std::memcpy(fexportSection->data.data() + exportOffset,
                        &fexport, sizeof(rpl::Export));

            // Write to .rela.fexport
            auto rela = rpl::Rela { };
            rela.info = rpl::R_PPC_ADDR32 | (symbol->index << 8);
            rela.addend = 0;
            rela.offset = static_cast<uint32_t>(fexportSection->header.addr + exportOffset);
            std::memcpy(fexportRelaSection->data.data() + relaOffset,
                        &rela, sizeof(rpl::Rela));
            ++exportIdx;
         }

         symbol->offset = textOffset;
         textOffset += LibraryFunctionStubSize;
      }

      if (fexportSectionIndex) {
         // Update loadAddr
         loadAddr = fexportSection->header.addr + static_cast<uint32_t>(fexportSection->data.size());
      }
   }

   // Generate .data
   if (dataSectionIndex) {
      dataSection->header.name = addSectionString(shStrTabSection->data, ".data");
      dataSection->header.type = rpl::SHT_PROGBITS;
      dataSection->header.flags = rpl::SHF_WRITE | rpl::SHF_ALLOC;
      dataSection->header.addralign = 32u;
      dataSection->header.addr = DataBaseAddress;
      dataSection->header.offset = 0u;
      dataSection->header.size = 0u;
      dataSection->header.link = 0u;
      dataSection->header.info = 0u;
      dataSection->header.entsize = 0u;
   }

   // Generate .rela.data
   if (dataRelaSectionIndex) {
      dataRelaSection->header.name = addSectionString(shStrTabSection->data, ".rela.data");
      dataRelaSection->header.type = rpl::SHT_RELA;
      dataRelaSection->header.flags = 0u;
      dataRelaSection->header.addralign = 4u;
      dataRelaSection->header.addr = 0u;
      dataRelaSection->header.offset = 0u;
      dataRelaSection->header.size = 0u;
      dataRelaSection->header.link = symTabSectionIndex;
      dataRelaSection->header.info = dataSectionIndex;
      dataRelaSection->header.entsize = static_cast<uint32_t>(sizeof(rpl::Rela));
   }

   // Generate .dexports
   if (dexportSectionIndex) {
      dexportSection->header.name = addSectionString(shStrTabSection->data, ".dexports");
      dexportSection->header.type = rpl::SHT_RPL_EXPORTS;
      dexportSection->header.flags = rpl::SHF_ALLOC;
      dexportSection->header.addralign = 4u;
      dexportSection->header.offset = 0u;
      dexportSection->header.addr = align_up(loadAddr, dexportSection->header.addralign);
      dexportSection->header.size = 8u + static_cast<uint32_t>(sizeof(rpl::Export) * numDataExports);
      dexportSection->header.link = 0u;
      dexportSection->header.info = 0u;
      dexportSection->header.entsize = 0u;
      dexportSection->data.resize(dexportSection->header.size);

      auto exports = rpl::Exports { };
      exports.count = numDataExports;
      exports.signature = 0u;
      std::memcpy(dexportSection->data.data(), &exports, sizeof(rpl::Exports));
   }

   // Generate .rela.dexports
   if (dexportRelaSectionIndex) {
      dexportRelaSection->header.name = addSectionString(shStrTabSection->data, ".rela.dexports");
      dexportRelaSection->header.type = rpl::SHT_RELA;
      dexportRelaSection->header.flags = 0u;
      dexportRelaSection->header.addralign = 4u;
      dexportRelaSection->header.addr = 0u;
      dexportRelaSection->header.offset = 0u;
      dexportRelaSection->header.size = static_cast<uint32_t>(sizeof(rpl::Rela) * numDataExports);
      dexportRelaSection->header.link = symTabSectionIndex;
      dexportRelaSection->header.info = dexportSectionIndex;
      dexportRelaSection->header.entsize = static_cast<uint32_t>(sizeof(rpl::Rela));
      dexportRelaSection->data.resize(dexportRelaSection->header.size);
   }

   // Write symbols and exports
   if (dataSymbolsSize) {
      auto exportIdx = 0;
      auto dataOffset = static_cast<uint32_t>(dataSection->data.size());
      dataSection->data.resize(dataSection->data.size() + dataSymbolsSize);

      for (auto &symbol : dataSymbols) {
         dataOffset = align_up(dataOffset, symbol->align);

         if (symbol->exported) {
            auto exportOffset = 8 + (sizeof(rpl::Export) * exportIdx);
            auto relaOffset = sizeof(rpl::Rela) * exportIdx;

            // Write to .dexport
            auto dexport = rpl::Export { };
            dexport.name = addSectionString(dexportSection->data, symbol->name);
            dexport.value = dataSection->header.addr + dataOffset;
            std::memcpy(dexportSection->data.data() + exportOffset,
                        &dexport, sizeof(rpl::Export));

            // Write to .rela.dexport
            auto rela = rpl::Rela { };
            rela.info = rpl::R_PPC_ADDR32 | (symbol->index << 8);
            rela.addend = 0;
            rela.offset = static_cast<uint32_t>(dexportSection->header.addr + exportOffset);
            std::memcpy(dexportRelaSection->data.data() + relaOffset,
                        &rela, sizeof(rpl::Rela));
            ++exportIdx;
         }

         symbol->offset = dataOffset;
         dataOffset += symbol->size;
      }

      decaf_check(dataOffset == dataSymbolsSize);

      if (dexportSectionIndex) {
         // Update loadAddr
         loadAddr = dexportSection->header.addr + static_cast<uint32_t>(dexportSection->data.size());
      }
   }

   // Generate .fimport_{}
   if (firstImportSectionIndex) {
      for (auto i = 0u; i < mLibraryDependencies.size(); ++i) {
         auto importSection = firstImportSection + i;
         importSection->header.name =
            addSectionString(shStrTabSection->data,
                             fmt::format(".fimport_{}", mLibraryDependencies[i]));
         importSection->header.type = rpl::SHT_RPL_IMPORTS;
         importSection->header.flags = rpl::SHF_ALLOC | rpl::SHF_EXECINSTR;
         importSection->header.addralign = 4u;
         importSection->header.offset = 0u;
         importSection->header.addr = align_up(loadAddr, importSection->header.addralign);
         importSection->header.size = 8u;
         importSection->header.link = 0u;
         importSection->header.info = 0u;
         importSection->header.entsize = 0u;
         importSection->data.resize(importSection->header.size);

         auto imports = rpl::Imports { };
         imports.count = 0u;
         imports.signature = 0u;
         std::memcpy(importSection->data.data(),
                     &imports, sizeof(rpl::Imports));

         loadAddr = importSection->header.addr + static_cast<uint32_t>(importSection->data.size());
      }
   }

   if (!mTypeInfo.empty()) {
      generateTypeDescriptors(this,
                              mTypeInfo,
                              dataSection->header.addr,
                              dataSection->data,
                              dataRelaSection->data);
   }

   // Generate symtab
   auto symbolCount = funcSymbols.size() + dataSymbols.size() + cafe::hle::Library::BaseSymbolIndex;
   symTabSection->data.resize(sizeof(rpl::Symbol) * symbolCount);

   auto getSymbol =
      [&symTabSection](uint32_t index)
      {
         return reinterpret_cast<rpl::Symbol *>(symTabSection->data.data() + (index * sizeof(rpl::Symbol)));
      };

   // Generate NULL symbol
   {
      auto nullSymbol = getSymbol(0);
      std::memset(nullSymbol, 0, sizeof(rpl::Symbol));
   }

   // Generate $TEXT symbol
   {
      auto textSymbol = getSymbol(1);
      textSymbol->name = addSectionString(strTabSection->data, "$TEXT");
      textSymbol->value = CodeBaseAddress;
      textSymbol->size = 0u;
      textSymbol->info = static_cast<uint8_t>(rpl::STT_SECTION | (rpl::STB_LOCAL << 4));
      textSymbol->shndx = static_cast<uint16_t>(textSectionIndex);
      textSymbol->other = uint8_t { 0 };
   }

   // Generate $DATA symbol
   {
      auto dataSymbol = getSymbol(2);
      dataSymbol->name = addSectionString(strTabSection->data, "$DATA");
      dataSymbol->value = DataBaseAddress;
      dataSymbol->size = 0u;
      dataSymbol->info = static_cast<uint8_t>(rpl::STT_SECTION | (rpl::STB_LOCAL << 4));
      dataSymbol->shndx = static_cast<uint16_t>(dataSectionIndex);
      dataSymbol->other = uint8_t { 0 };
   }

   for (auto &dataSymbol : dataSymbols) {
      auto symbol = getSymbol(dataSymbol->index);
      auto binding = dataSymbol->exported ? rpl::STB_GLOBAL : rpl::STB_LOCAL;
      symbol->name = addSectionString(strTabSection->data, dataSymbol->name);
      symbol->value = dataSymbol->offset + dataSection->header.addr;
      symbol->size = dataSymbol->size;
      symbol->info = static_cast<uint8_t>(rpl::STT_OBJECT | (binding << 4));
      symbol->shndx = static_cast<uint16_t>(dataSectionIndex);
      symbol->other = uint8_t { 0 };
   }

   for (auto &funcSymbol : funcSymbols) {
      auto symbol = getSymbol(funcSymbol->index);
      auto binding = funcSymbol->exported ? rpl::STB_GLOBAL : rpl::STB_LOCAL;
      symbol->name = addSectionString(strTabSection->data, funcSymbol->name);
      symbol->value = funcSymbol->offset + textSection->header.addr;
      symbol->size = LibraryFunctionStubSize;
      symbol->info = static_cast<uint8_t>(rpl::STT_FUNC | (binding << 4));
      symbol->shndx = static_cast<uint16_t>(textSectionIndex);
      symbol->other = uint8_t { 0 };
   }

   symTabSection->header.name = addSectionString(shStrTabSection->data, ".symtab");
   symTabSection->header.type = rpl::SHT_SYMTAB;
   symTabSection->header.flags = rpl::SHF_ALLOC;
   symTabSection->header.addralign = 4u;
   symTabSection->header.addr = align_up(loadAddr, symTabSection->header.addralign);
   symTabSection->header.offset = 0u; // Set later
   symTabSection->header.size = static_cast<uint32_t>(symTabSection->data.size());
   symTabSection->header.link = strTabSectionIndex;
   symTabSection->header.info = 1u;
   symTabSection->header.entsize = static_cast<uint32_t>(sizeof(rpl::Symbol));
   loadAddr = symTabSection->header.addr + symTabSection->header.size;

   // Generate strtab
   strTabSection->header.name = addSectionString(shStrTabSection->data, ".strtab");
   strTabSection->header.type = rpl::SHT_STRTAB;
   strTabSection->header.flags = rpl::SHF_ALLOC;
   strTabSection->header.addralign = 4u;
   strTabSection->header.addr = align_up(loadAddr, strTabSection->header.addralign);
   strTabSection->header.offset = 0u; // Set later
   strTabSection->header.size = static_cast<uint32_t>(strTabSection->data.size());
   strTabSection->header.link = 0u;
   strTabSection->header.info = 0u;
   strTabSection->header.entsize = 0u;
   loadAddr = strTabSection->header.addr + strTabSection->header.size;

   // Generate shstrtab
   shStrTabSection->header.name = addSectionString(shStrTabSection->data, ".shstrtab");
   shStrTabSection->header.type = rpl::SHT_STRTAB;
   shStrTabSection->header.flags = rpl::SHF_ALLOC;
   shStrTabSection->header.addralign = 4u;
   shStrTabSection->header.addr = align_up(loadAddr, shStrTabSection->header.addralign);
   shStrTabSection->header.offset = 0u; // Set later
   shStrTabSection->header.size = static_cast<uint32_t>(shStrTabSection->data.size());
   shStrTabSection->header.link = 0u;
   shStrTabSection->header.info = 0u;
   shStrTabSection->header.entsize = 0u;
   loadAddr = shStrTabSection->header.addr + shStrTabSection->header.size;

   // Generate SHT_RPL_FILEINFO
   fileInfoSection->header.name = 0u;
   fileInfoSection->header.type = rpl::SHT_RPL_FILEINFO;
   fileInfoSection->header.flags = 0u;
   fileInfoSection->header.addralign = 4u;
   fileInfoSection->header.addr = 0u;
   fileInfoSection->header.offset = 0u; // Set later
   fileInfoSection->header.size = static_cast<uint32_t>(sizeof(rpl::RPLFileInfo_v4_2));
   fileInfoSection->header.link = 0u;
   fileInfoSection->header.info = 0u;
   fileInfoSection->header.entsize = 0u;
   fileInfoSection->data.resize(fileInfoSection->header.size);

   auto infoFileName = addSectionString(fileInfoSection->data, mName);
   auto info = reinterpret_cast<rpl::RPLFileInfo_v4_2 *>(fileInfoSection->data.data());
   info->version = 0xCAFE0402u;
   info->textSize = 0u;
   info->textAlign = 32u;
   info->dataSize = 0u;
   info->dataAlign = 4096u;
   info->loadSize = 0u;
   info->loadAlign = 4u;
   info->tempSize = 0u;
   info->trampAdjust = 0u;
   info->trampAddition = 0u;
   info->sdaBase = 0u;
   info->sda2Base = 0u;
   info->stackSize = 0x10000u;
   info->filename = infoFileName;
   info->heapSize = 0x8000u;
   info->flags = 0u;
   info->minVersion = 0x5078u;
   info->compressionLevel = -1;
   info->fileInfoPad = 0u;
   info->cafeSdkVersion = 0x5335u;
   info->cafeSdkRevision = 0x10D4Bu;
   info->tlsAlignShift = uint16_t { 0u };
   info->tlsModuleIndex = int16_t { 0 };
   info->runtimeFileInfoSize = 0u;
   info->tagOffset = 0u;

   for (auto &section : sections) {
      if (section.data.size()) {
         section.header.size = static_cast<uint32_t>(section.data.size());
      }

      auto size = section.header.size;
      if (section.header.addr >= CodeBaseAddress &&
          section.header.addr < DataBaseAddress) {
         auto val = section.header.addr + section.header.size - CodeBaseAddress;
         if (val > info->textSize) {
            info->textSize = val;
         }
      } else if (section.header.addr >= DataBaseAddress &&
                 section.header.addr < LoadBaseAddress) {
         auto val = section.header.addr + section.header.size - DataBaseAddress;
         if (val > info->dataSize) {
            info->dataSize = val;
         }
      } else if (section.header.addr >= LoadBaseAddress) {
         auto val = section.header.addr + section.header.size - LoadBaseAddress;
         if (val > info->loadSize) {
            info->loadSize = val;
         }
      } else if (section.header.addr == 0 &&
                 section.header.type != rpl::SHT_RPL_CRCS &&
                 section.header.type != rpl::SHT_RPL_FILEINFO) {
         info->tempSize += (size + 128);
      }
   }

   info->textSize = align_up(info->textSize, info->textAlign);
   info->dataSize = align_up(info->dataSize, info->dataAlign);
   info->loadSize = align_up(info->loadSize, info->loadAlign);

   // Generate SHT_RPL_CRCS
   crcSection->header.name = 0u;
   crcSection->header.type = rpl::SHT_RPL_CRCS;
   crcSection->header.flags = 0u;
   crcSection->header.addralign = 4u;
   crcSection->header.addr = 0u;
   crcSection->header.offset = 0u; // Set later
   crcSection->header.size = static_cast<uint32_t>(4 * sections.size());
   crcSection->header.link = 0u;
   crcSection->header.info = 0u;
   crcSection->header.entsize = 4u;
   crcSection->data.resize(crcSection->header.size);

   for (auto i = 0u; i < sections.size(); ++i) {
      auto crc = uint32_t { 0u };
      auto &section = sections[i];

      if (section.data.size() && i != (sections.size() - 2)) {
         crc = crc32(0, Z_NULL, 0);
         crc = crc32(crc,
                     reinterpret_cast<Bytef *>(section.data.data()),
                     static_cast<uInt>(section.data.size()));
      }

      *reinterpret_cast<be2_val<uint32_t> *>(crcSection->data.data() + i * 4)
         = crc;
   }

   // Generate file header
   rpl::Header fileHeader;
   fileHeader.magic[0] = uint8_t { 0x7F };
   fileHeader.magic[1] = uint8_t { 'E' };
   fileHeader.magic[2] = uint8_t { 'L' };
   fileHeader.magic[3] = uint8_t { 'F' };

   fileHeader.fileClass = rpl::ELFCLASS32;
   fileHeader.encoding = rpl::ELFDATA2MSB;
   fileHeader.elfVersion = rpl::EV_CURRENT;
   fileHeader.abi = rpl::EABI_CAFE;
   fileHeader.abiVersion = rpl::EABI_VERSION_CAFE;

   fileHeader.type = uint16_t { 0xFE01 };
   fileHeader.machine = rpl::EM_PPC;
   fileHeader.version = 1u;
   fileHeader.phoff = 0u;
   fileHeader.shoff = 0x40u;
   fileHeader.flags = 0u;
   fileHeader.ehsize = static_cast<uint16_t>(sizeof(rpl::Header));
   fileHeader.phentsize = uint16_t { 0 };
   fileHeader.phnum = uint16_t { 0 };
   fileHeader.shentsize = static_cast<uint16_t>(sizeof(rpl::SectionHeader));
   fileHeader.shnum = static_cast<uint16_t>(sections.size());
   fileHeader.shstrndx = static_cast<uint16_t>(shStrTabSectionIndex);

   // Find and set the entry point
   auto entryPointAddr = 0u;
   if (auto itr = mSymbolMap.find("rpl_entry"); itr != mSymbolMap.end()) {
      entryPointAddr = textSection->header.addr + itr->second->offset;
   }

   fileHeader.entry = entryPointAddr;

   // Calculate file offsets
   auto offset = static_cast<uint32_t>(fileHeader.shoff);
   offset +=
      align_up(static_cast<uint32_t>(
         sizeof(rpl::SectionHeader) * sections.size()), 64);

   crcSection->header.offset = offset;
   offset += crcSection->header.size;

   fileInfoSection->header.offset = offset;
   offset += fileInfoSection->header.size;

   // Data sections
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_PROGBITS &&
          !(section.header.flags & rpl::SHF_EXECINSTR)) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // Export sections
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_RPL_EXPORTS) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // Import sections
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_RPL_IMPORTS) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // symtab & strtab
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_SYMTAB ||
          section.header.type == rpl::SHT_STRTAB) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // Code sections
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_PROGBITS &&
         (section.header.flags & rpl::SHF_EXECINSTR)) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // Relocation sections
   for (auto &section : sections) {
      if (section.header.type == rpl::SHT_RELA) {
         section.header.offset = offset;
         offset += section.header.size;
      }
   }

   // Write out the generated RPL
   mGeneratedRpl.resize(offset, 0);
   std::memcpy(mGeneratedRpl.data() + 0, &fileHeader, sizeof(rpl::Header));

   offset = fileHeader.shoff;
   for (auto &section : sections) {
      std::memcpy(mGeneratedRpl.data() + offset,
                  &section.header,
                  sizeof(rpl::SectionHeader));
      offset += fileHeader.shentsize;
   }

   for (auto &section : sections) {
      if (section.header.offset && section.data.size()) {
         std::memcpy(mGeneratedRpl.data() + section.header.offset,
                     section.data.data(),
                     section.data.size());
      }
   }

   // TODO: Move this to a debug api command?
   if (decaf::config()->system.dump_hle_rpl) {
      std::ofstream out { mName, std::fstream::binary };
      out.write(reinterpret_cast<const char *>(mGeneratedRpl.data()),
                mGeneratedRpl.size());
   }
}

} // namespace cafe::hle
