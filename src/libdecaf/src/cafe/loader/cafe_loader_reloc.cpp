#include "cafe_loader_basics.h"
#include "cafe_loader_error.h"
#include "cafe_loader_flush.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_reloc.h"
#include "cafe_loader_rpl.h"
#include "cafe_loader_log.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_utils.h"

#include "cafe/libraries/cafe_hle.h"
#include <libcpu/be2_struct.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libcpu/espresso/espresso_spr.h>
#include <zlib.h>

namespace cafe::loader::internal
{

constexpr auto TrampSize = uint32_t { 16 };
static std::array<uint8_t, 0x1FF8> sRelocBuffer;

static virt_ptr<rpl::Export>
LiBinSearchExport(virt_ptr<rpl::Export> exports,
                  uint32_t numExports,
                  virt_ptr<char> name)
{
   if (!exports || !numExports || !name || !name[0]) {
      return nullptr;
   }

   auto strTable = virt_cast<char *>(virt_cast<virt_addr>(exports) - 8);
   auto left = 0u;
   auto right = numExports;

   while (left < right) {
      auto index = left + (right - left) / 2;
      auto exportName = strTable + (exports[index].name & 0x7FFFFFFF);
      auto cmpValue = strcmp(name.get(), exportName.get());
      if (cmpValue == 0) {
         return exports + index;
      } else if (cmpValue < 0) {
         right = index;
      } else {
         left = index + 1;
      }
   }

   return nullptr;
}

static int32_t
sFixupOneSymbolTable(virt_ptr<LOADED_RPL> rpl,
                     uint32_t sectionIndex,
                     virt_ptr<rpl::SectionHeader> sectionHeader,
                     virt_ptr<LiImportTracking> imports,
                     uint32_t unk)
{
   auto strTable = virt_ptr<char> { nullptr };
   LiCheckAndHandleInterrupts();

   if (sectionHeader->link < rpl->elfHeader.shnum) {
      strTable = virt_cast<char *>(rpl->sectionAddressBuffer[sectionHeader->link]);
   }

   auto symbolEntrySize = sectionHeader->entsize;
   if (!symbolEntrySize) {
      symbolEntrySize = static_cast<uint32_t>(sizeof(rpl::Symbol));
   }

   auto symbolBuffer = rpl->sectionAddressBuffer[sectionIndex];
   auto numSymbols = sectionHeader->size / symbolEntrySize;
   for (auto i = 1u; i < numSymbols; ++i) {
      auto symbol = virt_cast<rpl::Symbol *>(symbolBuffer + (i * symbolEntrySize));
      if (symbol->shndx == 0 || symbol->shndx >= rpl->elfHeader.shnum) {
         continue;
      }

      auto targetSectionAddress = rpl->sectionAddressBuffer[symbol->shndx];
      if (!targetSectionAddress) {
         symbol->value = 0xCD000000u | i;
         continue;
      }

      LiCheckAndHandleInterrupts();

      auto targetSectionHeader = getSectionHeader(rpl, symbol->shndx);
      auto targetSectionOffset = symbol->value - targetSectionHeader->addr;
      if (!imports || imports[symbol->shndx].numExports == 0 || targetSectionOffset < 8) {
         auto binding = symbol->info >> 4;
         auto type = symbol->info & 0xf;

         // Not really sure what this is doing tbh
         if (type != rpl::STT_TLS &&
             (targetSectionHeader->type != rpl::SHT_RPL_IMPORTS || unk != 1) &&
             unk != 2) {
            symbol->value = static_cast<uint32_t>(targetSectionAddress + targetSectionOffset);
         }

         continue;
      }

      if (!strTable) {
         Loader_ReportError("*** imports require a string table but there isn't one - can't link blind symbol.");
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sFixupOneSymbolTable", 529);
         return -470007;
      }

      auto symbolName = strTable + symbol->name;
      if (!symbolName[0]) {
         symbol->value = 0xCD000000u | i;
         continue;
      }

      auto &import = imports[symbol->shndx];
      auto symbolExport = LiBinSearchExport(import.exports,
                                            import.numExports,
                                            symbolName);
      if (symbolExport) {
         symbol->value = symbolExport->value;
      } else {
         symbol->value =
            cafe::hle::registerUnimplementedSymbol(
               std::string_view {
                  import.rpl->moduleNameBuffer.get(),
                  import.rpl->moduleNameLen
               },
               symbolName.get());
         if (!symbol->value) {
            // Must not be from a HLE library, so let's error out like normal
            Loader_ReportError("*** could not find imported symbol \"{}\".", symbolName);
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sFixupOneSymbolTable", 551);
            return -470005;
         }
      }
   }

   return 0;
}

static int32_t
LiCpu_RelocAdd(bool isRpx,
               virt_addr targetSectionBuffer,
               rpl::RelocationType relaType,
               bool isWeakSymbol,
               uint32_t offset,
               uint32_t addend,
               uint32_t symbolValue,
               virt_addr *preTrampBuffer,
               uint32_t *preTrampAvailableEntries,
               virt_addr *postTrampBuffer,
               uint32_t *postTrampAvailableEntries,
               int32_t tlsModuleIndex,
               uint32_t fileType)
{
   auto target = targetSectionBuffer + offset;
   auto value = symbolValue + addend;
   auto relValue = value - static_cast<uint32_t>(target);

   if (isRpx) {
      // SDA based relocations are only valid for a .rpx
      switch (relaType) {
      case rpl::R_PPC_EMB_SDA21:
      {
         auto ins = espresso::Instruction { +*virt_cast<uint32_t *>(target) };
         if (ins.rA == 2) {
            value -= getGlobalStorage()->sda2Base;
         } else if (ins.rA == 13) {
            value -= getGlobalStorage()->sdaBase;
         } else if (ins.rA != 0) {
            Loader_ReportError("***ERROR: SDA21_HI malformed.\n");
            LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 166);
            return -470039;
         }

         ins.simm = static_cast<uint16_t>(value);
         *virt_cast<uint32_t *>(target) = ins.value;
         return 0;
      }
      case rpl::R_PPC_EMB_RELSDA:
      case rpl::R_PPC_DIAB_SDA21_LO:
      case rpl::R_PPC_DIAB_SDA21_HI:
      case rpl::R_PPC_DIAB_SDA21_HA:
      case rpl::R_PPC_DIAB_RELSDA_LO:
      case rpl::R_PPC_DIAB_RELSDA_HI:
      case rpl::R_PPC_DIAB_RELSDA_HA:
         // TODO: Implement these relocation types
         decaf_abort(fmt::format("Unimplemented relocation type {}", relaType));
         return -470038;
      }
   }

