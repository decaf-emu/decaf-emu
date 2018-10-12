#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_link.h"
#include "cafe_loader_log.h"
#include "cafe_loader_purge.h"
#include "cafe_loader_reloc.h"
#include "cafe_loader_query.h"
#include "cafe_loader_utils.h"

#include <algorithm>
#include <cctype>

namespace cafe::loader::internal
{

static virt_ptr<LOADED_RPL>
LiFindRPLByName(virt_ptr<char> name)
{
   char buffer[64];
   auto resolvedName = LiResolveModuleName(name.get());
   if (resolvedName.size() >= 64) {
      resolvedName = resolvedName.substr(0, 63);
   }

   std::transform(resolvedName.begin(), resolvedName.end(), buffer, ::tolower);
   buffer[resolvedName.size()] = 0;
   resolvedName = buffer;

   auto globals = getGlobalStorage();
   for (auto module = globals->firstLoadedRpl; module; module = module->nextLoadedRpl) {
      if (module->moduleNameLen != resolvedName.size()) {
         continue;
      }

      if (resolvedName.compare(module->moduleNameBuffer.get()) == 0) {
         return module;
      }
   }

   return nullptr;
}

static void
sReportCodeHeap(virt_ptr<GlobalStorage> globals,
                const char *msg)
{
}

static int32_t
sCheckOne(virt_ptr<LOADED_RPL> module)
{
   module->loadStateFlags |= LoaderStateFlag2;

   // Get the section header string table
   if (!module->elfHeader.shstrndx) {
      Loader_ReportError(
         "*** Error: Could not get section string table index for \"{}\".",
         module->moduleNameBuffer);
      LiSetFatalError(0x18729Bu, module->fileType, 1, "sCheckOne", 73);
      return -470071;
   }

   auto shStrAddr = module->sectionAddressBuffer[module->elfHeader.shstrndx];
   if (!shStrAddr) {
      Loader_ReportError(
         "*** Error: Could not get section string table for \"{}\".",
         module->moduleNameBuffer);
      LiSetFatalError(0x18729Bu, module->fileType, 1, "sCheckOne", 65);
      return -470072;
   }

   for (auto i = 0u; i < module->elfHeader.shnum; ++i) {
      auto sectionHeader = getSectionHeader(module, i);
      if (sectionHeader->type != rpl::SHT_RPL_IMPORTS) {
         continue;
      }

      auto name = virt_cast<char *>(shStrAddr + sectionHeader->name) + 9;
      auto rpl = LiFindRPLByName(name);
      if (!rpl) {
         Loader_ReportError("*** Imp Sec num {} @ 0x{}", i, sectionHeader);
         Loader_ReportError("*** Imp Name 0x{:02X} 0x{:02X} 0x{:02X} 0x{:02X}",
                            name[0], name[1], name[2], name[3]);
         Loader_ReportError(
            "*** Error. Could not find module \"{}\" during linking of \"{}\"!",
            name, module->moduleNameBuffer);
         LiSetFatalError(0x18729Bu, module->fileType, 1, "sCheckOne", 99);
         return -470021;
      }

      if (rpl->loadStateFlags & LoaderStateFlag2) {
         rpl->loadStateFlags |= LoaderStateFlag8;
         module->loadStateFlags |= LoaderStateFlag8;
      }

      if (!rpl->entryPoint && !(rpl->loadStateFlags & LoaderStateFlag8)) {
         auto error = sCheckOne(rpl);
         if (error) {
            return error;
         }
      }
   }

   module->loadStateFlags &= ~LoaderStateFlag2;
   return 0;
}

static int32_t
sCheckCircular(uint32_t numModules,
               virt_ptr<virt_ptr<LOADED_RPL>> modules,
               uint32_t checkIndex)
{
   for (auto i = 0u; i < numModules; ++i) {
      modules[i]->loadStateFlags &= ~LoaderStateFlag2;
   }

   auto result = sCheckOne(modules[checkIndex]);

   for (auto i = 0u; i < numModules; ++i) {
      modules[i]->loadStateFlags &= ~LoaderStateFlag2;
   }

   return result;
}

static int32_t
sValidateLinkData(virt_ptr<GlobalStorage> globals,
                  virt_ptr<LOADER_LinkInfo> linkInfo,
                  uint32_t linkInfoSize,
                  virt_ptr<virt_ptr<LOADED_RPL>> *outRplPointers,
                  uint32_t *outRplPointersAllocSize)
{
   if (auto error = LiValidateAddress(linkInfo,
                                      linkInfoSize, 3,
                                      -470022,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "link data")) {
      LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 165);
      return error;
   }

