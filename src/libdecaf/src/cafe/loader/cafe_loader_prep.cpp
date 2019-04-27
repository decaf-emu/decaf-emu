#include "cafe_loader_bounce.h"
#include "cafe_loader_entry.h"
#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_basics.h"
#include "cafe_loader_purge.h"
#include "cafe_loader_prep.h"
#include "cafe_loader_shared.h"
#include "cafe_loader_log.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe_loader_utils.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel_processid.h"

#include <common/strutils.h>

namespace cafe::loader::internal
{

constexpr auto MaxModuleNameLen = 0x3Bu;

int32_t
LiLoadForPrep(virt_ptr<char> moduleName,
              uint32_t moduleNameLen,
              virt_ptr<void> chunkBuffer,
              virt_ptr<LOADED_RPL> *outLoadedRpl,
              LiBasicsLoadArgs *loadArgs,
              uint32_t unk)
{
   auto globals = getGlobalStorage();
   auto rpl = virt_ptr<LOADED_RPL> { nullptr };
   auto error = LiLoadRPLBasics(moduleName,
                                moduleNameLen,
                                chunkBuffer,
                                globals->processCodeHeap,
                                globals->processCodeHeap,
                                true,
                                0,
                                &rpl,
                                loadArgs,
                                unk);
   if (error) {
      return error;
   }

   auto fileInfo = rpl->fileInfoBuffer;
   if (globals->loadedRpx && (fileInfo->flags & rpl::RPL_IS_RPX)) {
      Loader_ReportError("***Attempt to load RPX when main program already loaded.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiLoadForPrep", 1175);
      error = -470093;
   } else if (!globals->loadedRpx && !(fileInfo->flags & rpl::RPL_IS_RPX)) {
      Loader_ReportError("***Attempt to load non-RPX as main program.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiLoadForPrep", 1183);
      error = -470094;
   } else {
      rpl->nextLoadedRpl = nullptr;
      if (globals->lastLoadedRpl) {
         globals->lastLoadedRpl->nextLoadedRpl = rpl;
         globals->lastLoadedRpl = rpl;
      } else {
         globals->firstLoadedRpl = rpl;
         globals->lastLoadedRpl = rpl;
      }

      *outLoadedRpl = rpl;
      return 0;
   }

   if (rpl) {
      LiPurgeOneUnlinkedModule(rpl);
   }

   return error;
}

int32_t
LOADER_Prep(kernel::UniqueProcessId upid,
            virt_ptr<LOADER_MinFileInfo> minFileInfo)
{
   auto globals = getGlobalStorage();
   auto error = 0;

   LiCheckAndHandleInterrupts();

   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Prep", 1262);
      LiCloseBufferIfError();
      return Error::DifferentProcess;
   }

   if (minFileInfo) {
      if (auto error = LiValidateMinFileInfo(minFileInfo, "LOADER_Prep")) {
         LiCloseBufferIfError();
         return error;
      }
   }

   *minFileInfo->outKernelHandle = nullptr;

   LiResolveModuleName(virt_addrof(minFileInfo->moduleNameBuffer),
                       virt_addrof(minFileInfo->moduleNameBufferLen));
   auto moduleName =
      std::string_view {
         minFileInfo->moduleNameBuffer.get(),
         minFileInfo->moduleNameBufferLen
      };

   if (minFileInfo->moduleNameBufferLen == 8 &&
       strncmp(minFileInfo->moduleNameBuffer.get(), "coreinit", 8) == 0) {
      Loader_ReportError("*** Loader Failure (system module re-load).");
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Prep", 1305);
      LiCloseBufferIfError();
      return -470029;
   }

   for (auto itr = globals->firstLoadedRpl; itr; itr = itr->nextLoadedRpl) {
      if (itr->moduleNameLen == minFileInfo->moduleNameBufferLen &&
          strncmp(itr->moduleNameBuffer.get(),
                  minFileInfo->moduleNameBuffer.get(),
                  minFileInfo->moduleNameBufferLen) == 0) {
         Loader_ReportError("*** module \"{}\" already loaded.\n",
            std::string_view { minFileInfo->moduleNameBuffer.get(),
                               minFileInfo->moduleNameBufferLen });
         LiSetFatalError(0x18729Bu, itr->fileType, 1, "LOADER_Prep", 1292);
         LiCloseBufferIfError();
         return -470028;
      }
   }