   switch (relaType) {
   case rpl::R_PPC_NONE:
      break;
   case rpl::R_PPC_ADDR32:
      *virt_cast<uint32_t *>(target) = value;
      break;
   case rpl::R_PPC_ADDR16_LO:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>(value & 0xFFFF);
      break;
   case rpl::R_PPC_ADDR16_HI:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>(value >> 16);
      break;
   case rpl::R_PPC_ADDR16_HA:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>((value + 0x8000) >> 16);
      break;
   case rpl::R_PPC_DTPMOD32:
      *virt_cast<int32_t *>(target) = tlsModuleIndex;
      break;
   case rpl::R_PPC_DTPREL32:
      *virt_cast<uint32_t *>(target) = value;
      break;
   case rpl::R_PPC_GHS_REL16_HA:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>((value + 0x8000) >> 16);
      break;
   case rpl::R_PPC_GHS_REL16_HI:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>(relValue >> 16);
      break;
   case rpl::R_PPC_GHS_REL16_LO:
      *virt_cast<uint16_t *>(target) = static_cast<uint16_t>(relValue & 0xFFFF);
      break;
   case rpl::R_PPC_REL14:
   {
      if (isWeakSymbol && !symbolValue) {
         symbolValue = static_cast<uint32_t>(target);
         value = symbolValue + addend;
      }

      auto distance = static_cast<int32_t>(value) - static_cast<int32_t>(target);
      if (distance > 0x7FFC || distance < -0x7FFC) {
         Loader_ReportError("***14-bit relative branch cannot hit target.");
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 398);
         return -470039;
      }

      if (distance & 3) {
         Loader_ReportError("***RELOC ERROR {}: lower 2 bits must be zero before shifting.",
                            -470040);
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 408);
         return -470040;
      }