   if (linkInfoSize < sizeof(LOADER_LinkInfo)) {
      Loader_ReportError("*** invalid link data size.");
      LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 174);
      return -470020;
   }

   if (linkInfo->size != linkInfoSize) {
      Loader_ReportError("*** incorrect link data size.");
      LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 183);
      return -470020;
   }

   if (!linkInfo->numModules) {
      Loader_ReportError("*** incorrect # of modules being linked.");
      LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 191);
      return -470022;
   }

   if (sizeof(LOADER_LinkModule) * linkInfo->numModules + 8 != linkInfo->size) {
      Loader_ReportError("*** link data size does not match calculation.");
      LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 203);
      return -470023;
   }

   auto allocPtr = virt_ptr<void> { nullptr };
   auto allocSize = uint32_t { 0 };
   auto largestFree = uint32_t { 0 };
   if (auto error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                              4 * linkInfo->numModules,
                                              4,
                                              &allocPtr, 1, &allocSize,
                                              &largestFree,
                                              ios::mcp::MCPFileType::ProcessCode)) {
      Loader_ReportError(
         "*** memory allocation failed {} bytes, for list of LOADED_RPL pointers mNumModules = {};  (needed {}, available {}).",
         4 * linkInfo->numModules, linkInfo->numModules, allocSize, largestFree);
      LiSetFatalError(0x187298u, 0, 0, "sValidateLinkData", 214);
      return -470021;
   }

   // Generate the rpl pointer list
   auto rplPointers = virt_cast<virt_ptr<LOADED_RPL> *>(allocPtr);
   for (auto i = 0u; i < linkInfo->numModules; ++i) {
      auto rpl = virt_ptr<LOADED_RPL> { nullptr };
      if (!linkInfo->modules[i].loaderHandle) {
         rpl = globals->loadedRpx;
      } else {
         rpl = getModule(linkInfo->modules[i].loaderHandle);
         if (!rpl) {
            Loader_ReportError("*** Module with base {} not found in attempt to link.",
                               linkInfo->modules[i].loaderHandle);
            LiSetFatalError(0x18729Bu, 0, 1, "sValidateLinkData", 239);
            LiCacheLineCorrectFreeEx(globals->processCodeHeap, allocPtr, allocSize);
            return -470022;
         }
      }

      rplPointers[i] = rpl;

      if (rpl->loadStateFlags & LoaderStateFlags_Unk0x20000000) {
         continue;
      }

      if (!(rpl->loadStateFlags & LoaderSetup)) {
         Loader_ReportError("*** Module with base {} has not been set up.",
                            linkInfo->modules[i].loaderHandle);
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sValidateLinkData", 251);
         LiCacheLineCorrectFreeEx(globals->processCodeHeap, allocPtr, allocSize);
         return -470023;
      }

      if (rpl->entryPoint) {
         Loader_ReportError("*** Module with base 0x{:08X} has already been linked.",
                            linkInfo->modules[i].loaderHandle);
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sValidateLinkData", 259);
         LiCacheLineCorrectFreeEx(globals->processCodeHeap, allocPtr, allocSize);
         return - 470024;
      }
   }

   for (auto i = 0u; i < linkInfo->numModules; ++i) {
      if (rplPointers[i]->loadStateFlags & LoaderStateFlags_Unk0x20000000) {
         continue;
      }

      if (auto error = sCheckCircular(linkInfo->numModules, rplPointers, i)) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap, allocPtr, allocSize);
         return error;
      }
   }

   *outRplPointers = rplPointers;
   *outRplPointersAllocSize = allocSize;
   return 0;
}

