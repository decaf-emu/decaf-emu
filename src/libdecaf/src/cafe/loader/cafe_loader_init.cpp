#include "cafe_loader_basics.h"
#include "cafe_loader_bounce.h"
#include "cafe_loader_entry.h"
#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_ipcldriver.h"
#include "cafe_loader_prep.h"
#include "cafe_loader_setup.h"
#include "cafe_loader_shared.h"
#include "cafe_loader_log.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe_loader_utils.h"
#include "cafe/cafe_stackobject.h"

namespace cafe::loader::internal
{

static uint32_t sDataAreaSize = 0;
static uint32_t sNumDataAreaAllocations = 0;
static bool sIPCLInitialised[3] = { false, false, false };

static int32_t
LiLoadCoreIntoProcess(virt_ptr<RPL_STARTINFO> startInfo)
{
   auto error = 0;
   auto globals = getGlobalStorage();
   Loader_LogEntry(2, 1, 512, "LiLoadCoreIntoProcess");

   if (!getProcFlags().disableSharedLibraries()) {
      auto sharedModule = findLoadedSharedModule("coreinit");
      if (!sharedModule) {
         Loader_ReportError("***Failed to find shared coreinit on module list.");
         LiSetFatalError(0x18729Bu, 0, 1, "LiLoadCoreIntoProcess", 161);
         Loader_LogEntry(2, 1, 1024, "LiLoadCoreIntoProcess");
         return -470010;
      }

      auto allocPtr = virt_ptr<void> { nullptr };
      auto allocSize = uint32_t { 0 };
      auto largestFree = uint32_t { 0 };
      error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                        sizeof(LOADED_RPL),
                                        4,
                                        &allocPtr,
                                        1,
                                        &allocSize,
                                        &largestFree,
                                        sharedModule->fileType);
      if (error) {
         Loader_ReportError(
            "***Failed to allocate system coreinit RPL tracking for process;  (needed {}, available {}).\n",
            allocSize, largestFree);
         Loader_LogEntry(2, 1, 1024, "LiLoadCoreIntoProcess");
         return error;
      }

      auto trackingModule = virt_cast<LOADED_RPL *>(allocPtr);
      *trackingModule = *sharedModule;
      trackingModule->selfBufferSize = allocSize;
      trackingModule->globals = globals;

      // Add to global loaded module linked list
      trackingModule->nextLoadedRpl = nullptr;
      if (globals->lastLoadedRpl) {
         globals->lastLoadedRpl->nextLoadedRpl = trackingModule;
      } else {
         globals->firstLoadedRpl = trackingModule;
      }
      globals->lastLoadedRpl = trackingModule;

      startInfo->coreinit = trackingModule;
   } else {
      decaf_abort("Unimplemented LiLoadCoreIntoProcess for disableSharedLibraries");
   }

   Loader_LogEntry(2, 1, 1024, "LiLoadCoreIntoProcess");
   return error;
}