      if ((distance >= 0 && (distance & 0xFFFF8000)) ||
          (distance < 0 && ((distance & 0xFFFF8000) != 0xFFFF8000))) {
         Loader_ReportError(
            "***RELOC ERROR {}: upper 17 bits before shift must all be the same.",
            -470040);
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 429);
         return -470040;
      }

      *virt_cast<uint32_t *>(target) =
         (*virt_cast<uint32_t *>(target) & 0xFFBF0003) |
         static_cast<uint16_t>(distance & 0xFFFC);
      break;
   }
   case rpl::R_PPC_REL24:
   {
      if (isWeakSymbol && !symbolValue) {
         symbolValue = static_cast<uint32_t>(target);
         value = symbolValue + addend;
      }

      // Check if we need to generate a trampoline
      auto distance = static_cast<int32_t>(value) - static_cast<int32_t>(target);
      if (distance > 0x1FFFFFC || distance < -0x1FFFFFC) {
         auto tramp = virt_ptr<uint32_t> { nullptr };

         if (*postTrampBuffer - target <= 0x1FFFFFC) {
            if (*postTrampAvailableEntries == 0) {
               Loader_ReportError("Post tramp buffer exhausted.");
               LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 323);
               return -470037;
            }

            tramp = virt_cast<uint32_t *>(*postTrampBuffer);
            *postTrampBuffer += 16;
            *postTrampAvailableEntries -= 1;
         } else if (target - *preTrampBuffer <= 0x1FFFFFC) {
            if (*preTrampAvailableEntries == 0) {
               Loader_ReportError("Pre tramp buffer exhausted.");
               LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 310);
               return -470037;
            }

            tramp = virt_cast<uint32_t *>(*preTrampBuffer);
            *preTrampBuffer -= 16;
            *preTrampAvailableEntries -= 1;
         } else {
            Loader_ReportError("**Cannot link 24-bit jump (too far to tramp buffer).");
            LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 303);
            return -470037;
         }

         /* Write the trampoline:
          * lis r11, 0
          * ori r11, r11, 0
          * mtctr r11
          * bctr
          */
         auto lis = espresso::encodeInstruction(espresso::InstructionID::addis);
         lis.rD = 11;
         lis.rA = 0;
         lis.simm = (value >> 16) & 0xFFFF;

         auto ori = espresso::encodeInstruction(espresso::InstructionID::ori);
         ori.rA = 11;
         ori.rS = 11;
         ori.uimm = value & 0xFFFF;

         auto mtctr = espresso::encodeInstruction(espresso::InstructionID::mtspr);
         mtctr.rS = 11;
         espresso::encodeSPR(mtctr, espresso::SPR::CTR);

         auto bctr = espresso::encodeInstruction(espresso::InstructionID::bcctr);
         bctr.bo = 0b10100;

         tramp[0] = lis.value;
         tramp[1] = ori.value;
         tramp[2] = mtctr.value;
         tramp[3] = bctr.value;

         symbolValue = static_cast<uint32_t>(virt_cast<virt_addr>(tramp));
         value = symbolValue + addend;
         distance = static_cast<int32_t>(value) - static_cast<int32_t>(target);
      }

      if (distance & 3) {
         Loader_ReportError(
            "***RELOC ERROR {}: lower 2 bits must be zero before shifting.",
            -470022);
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 348);
         return -470037;
      }

      if (distance < 0 && (distance & 0xFE000000) != 0xFE000000) {
         Loader_ReportError(
            "***RELOC ERROR {}: upper 7 bits before shift must all be the same (1).",
            -470040);
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 359);
         return -470038;
      }

      if (distance >= 0 && (distance & 0xFE000000)) {
         Loader_ReportError(
            "***RELOC ERROR {}: upper 7 bits before shift must all be the same (0).",
            -470040);
         LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 371);
         return -470038;
      }

      *virt_cast<uint32_t *>(target) =
         (*virt_cast<uint32_t *>(target) & 0xFC000003) |
         (static_cast<uint32_t>(distance) & 0x3FFFFFC);
      break;
   }
   default:
      Loader_ReportError("***ERROR: Unsupported Relocation_Add Type ({}):", relaType);
      LiSetFatalError(0x18729Bu, fileType, 1, "LiCpu_RelocAdd", 462);
      return -470038;
   }

   return 0;
}