static void
sSetLinkOutput(virt_ptr<LOADER_LinkInfo> linkInfo,
               virt_ptr<virt_ptr<LOADED_RPL>> linkModules)
{
   for (auto i = 0u; i < linkInfo->numModules; ++i) {
      auto &linkOutput = linkInfo->modules[i];
      auto &linkModule = linkModules[i];

      if (!linkModule->entryPoint) {
         Loader_Panic(0x130008,
                      "*** Linker trying to return successful output when a module ({}) is not linked!",
                      linkModule->moduleNameBuffer);
      }

      linkOutput.entryPoint = linkModule->entryPoint;
      linkOutput.textAddr = linkModule->textAddr;
      linkOutput.textOffset = linkModule->textOffset;
      linkOutput.textSize = linkModule->textSize;
      linkOutput.dataAddr = linkModule->dataAddr;
      linkOutput.dataOffset = linkModule->dataOffset;
      linkOutput.dataSize = linkModule->dataSize;
      linkOutput.loadAddr = linkModule->loadAddr;
      linkOutput.loadOffset = linkModule->loadOffset;
      linkOutput.loadSize = linkModule->loadSize;
   }
}

int32_t
LOADER_Link(kernel::UniqueProcessId upid,
            virt_ptr<LOADER_LinkInfo> linkInfo,
            uint32_t linkInfoSize,
            virt_ptr<LOADER_MinFileInfo> minFileInfo)
{
   auto error = int32_t { 0 };
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Link", 731);
      return Error::DifferentProcess;
   }

   auto linkModules = virt_ptr<virt_ptr<LOADED_RPL>> { nullptr };
   auto linkModulesAllocSize = uint32_t { 0 };
   if (error = sValidateLinkData(globals, linkInfo, linkInfoSize, &linkModules, &linkModulesAllocSize)) {
      Loader_ReportError("*** Link Data not valid.");
      sReportCodeHeap(globals, "link done");
      return error;
   }

   if (error = LiValidateMinFileInfo(minFileInfo, "LOADER_Link")) {
      sReportCodeHeap(globals, "link done");
      return error;
   }

   auto numUnlinkedModules = 0u;
   for (auto i = 0u; i < linkInfo->numModules; ++i) {
      if (!(linkModules[i]->loadStateFlags & LoaderStateFlags_Unk0x20000000)) {
         ++numUnlinkedModules;
      }
   }

   if (!numUnlinkedModules) {
      sSetLinkOutput(linkInfo, linkModules);
      LiCacheLineCorrectFreeEx(globals->processCodeHeap, linkModules, linkModulesAllocSize);
      sReportCodeHeap(globals, "link done");
      return 0;
   }

   auto allocSize = uint32_t { 0 };
   auto largestFree = uint32_t { 0 };
   auto allocPtr = virt_ptr<void> { nullptr };
   if (error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                         4 * numUnlinkedModules,
                                         4,
                                         &allocPtr, 1,
                                         &allocSize,
                                         &largestFree,
                                         ios::mcp::MCPFileType::ProcessCode)) {
      Loader_ReportError(
         "*** memory allocation failed {} bytes, for list of LOADED_RPL pointers actNumLink = {};  (needed {}, available {}).",
         4 * numUnlinkedModules,
         numUnlinkedModules,
         allocSize,
         largestFree);
      LiCacheLineCorrectFreeEx(globals->processCodeHeap, linkModules, linkModulesAllocSize);
      LiSetFatalError(0x187298u, 0, 0, "LOADER_Link", 773);
      return -470021;
   }

   auto unlinkedModules = virt_cast<virt_ptr<LOADED_RPL> *>(allocPtr);
   auto unlinkedModulesSize = allocSize;
   numUnlinkedModules = 0u;
   for (auto i = 0u; i < linkInfo->numModules; ++i) {
      if (!(linkModules[i]->loadStateFlags & LoaderStateFlags_Unk0x20000000)) {
         unlinkedModules[numUnlinkedModules] = linkModules[i];
         unlinkedModules[numUnlinkedModules]->loadStateFlags |= LoaderStateFlag2;
         ++numUnlinkedModules;
      }
   }

   sReportCodeHeap(globals, "fixup start");

   auto maxShnum = 0u;
   auto numLoadStateFlag8 = 0u;
   auto importTracking = virt_ptr<LiImportTracking> { nullptr };
   auto importTrackingSize = uint32_t { 0 };
   auto remainingUnlinkedModules = 0u;
   auto unlinkedModuleIndex = 0u;

   for (auto i = 0u; i < numUnlinkedModules; ++i) {
      if (unlinkedModules[i]->elfHeader.shnum > maxShnum) {
         maxShnum = unlinkedModules[i]->elfHeader.shnum;
      }

      if (unlinkedModules[i]->loadStateFlags & LoaderStateFlag8) {
         numLoadStateFlag8++;
      }
   }

   // Allocate import tracking
   if (error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                         sizeof(LiImportTracking) * maxShnum,
                                         4,
                                         &allocPtr,
                                         1,
                                         &allocSize,
                                         &largestFree,
                                         ios::mcp::MCPFileType::ProcessCode)) {
      Loader_ReportError(
         "*** Could not allocate space for largest # of sections ({});  (needed {}, available {}).",
         maxShnum, allocSize, largestFree);
      goto out;
   }

   importTracking = virt_cast<LiImportTracking *>(allocPtr);
   importTrackingSize = allocSize;

   if (numLoadStateFlag8 != 0) {
      for (auto i = 0u; i < numUnlinkedModules; ++i) {
         LiCheckAndHandleInterrupts();
         auto module = unlinkedModules[i];
         if (module->loadStateFlags & LoaderStateFlag8) {
            if (!module->elfHeader.shstrndx) {
               Loader_ReportError(
                  "*** Error: Could not get section string table index for \"{}\".",
                  module->moduleNameBuffer);
               LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 839);
               error = -470071;
               goto out;
            }

            auto shStrAddr = module->sectionAddressBuffer[module->elfHeader.shstrndx];
            if (!shStrAddr) {
               Loader_ReportError(
                  "*** Error: Could not get section string table for \"{}\".",
                  module->moduleNameBuffer);
               LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 831);
               error = -470072;
               goto out;
            }

            for (auto j = 1u; j < module->elfHeader.shnum; ++j) {
               auto sectionHeader = getSectionHeader(module, j);
               if (sectionHeader->type == rpl::SHT_RPL_IMPORTS) {
                  auto name = virt_cast<char *>(shStrAddr + sectionHeader->name) + 9;
                  auto rpl = LiFindRPLByName(name);
                  if (!rpl) {
                     Loader_ReportError("*** Error: Could not get imported RPL name.");
                     LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 854);
                     error = -470073;
                     goto out;
                  }

                  if ((rpl->loadStateFlags & LoaderStateFlag2) &&
                      (rpl->loadStateFlags & LoaderStateFlag8)) {
                     if (error = LiFixupRelocOneRPL(rpl, nullptr, 1)) {
                        Loader_ReportError(
                           "*** Error. Could not find module \"{}\" during linking!",
                           name);
                        LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 863);
                        error = -470010;
                        goto out;
                     }
                  }
               }
            }
         }
      }
   }

   // PLEASE SOMEONE TELL ME WHAT THIS LOAD STATE FLAGS MEANS!!
   for (auto i = 0u; i < numUnlinkedModules; ++i) {
      unlinkedModules[i]->loadStateFlags |= LoaderStateFlag2;
   }

   // Loop through trying to link everything
   remainingUnlinkedModules = numUnlinkedModules;
   unlinkedModuleIndex = 0u;

   while (remainingUnlinkedModules > 0) {
      LiCheckAndHandleInterrupts();
      auto module = unlinkedModules[unlinkedModuleIndex];

      if (module->loadStateFlags & LoaderStateFlag2) {
         std::memset(importTracking.get(), 0,
                     sizeof(LiImportTracking) * module->elfHeader.shnum);

         if (!module->elfHeader.shstrndx) {
            Loader_ReportError(
               "*** Error: Could not get section string table index for \"{}\".",
               module->moduleNameBuffer);
            LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 927);
            error = -470071;
            goto out;
         }

         auto shStrAddr = module->sectionAddressBuffer[module->elfHeader.shstrndx];
         if (!shStrAddr) {
            Loader_ReportError(
               "*** Error: Could not get section string table for \"{}\".",
               module->moduleNameBuffer);
            LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 919);
            error = -470072;
            goto out;
         }

         auto j = 1u;
         for (; j < module->elfHeader.shnum; ++j) {
            auto sectionHeader = getSectionHeader(module, j);
            if (sectionHeader->type == rpl::SHT_RPL_IMPORTS) {
               auto name = virt_cast<char *>(shStrAddr + sectionHeader->name) + 9;
               auto rpl = LiFindRPLByName(name);
               if (!rpl) {
                  Loader_ReportError(
                     "*** Error. Could not find module \"{}\" during linking!",
                     name);
                  LiSetFatalError(0x18729Bu, module->fileType, 1, "LOADER_Link", 950);
                  error = -470010;
                  goto out;
               }

               if ((rpl->loadStateFlags & LoaderStateFlag2) &&
                   !(rpl->loadStateFlags & LoaderStateFlag8)) {
                  break;
               }

               if (sectionHeader->flags & rpl::SHF_EXECINSTR) {
                  importTracking[j].numExports = rpl->numFuncExports;
                  importTracking[j].exports = virt_cast<rpl::Export *>(rpl->funcExports);
               } else {
                  importTracking[j].numExports = rpl->numDataExports;
                  importTracking[j].exports = virt_cast<rpl::Export *>(rpl->dataExports);
               }

               importTracking[j].tlsModuleIndex = rpl->fileInfoBuffer->tlsModuleIndex;
               importTracking[j].rpl = rpl;
            }
         }

         if (j == module->elfHeader.shnum) {
            auto unk = (module->loadStateFlags & LoaderStateFlag8) ? 2 : 0;
            if (error = LiFixupRelocOneRPL(module, importTracking, unk)) {
               break;
            }
         }

         --remainingUnlinkedModules;
      }

      unlinkedModuleIndex++;
      if (unlinkedModuleIndex >= numUnlinkedModules) {
         unlinkedModuleIndex = 0;
      }
   }

   sReportCodeHeap(globals, "fixup done");
   LiCacheLineCorrectFreeEx(globals->processCodeHeap, importTracking,
                            importTrackingSize);