int32_t
LOADER_Init(kernel::UniqueProcessId upid,
            uint32_t numCodeAreaHeapBlocks,
            uint32_t maxCodeSize,
            uint32_t maxDataSize,
            virt_ptr<virt_ptr<LOADED_RPL>> outLoadedRpx,
            virt_ptr<virt_ptr<LOADED_RPL>> outModuleList,
            virt_ptr<RPL_STARTINFO> startInfo)
{
   auto trackingBlockSize = align_up(TinyHeapBlockSize * numCodeAreaHeapBlocks + TinyHeapHeaderSize, 0x40);
   auto globals = getGlobalStorage();
   auto error = 0;
   auto chunkBufferSize = uint32_t { 0 };
   auto chunkBuffer = virt_ptr<void> { nullptr };
   auto loadRpxName = std::string_view { };
   auto loadFileType = ios::mcp::MCPFileType::CafeOS;
   auto fileName = StackArray<char, 64> { };
   auto moduleName = StackArray<char, 64> { };
   auto fileNameLen = uint32_t { 0 };
   auto loadArgs = LiBasicsLoadArgs { };
   auto rpx = virt_ptr<LOADED_RPL> { nullptr };
   auto fileInfo = virt_ptr<rpl::RPLFileInfo_v4_2> { nullptr };
   auto dataArea = virt_addr { 0 };

   sDataAreaSize = 0;
   sNumDataAreaAllocations = 0;
   maxCodeSize = std::min(maxCodeSize, 0x0E000000u);

   globals->numCodeAreaHeapBlocks = numCodeAreaHeapBlocks;
   globals->currentUpid = upid;
   globals->processCodeHeapTrackingBlockSize = trackingBlockSize;
   globals->maxCodeSize = maxCodeSize;
   globals->maxDataSize = maxDataSize;
   globals->availableCodeSize = maxCodeSize - trackingBlockSize;
   globals->userHasControl = FALSE;
   globals->firstLoadedRpl = nullptr;
   globals->lastLoadedRpl = nullptr;
   globals->loadedRpx = nullptr;

   std::memset(startInfo.getRawPointer(), 0, sizeof(RPL_STARTINFO));
   startInfo->dataAreaEnd = virt_addr { 0x10000000u } + maxDataSize;

   // Setup code heap
   globals->processCodeHeap = virt_cast<TinyHeap *>(virt_addr { 0x10000000 - trackingBlockSize });

   auto heapError =
      TinyHeap_Setup(globals->processCodeHeap,
                     globals->processCodeHeapTrackingBlockSize,
                     virt_cast<void *>(virt_addr { 0x10000000 - globals->availableCodeSize - trackingBlockSize }),
                     globals->availableCodeSize);
   if (heapError != TinyHeapError::OK) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Init", 204);
      Loader_ReportError("*** Process code heap setup failed.\n");
      goto error;
   }

   if (!sIPCLInitialised[cpu::this_core::id()]) {
      IPCLDriver_Init();
      IPCLDriver_Open();
      LiInitIopInterface();
   }

   error = LiInitSharedForProcess(startInfo);
   Loader_LogEntry(2, 1, 0, "LOADER_Init  LoadINIT ShrForPro ret={}.", error);
   if (error) {
      Loader_ReportError("*** Process shared resource init failure.");
      goto error;
   }

   LiInitBuffer(true);

   if (upid == kernel::UniqueProcessId::Root) {
      loadRpxName = std::string_view { "root.rpx" };
      loadFileType = ios::mcp::MCPFileType::CafeOS;
   } else {
      loadRpxName = getLoadRpxName().getRawPointer();
      loadFileType = ios::mcp::MCPFileType::ProcessCode;
   }

   error = LiBounceOneChunk(loadRpxName, loadFileType, upid, &chunkBufferSize, 0, 1, &chunkBuffer);
   Loader_LogEntry(2, 1, 0,
      "LOADER_Init  LoadINIT BncLoad ret, pName={}, mProcId={}, &pLoaded={}, err={}.",
         loadRpxName, static_cast<int>(upid), chunkBuffer, error);

   if (error) {
      Loader_ReportError(
         "***LiBounceOneChunk failed loading \"{}\" of type {} at offset 0x{:08X} err={}.",
         loadRpxName,
         loadFileType,
         0,
         error);
      goto error;
   }

   fileNameLen = std::min<uint32_t>(static_cast<uint32_t>(loadRpxName.size()), 59);
   std::memcpy(fileName.getRawPointer(), loadRpxName.data(), fileNameLen);

   // Resolve module name and copy to guest stack buffer
   loadRpxName = LiResolveModuleName(loadRpxName);
   std::memcpy(moduleName.getRawPointer(), loadRpxName.data(), loadRpxName.size());
   moduleName[loadRpxName.size()] = char { 0 };

   loadArgs.upid = upid;
   loadArgs.loadedRpl = nullptr;
   loadArgs.readHeapTracking = globals->processCodeHeap;
   loadArgs.pathNameLen = fileNameLen;
   loadArgs.pathName = fileName;
   loadArgs.fileType = loadFileType;
   loadArgs.chunkBuffer = chunkBuffer;
   loadArgs.chunkBufferSize = chunkBufferSize;
   loadArgs.fileOffset = 0u;

   error = LiLoadForPrep(moduleName,
                         loadRpxName.size(),
                         chunkBuffer,
                         &rpx,
                         &loadArgs,
                         0);
   if (error) {
      Loader_ReportError("***LiLoadForPrep() failed with err={}.", error);
      goto error;
   }

   globals->loadedRpx = rpx;
   rpx->loadStateFlags |= LoadStateFlags::LoaderStateFlag4;

   // Allocate data buffer
   dataArea = virt_addr { startInfo->dataAreaStart };
   fileInfo = rpx->fileInfoBuffer;
   if (fileInfo->dataSize) {
      dataArea = align_up(dataArea, fileInfo->dataAlign);
      rpx->dataBuffer = virt_cast<void *>(dataArea);
      dataArea = align_up(dataArea + fileInfo->dataSize, 64);
      ++sNumDataAreaAllocations;
   }

   // Allocate load buffer
   if (fileInfo->loadSize != fileInfo->fileInfoPad) {
      dataArea = align_up(dataArea, fileInfo->loadAlign);
      rpx->loadBuffer = virt_cast<void *>(dataArea);
      dataArea = align_up(dataArea + fileInfo->loadSize - fileInfo->fileInfoPad, 64);
      ++sNumDataAreaAllocations;
   }

   sDataAreaSize += static_cast<uint32_t>(dataArea - startInfo->dataAreaStart);
   if (dataArea < virt_addr { 0x10000000 } || dataArea >= startInfo->dataAreaEnd) {
      Loader_ReportError(
         "*** Insufficient data area space in process. end @ {}, areaend @ {}",
         dataArea, startInfo->dataAreaEnd);
      LiSetFatalError(0x18729Bu, rpx->fileType, 1, "LOADER_Init", 338);
      error = -470019;
      goto error;
   }

   startInfo->dataAreaStart = dataArea;
   if (upid == kernel::UniqueProcessId::Root) {
      startInfo->systemHeapSize = 0x4000u;
      startInfo->stackSize = 0x4000u;
   } else {
      if (fileInfo->stackSize) {
         startInfo->stackSize = fileInfo->stackSize;
      } else {
         startInfo->stackSize = 0x10000u;
      }

      startInfo->systemHeapSize = std::max<uint32_t>(fileInfo->heapSize, 0x8000u);
   }

   error = LiSetupOneRPL(upid, rpx, globals->processCodeHeap, globals->processCodeHeap);
   if (error) {
      goto error;
   }

   Loader_LogEntry(2, 1, 0,
      "LOADER_Init  LoadINIT Setup1RPL ret, mpName={}, aProcId={}, mpCodeHeapTracking={}, err={}.",
      rpx->moduleNameBuffer,
      static_cast<int>(upid),
      globals->processCodeHeap,
      0);

   if ((startInfo->dataAreaEnd - startInfo->dataAreaStart) < startInfo->systemHeapSize) {
      Loader_ReportError("***Insufficient space for stacks and system heap in process to start it.\n");
      LiSetFatalError(0x18729Bu, rpx->fileType, 1, "LOADER_Init", 375);
      error = -470020;
      goto error;
   }

   // Relocate sda base
   globals->sdaBase = 0u;
   globals->sda2Base = 0u;

   for (auto i = 1u; i < rpx->elfHeader.shnum; ++i) {
      auto sectionHeader = getSectionHeader(rpx, i);
      auto sectionAddress = rpx->sectionAddressBuffer[i];
      if (!sectionHeader->size ||
          !sectionAddress ||
          (sectionHeader->flags & rpl::SHF_EXECINSTR)) {
         continue;
      }

      if ((fileInfo->sdaBase - 0x8000) >= sectionHeader->addr &&
         (fileInfo->sdaBase - 0x8000) < (sectionHeader->addr + sectionHeader->size)) {
         globals->sdaBase = sectionAddress + fileInfo->sdaBase - sectionHeader->addr;
      }

      if ((fileInfo->sda2Base - 0x8000) >= sectionHeader->addr &&
         (fileInfo->sda2Base - 0x8000) < (sectionHeader->addr + sectionHeader->size)) {
         globals->sda2Base = sectionAddress + fileInfo->sda2Base - sectionHeader->addr;
      }
   }

   if (!globals->sdaBase) {
      globals->sdaBase = virt_cast<virt_addr>(rpx->dataBuffer) + 0x8000u;
   }

   if (!globals->sda2Base) {
      globals->sda2Base = virt_cast<virt_addr>(rpx->loadBuffer) + 0x8000u;
   }

   startInfo->sdaBase = globals->sdaBase;
   startInfo->sda2Base = globals->sda2Base;

   error = LiLoadCoreIntoProcess(startInfo);
   Loader_LogEntry(2, 1, 0,
      "LOADER_Init  LoadINIT LdCrIntoPc ret, mpName={}, aProcId={}, mpCodeHeapTracking={}, err={}.",
      rpx->moduleNameBuffer, static_cast<int>(upid), globals->processCodeHeap, error);
   if (error) {
      goto error;
   }

   if (!startInfo->coreinit->entryPoint) {
      Loader_ReportError("***Error: No main program entrypoint found.");
      LiSetFatalError(0x18729Bu, rpx->fileType, 1, "LOADER_Init", 432);
      error = -470080;
      goto error;
   }

   startInfo->entryPoint = startInfo->coreinit->entryPoint;
   startInfo->unk0x10 = 0u;
   *outLoadedRpx = rpx;
   *outModuleList = globals->firstLoadedRpl;
   Loader_LogEntry(2, 1, 1024, "LOADER_Init");

   for (auto module = globals->firstLoadedRpl; module; module = module->nextLoadedRpl) {
      gLog->debug("Loaded module {}", module->moduleNameBuffer);
      for (auto i = 0u; i < module->elfHeader.shnum; ++i) {
         auto size = 0u;
         if (module->sectionHeaderBuffer) {
            size = getSectionHeader(module, i)->size;
         }
         gLog->debug("  Section {}, {} - {}",
                     i,
                     module->sectionAddressBuffer[i],
                     module->sectionAddressBuffer[i] + size);
      }
   }
   return 0;

error:
   globals->currentUpid = kernel::UniqueProcessId::Kernel;
   LiCloseBufferIfError();
   Loader_LogEntry(2, 1, 1024, "LOADER_Init");
   return error;
}

} // namespace cafe::loader::internal