static int32_t
sExecReloc(virt_ptr<LOADED_RPL> rpl,
           bool isRpx,
           uint32_t sectionIndex,
           virt_ptr<rpl::SectionHeader> sectionHeader,
           virt_addr *preTrampBuffer,
           uint32_t *preTrampBufferSize,
           virt_addr *postTrampBuffer,
           uint32_t *postTrampBufferSize,
           virt_ptr<LiImportTracking> imports)
{
   if (sectionHeader->info >= rpl->elfHeader.shnum ||
       !rpl->sectionAddressBuffer[sectionHeader->info]) {
      return 0;
   }

   auto targetSectionBuffer = rpl->sectionAddressBuffer[sectionHeader->info];
   auto targetSectionVirtualAddress = getSectionHeader(rpl, sectionHeader->info)->addr;

   if (sectionHeader->link >= rpl->elfHeader.shnum ||
       !rpl->sectionAddressBuffer[sectionHeader->link]) {
      Loader_ReportError("*** relocations symbol table missing.");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 93);
      return -470003;
   }

   auto symbolSectionHeader = getSectionHeader(rpl, sectionHeader->link);
   auto symbolSectionEntSize = symbolSectionHeader->entsize;
   auto symbolSection = rpl->sectionAddressBuffer[sectionHeader->link];
   if (!symbolSectionEntSize) {
      symbolSectionEntSize = static_cast<uint32_t>(sizeof(rpl::Symbol));
   }

   auto symbolCount = symbolSectionHeader->size / symbolSectionEntSize;
   if (symbolCount == 0) {
      Loader_ReportError("*** relocations symbol table empty.");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 107);
      return -470004;
   }

   auto symbolStringTable = virt_ptr<char> { nullptr };
   if (symbolSectionHeader->link < rpl->elfHeader.shnum) {
      symbolStringTable = virt_cast<char *>(rpl->sectionAddressBuffer[symbolSectionHeader->link]);
   }

   auto relaEntSize = sectionHeader->entsize;
   auto relaSectionSize = sectionHeader->size;
   auto relaSectionAddress = rpl->sectionAddressBuffer[sectionIndex];
   if (sectionHeader->flags & rpl::SHF_DEFLATED) {
      relaSectionSize = *virt_cast<uint32_t *>(relaSectionAddress);
   }

   if (!relaEntSize) {
      relaEntSize = static_cast<uint32_t>(sizeof(rpl::Rela));
   } else if (relaEntSize != sizeof(rpl::Rela)) {
      Loader_ReportError(
         "*** sExecReloc: relocation section sh_entsize is not {}; err = {}",
         sizeof(rpl::Rela), -470052);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 144);
      return -470052;
   }

   auto relaCount = relaSectionSize / relaEntSize;
   if (relaSectionSize != relaCount * relaEntSize) {
      Loader_ReportError(
         "*** sExecReloc: relocation section size not a multiple of %d; err = %d.\n",
         sizeof(rpl::Rela), -470053);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 154);
      return -470053;
   }

   if (relaCount <= 0) {
      return 0;
   }

   auto stream = z_stream { };
   std::memset(&stream, 0, sizeof(stream));

   if (sectionHeader->flags & rpl::SHF_DEFLATED) {
      auto zlibError = inflateInit(&stream);
      if (zlibError != Z_OK) {
         switch (zlibError) {
         case Z_STREAM_ERROR:
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 186);
            Loader_ReportError("*** sExecReloc: could not initialize ZLIB uncompressor; ZLIB err = {}.",
                               Error::ZlibStreamError);
            return Error::ZlibStreamError;
         case Z_MEM_ERROR:
            LiSetFatalError(0x187298u, rpl->fileType, 0, "sExecReloc", 178);
            Loader_ReportError("*** sExecReloc: could not initialize ZLIB uncompressor; ZLIB err = {}.",
                               Error::ZlibMemError);
            return Error::ZlibMemError;
         case Z_VERSION_ERROR:
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 182);
            Loader_ReportError("*** sExecReloc: could not initialize ZLIB uncompressor; ZLIB err = {}.",
                               Error::ZlibVersionError);
            return Error::ZlibVersionError;
         default:
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 191);
            Loader_ReportError("***Unknown ZLIB error {} (0x{:08X}).", zlibError, zlibError);
            return Error::ZlibUnknownError;
         }
      }

      stream.avail_in = sectionHeader->size;
      stream.next_in = virt_cast<Bytef *>(relaSectionAddress + 4).get();
   }

   auto remainingBytes = relaSectionSize;
   auto remainingRela = relaCount;
   auto zlibError = Z_STREAM_END;

   while (remainingBytes > 0) {
      // Read whole shit
      auto availableBytes = remainingBytes;
      auto relas = virt_cast<rpl::Rela *>(relaSectionAddress).get();
      auto error = 0;

      if (sectionHeader->flags & rpl::SHF_DEFLATED) {
         availableBytes = std::min<uInt>(remainingBytes, static_cast<uInt>(sRelocBuffer.size()));
         stream.avail_out = availableBytes;
         stream.next_out = reinterpret_cast<Bytef *>(sRelocBuffer.data());

         LiCheckAndHandleInterrupts();
         zlibError = inflate(&stream, Z_NO_FLUSH);
         if (zlibError != Z_BUF_ERROR &&
            (zlibError < Z_OK || zlibError == Z_NEED_DICT)) {
            switch (zlibError) {
            case Z_MEM_ERROR:
               LiSetFatalError(0x187298u, rpl->fileType, 0, "sExecReloc", 236);
               error = Error::ZlibMemError;
               break;
            case Z_NEED_DICT:
               zlibError = Z_DATA_ERROR;
               // fallthrough
            case Z_DATA_ERROR:
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 240);
               error = Error::ZlibDataError;
               break;
            case Z_STREAM_ERROR:
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 244);
               error = Error::ZlibStreamError;
               break;
            default:
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 191);
               error = Error::ZlibUnknownError;
               break;
            }

            Loader_ReportError("*** sExecReloc: unable to uncompress relocation section; ZLIB err = {}.",
                               zlibError);
            inflateEnd(&stream);
            return error;
         }

         if (stream.avail_out) {
            Loader_ReportError("*** sExecReloc: ZLIB inflate's avail_out is {}, expected 0; err = {}.",
                               stream.avail_out, -470055);
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 254);
            inflateEnd(&stream);
            return -470055;
         }

         if (stream.total_out != sizeof(rpl::Rela) * (stream.total_out / sizeof(rpl::Rela))) {
            Loader_ReportError(
               "*** sExecReloc: inflate did not read a multiple of sizeof(ELF32_RELOC_ADD); err = {}.",
               -470055);
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 261);
            inflateEnd(&stream);
            return -470055;
         }

         relas = reinterpret_cast<rpl::Rela *>(sRelocBuffer.data());
      }

      auto availableRela = availableBytes / sizeof(rpl::Rela);
      for (auto relaIndex = 0u; relaIndex < availableRela; ++relaIndex) {
         auto &rela = relas[relaIndex];
         auto symbolIndex = rela.info >> 8;
         auto relaType = static_cast<rpl::RelocationType>(rela.info & 0xFF);

         LiCheckAndHandleInterrupts();

         if (symbolIndex > symbolCount) {
            Loader_ReportError(
               "***{} Rel invalid: rel ix {}, ref sym ix {}, max is {}. type {}",
               sectionIndex,
               relaIndex,
               symbolIndex,
               symbolCount,
               static_cast<int>(relaType));
            Loader_ReportError("*** r_info 0x{:08X}\n", rela.offset);
            Loader_ReportError("*** r_addend 0x{:08X}\n", rela.addend);
            Loader_ReportError("*** r_offset 0x{:08X}\n", rela.offset);
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 286);
            error = -470005;
            break;
         }

         auto symbol = virt_cast<rpl::Symbol *>(symbolSection + symbolIndex * symbolSectionEntSize);
         auto symbolValue = symbol->value;
         auto symbolBinding = symbol->info >> 4;
         auto symbolType = symbol->info & 0xf;

         if ((symbolValue & 0xFF000000) == 0xCD000000) {
            if (symbolBinding != rpl::STB_WEAK) {
               if (symbolStringTable) {
                  Loader_ReportError(
                     "***Rel invalid: sym ix {} val 0x{:08X} \"{}\"",
                     symbolIndex, symbolValue, symbolStringTable + symbol->name);
               } else {
                  Loader_ReportError(
                     "***Rel invalid: sym ix {} val 0x{:08X}",
                     symbolIndex, symbolValue);
               }

               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 308);
               error = -470006;
               break;
            } else {
               symbolValue = 0u;
            }
         }

         auto tlsModuleIndex = -1;
         if (relaType == rpl::R_PPC_DTPMOD32 || relaType == rpl::R_PPC_DTPREL32) {
            if (symbolType != rpl::STT_TLS) {
               Loader_ReportError("***TLS relocation used with non-TLS symbol 0x{:08X}", symbolValue);
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 349);
               error = -470065;
               break;
            }

            if (imports && imports[symbol->shndx].numExports) {
               tlsModuleIndex = imports[symbol->shndx].tlsModuleIndex;
               if (tlsModuleIndex == -1) {
                  Loader_ReportError("***TLS relocation used with non-TLS import module 0x{:08X}.", symbolValue);
                  LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 327);
                  error = -470101;
                  break;
               }
            } else {
               tlsModuleIndex = rpl->fileInfoBuffer->tlsModuleIndex;
               if (tlsModuleIndex == -1) {
                  Loader_ReportError("***TLS relocation used with non-TLS module 0x{:08X}.", symbolValue);
                  LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 339);
                  error = -470102;
                  break;
               }
            }
         }

         error = LiCpu_RelocAdd(isRpx,
                                targetSectionBuffer,
                                relaType,
                                symbolBinding == rpl::STB_WEAK,
                                rela.offset - targetSectionVirtualAddress,
                                rela.addend,
                                symbolValue,
                                preTrampBuffer,
                                preTrampBufferSize,
                                postTrampBuffer,
                                postTrampBufferSize,
                                tlsModuleIndex,
                                rpl->fileType);
         if (error) {
            break;
         }

         --remainingRela;
      }

      if (error) {
         if (sectionHeader->flags & rpl::SHF_DEFLATED) {
            inflateEnd(&stream);
         }

         return error;
      }

      remainingBytes -= availableBytes;
   }

   if (sectionHeader->flags & rpl::SHF_DEFLATED) {
      inflateEnd(&stream);
   }

   if (remainingRela) {
      Loader_ReportError(
         "*** sExecReloc: not all relocations were uncompressed; err = {}.",
         -470053);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 379);
      return -470053;
   }

   if (zlibError != Z_STREAM_END) {
      Loader_ReportError(
         "*** sExecReloc: streaming failure err (zlib_ret != Z_STREAM_END) = {}.",
         -470055);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sExecReloc", 404);
      return -470055;
   }

   LiCheckAndHandleInterrupts();
   return 0;
}