out:
   // PLEASE SOMEONE TELL ME WHAT THIS LOAD STATE FLAGS MEANS!!
   for (auto i = 0u; i < numUnlinkedModules; ++i) {
      unlinkedModules[i]->loadStateFlags &= ~LoaderStateFlag8;
   }

   if (error) {
      // Purge all unlinked modules
      for (auto i = 0u; i < numUnlinkedModules; ++i) {
         auto module = virt_ptr<LOADED_RPL> { nullptr };
         auto prev = virt_ptr<LOADED_RPL> { nullptr };

         for (module = globals->firstLoadedRpl; module; module = module->nextLoadedRpl) {
            if (module == unlinkedModules[i]) {
               break;
            }

            prev = module;
         }

         if (!module) {
            Loader_ReportError("**** Module disappeared while being linked!");
         } else if (!(module->loadStateFlags & LoaderStateFlag4)) {
            if (!module->nextLoadedRpl) {
               globals->lastLoadedRpl = prev;
            }

            if (prev) {
               prev->nextLoadedRpl = module->nextLoadedRpl;
            }

            LiPurgeOneUnlinkedModule(module);
         }
      }
   } else {
      sSetLinkOutput(linkInfo, linkModules);
   }

   for (auto i = 0u; i < numUnlinkedModules; ++i) {
      auto module = unlinkedModules[unlinkedModuleIndex];
      if (!module) {
         continue;
      }

      if (module->compressedRelocationsBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  module->compressedRelocationsBuffer,
                                  module->compressedRelocationsBufferSize);
         module->compressedRelocationsBuffer = nullptr;
      }

      if (module->crcBuffer && module->crcBufferSize) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  module->crcBuffer,
                                  module->crcBufferSize);
         module->crcBuffer = nullptr;
         module->crcBufferSize = 0u;
         module->sectionAddressBuffer[module->elfHeader.shnum - 2] = virt_addr { 0 };
      }
   }

   LiCacheLineCorrectFreeEx(globals->processCodeHeap, linkModules, linkModulesAllocSize);
   LiCacheLineCorrectFreeEx(globals->processCodeHeap, unlinkedModules, unlinkedModulesSize);
   sReportCodeHeap(globals, "link done");
   return 0;
}

} // namespace cafe::loader::internal