   // Check if module already loaded as a shared library
   if (!getProcFlags().disableSharedLibraries()) {
      auto sharedModule = findLoadedSharedModule(moduleName);
      if (sharedModule) {
         auto allocPtr = virt_ptr<void> { nullptr };
         auto allocSize = uint32_t { 0 };
         auto largestFree = uint32_t { 0 };
         error = LiCacheLineCorrectAllocEx(getGlobalStorage()->processCodeHeap,
                                           sizeof(LOADED_RPL),
                                           4,
                                           &allocPtr,
                                           1,
                                           &allocSize,
                                           &largestFree,
                                           sharedModule->fileType);
         if (error) {
            Loader_ReportError(
               "***Allocate Error {}, Failed to allocate {} bytes for system shared RPL tracking for {} in current process;  (needed {}, available {}).",
               error, allocSize, moduleName, allocSize, largestFree);
            LiCloseBufferIfError();
            return error;
         }

         auto trackingModule = virt_cast<LOADED_RPL *>(allocPtr);
         *trackingModule = *sharedModule;
         trackingModule->selfBufferSize = allocSize;
         trackingModule->globals = globals;
         trackingModule->loadStateFlags &= ~LoaderStateFlag4;

         // Add to global loaded module linked list
         trackingModule->nextLoadedRpl = nullptr;
         if (globals->lastLoadedRpl) {
            globals->lastLoadedRpl->nextLoadedRpl = trackingModule;
         } else {
            globals->firstLoadedRpl = trackingModule;
         }
         globals->lastLoadedRpl = trackingModule;

         *minFileInfo->outKernelHandle = trackingModule->moduleNameBuffer;
         *minFileInfo->outNumberOfSections = trackingModule->elfHeader.shnum;

         error = LiGetMinFileInfo(trackingModule, minFileInfo);
         if (error) {
            LiCloseBufferIfError();
         }

         return error;
      }
   }

   auto filename = StackArray<char, 64> { };
   auto filenameLen = std::min<uint32_t>(minFileInfo->moduleNameBufferLen, 59);
   std::memcpy(filename.get(),
               minFileInfo->moduleNameBuffer.get(),
               filenameLen);
   string_copy(filename.get() + filenameLen,
               ".rpl",
               filename.size() - filenameLen);

   LiCheckAndHandleInterrupts();
   LiInitBuffer(false);

   auto chunkBuffer = virt_ptr<void> { nullptr };
   auto chunkBufferSize = uint32_t { 0 };
   error = LiBounceOneChunk(filename.get(),
                            ios::mcp::MCPFileType::CafeOS,
                            globals->currentUpid,
                            &chunkBufferSize,
                            0, 1,
                            &chunkBuffer);
   LiCheckAndHandleInterrupts();
   if (error) {
      Loader_ReportError(
         "***LiBounceOneChunk failed loading \"{}\" of type {} at offset 0x{:08X} err={}.",
         filename.get(), 1, 0, error);
      LiCloseBufferIfError();
      return error;
   }

   auto loadArgs = LiBasicsLoadArgs { };
   auto rpl = virt_ptr<LOADED_RPL> { nullptr };
   loadArgs.upid = upid;
   loadArgs.loadedRpl = nullptr;
   loadArgs.readHeapTracking = globals->processCodeHeap;
   loadArgs.pathNameLen = filenameLen + 5;
   loadArgs.pathName = filename;
   loadArgs.fileType = ios::mcp::MCPFileType::CafeOS;
   loadArgs.chunkBuffer = chunkBuffer;
   loadArgs.chunkBufferSize = chunkBufferSize;
   loadArgs.fileOffset = 0u;

   error = LiLoadForPrep(filename,
                         filenameLen,
                         chunkBuffer,
                         &rpl,
                         &loadArgs,
                         0);
   if (error) {
      Loader_ReportError("***LiLoadForPrep failure {}. loading \"{}\".",
                         error, filename.get());
      LiCloseBufferIfError();
      return error;
   }

   *minFileInfo->outKernelHandle = rpl->moduleNameBuffer;
   *minFileInfo->outNumberOfSections = rpl->elfHeader.shnum;
   error = LiGetMinFileInfo(rpl, minFileInfo);
   if (error) {
      LiCloseBufferIfError();
   }

   return error;
}

} // namespace cafe::loader::internal