int32_t
LiFixupRelocOneRPL(virt_ptr<LOADED_RPL> rpl,
                   virt_ptr<LiImportTracking> imports,
                   uint32_t unk)
{
   LiCheckAndHandleInterrupts();

   // Apply imports to symbol table
   for (auto i = 1u; i < static_cast<unsigned int>(rpl->elfHeader.shnum - 2); ++i) {
      auto sectionHeader = getSectionHeader(rpl, i);
      if (sectionHeader->type == rpl::SHT_SYMTAB) {
         if (auto error = sFixupOneSymbolTable(rpl, i, sectionHeader, imports, unk)) {
            return error;
         }
      }
   }

   auto isRpx = rpl == getGlobalStorage()->loadedRpx;
   auto textAddress = virt_cast<virt_addr>(rpl->textBuffer);

   // Pre tramp buffer grows downawrds
   auto preTrampBufferEnd = (textAddress + rpl->fileInfoBuffer->trampAdjust) & 0xFFFFFFF0u;
   auto preTrampNext = preTrampBufferEnd - TrampSize;
   auto preTrampAvailable = static_cast<uint32_t>((preTrampBufferEnd - textAddress) / TrampSize);

   // Post tramp buffer grows upwards
   auto postTrampBufferSize = (textAddress + rpl->fileInfoBuffer->textSize) - rpl->postTrampBuffer;
   auto postTrampNext = virt_addr { rpl->postTrampBuffer };
   auto postTrampAvailable = static_cast<uint32_t>(postTrampBufferSize / TrampSize);

   auto textMax = rpl->postTrampBuffer + (postTrampAvailable * TrampSize);

   // Apply relocations
   for (auto i = 1u; i < static_cast<unsigned int>(rpl->elfHeader.shnum - 2); ++i) {
      auto sectionHeader = getSectionHeader(rpl, i);
      if (sectionHeader->type == rpl::SHT_RELA) {
         if (auto error = sExecReloc(rpl,
                                     isRpx,
                                     i,
                                     sectionHeader,
                                     &preTrampNext,
                                     &preTrampAvailable,
                                     &postTrampNext,
                                     &postTrampAvailable,
                                     imports)) {
            return error;
         }
      }
   }

   // Flush the code cache
   if (textAddress) {
      if (textMax - textAddress > rpl->textBufferSize) {
         Loader_ReportError("*** Fatal error: code overflowed its allocation.");
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiFixupRelocOneRPL", 695);
         return -470069;
      }

      LiSafeFlushCode(textAddress, rpl->textBufferSize);
   }

   // Relocate entry point
   rpl->entryPoint = virt_addr { 0 };

   for (auto i = 1u; i < static_cast<unsigned int>(rpl->elfHeader.shnum - 2); ++i) {
      auto sectionHeader = getSectionHeader(rpl, i);
      auto sectionAddress = rpl->sectionAddressBuffer[i];
      LiCheckAndHandleInterrupts();

      if (!sectionHeader->size || !sectionAddress) {
         continue;
      }

      if (sectionHeader->type != rpl::SHT_PROGBITS ||
          !(sectionHeader->flags & rpl::SHF_EXECINSTR)) {
         Loader_FlushDataRangeNoSync(sectionAddress, sectionHeader->size);
      }

      if (rpl->elfHeader.entry >= sectionHeader->addr &&
          rpl->elfHeader.entry < sectionHeader->addr + sectionHeader->size) {
         rpl->entryPoint = sectionAddress + (rpl->elfHeader.entry - sectionHeader->addr);
         break;
      }
   }

   if (!rpl->entryPoint) {
      Loader_ReportError("*** Could not find/relocate module entry point. this is required.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiFixupRelocOneRPL", 740);
      return -470092;
   }

   rpl->loadStateFlags &= ~LoaderStateFlag2;
   return 0;
}

} // namespace cafe::loader::internal
