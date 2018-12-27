#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_dynload.h"
#include "coreinit_ghs.h"
#include "coreinit_handle.h"
#include "coreinit_mutex.h"
#include "coreinit_memheap.h"
#include "coreinit_osreport.h"
#include "coreinit_systemheap.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle.h"
#include "cafe/loader/cafe_loader_rpl.h"
#include "cafe/kernel/cafe_kernel_info.h"
#include "cafe/kernel/cafe_kernel_loader.h"
#include "cafe/kernel/cafe_kernel_mmu.h"
#include "debug_api/debug_api_controller.h"

#include <common/strutils.h>

namespace cafe::coreinit
{

using OSDynLoad_RplEntryFn = virt_func_ptr<
   int32_t (OSDynLoad_ModuleHandle moduleHandle,
            OSDynLoad_EntryReason reason)>;

using OSDynLoad_InitDefaultHeapFn = virt_func_ptr<
   void(virt_ptr<MEMHeapHandle> mem1,
        virt_ptr<MEMHeapHandle> foreground,
        virt_ptr<MEMHeapHandle> mem2)>;

constexpr auto MaxTlsModuleIndex = uint32_t { 0x7fff };

struct RPL_DATA
{
   be2_val<OSHandle> handle;
   be2_virt_ptr<void> loaderHandle;
   be2_virt_ptr<char> moduleName;
   be2_val<uint32_t> moduleNameLen;
   be2_val<uint32_t> sectionInfoCount;
   be2_virt_ptr<loader::LOADER_SectionInfo> sectionInfo;
   be2_virt_ptr<virt_ptr<RPL_DATA>> importModules;
   be2_val<uint32_t> importModuleCount;
   be2_val<uint32_t> userFileInfoSize;
   be2_virt_ptr<loader::LOADER_UserFileInfo> userFileInfo;
   be2_virt_ptr<OSDynLoad_NotifyData> notifyData;
   be2_val<virt_addr> entryPoint;
   be2_val<uint32_t> dataSectionSize;
   be2_virt_ptr<void> dataSection;
   be2_val<uint32_t> loadSectionSize;
   be2_virt_ptr<void> loadSection;
   be2_val<OSDynLoad_FreeFn> dynLoadFreeFn;
   be2_val<virt_addr> codeExports;
   be2_val<uint32_t> numCodeExports;
   be2_val<virt_addr> dataExports;
   be2_val<uint32_t> numDataExports;
   be2_virt_ptr<RPL_DATA> next;
   UNKNOWN(0x94 - 0x58);
};

struct StaticDynLoadData
{
   be2_array<char, 32> loaderLockName;

   be2_val<OSDynLoad_AllocFn> allocFn;
   be2_val<OSDynLoad_FreeFn> freeFn;

   be2_val<BOOL> tlsAllocLocked;
   be2_val<uint32_t> tlsHeader;
   be2_val<uint32_t> tlsModuleIndex;
   be2_val<OSDynLoad_AllocFn> tlsAllocFn;
   be2_val<OSDynLoad_FreeFn> tlsFreeFn;

   be2_virt_ptr<OSDynLoad_NotifyCallback> notifyCallbacks;

   be2_struct<OSHandleTable> handleTable;

   be2_array<char, 16> coreinitModuleName;
   be2_virt_ptr<RPL_DATA> rplDataList;

   be2_virt_ptr<RPL_DATA> rpxData;
   be2_virt_ptr<const char> rpxName;
   be2_val<uint32_t> rpxNameLength;

   be2_val<BOOL> inModuleEntryPoint;
   be2_virt_ptr<RPL_DATA> linkingRplList;

   be2_val<MEMHeapHandle> preInitMem1Heap;
   be2_val<MEMHeapHandle> preInitMem2Heap;
   be2_val<MEMHeapHandle> preInitForegroundHeap;

   be2_val<uint32_t> numAllocations;
   be2_val<uint32_t> totalAllocatedBytes;

   be2_val<uint32_t> numModules;
   be2_virt_ptr<virt_ptr<RPL_DATA>> modules;

   struct FatalErrorInfo
   {
      be2_val<uint32_t> msgType;
      be2_val<uint32_t> error;
      be2_val<uint32_t> loaderError;
      be2_val<uint32_t> line;
      be2_array<char, 64> funcName;
   };

   be2_struct<FatalErrorInfo> fatalError;
};

static virt_ptr<StaticDynLoadData>
sDynLoadData = nullptr;

static virt_ptr<OSMutex>
sDynLoad_LoaderLock = nullptr;

namespace internal
{

static OSDynLoad_Error
doImports(virt_ptr<RPL_DATA> rplData);

static void
release(OSDynLoad_ModuleHandle moduleHandle,
        virt_ptr<virt_ptr<RPL_DATA>> unloadedModuleList);

/**
 * Perform an allocation for the OSDYNLOAD_HEAP heap.
 */
static OSDynLoad_Error
dynLoadHeapAlloc(const char *name,
                 OSDynLoad_AllocFn allocFn,
                 uint32_t size,
                 int32_t align,
                 virt_ptr<virt_ptr<void>> outPtr)
{
   auto error = cafe::invoke(cpu::this_core::state(), allocFn, size, align,
                             outPtr);

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "OSDYNLOAD_HEAP:{},ALLOC,=\"{}\",-{}",
         name, *outPtr, size));
   }

   return error;
}


/**
 * Perform a free for the OSDYNLOAD_HEAP heap.
 */
static void
dynLoadHeapFree(const char *name,
                OSDynLoad_FreeFn freeFn,
                virt_ptr<void> ptr,
                uint32_t size)
{
   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "OSDYNLOAD_HEAP:{},FREE,=\"{}\",-{}",
         name, ptr, size));
   }

   cafe::invoke(cpu::this_core::state(), freeFn, ptr);
}


/**
 * Perform an allocation for the RPL_SYSHEAP heap.
 */
static virt_ptr<void>
rplSysHeapAlloc(const char *name,
                uint32_t size,
                int32_t align)
{
   auto ptr = OSAllocFromSystem(size, align);

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_SYSHEAP:{},ALLOC,=\"{}\",-{}",
         name, ptr, size));
   }

   return ptr;
}


/**
 * Perform a free for the RPL_SYSHEAP heap.
 */
static void
rplSysHeapFree(const char *name,
               virt_ptr<void> ptr,
               uint32_t size)
{
   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_SYSHEAP:{},FREE,=\"{}\",-{}",
         name, ptr, size));
   }

   OSFreeToSystem(ptr);
}


/**
 * Set dynload fatal error info.
 */
static void
setFatalErrorInfo(uint32_t fatalMsgType,
                  uint32_t fatalErr,
                  uint32_t loaderError,
                  uint32_t fatalLine,
                  virt_ptr<const char> fatalFunction)
{
   if (sDynLoadData->fatalError.error) {
      return;
   }

   sDynLoadData->fatalError.msgType = fatalMsgType;
   sDynLoadData->fatalError.error = fatalErr;
   sDynLoadData->fatalError.loaderError = loaderError;
   sDynLoadData->fatalError.line = fatalLine;
   sDynLoadData->fatalError.funcName = fatalFunction.get();
}


/**
 * Set dynload fatal error info with some sort of deviceLocation magic.
 */
static void
setFatalErrorInfo2(uint32_t deviceLocation,
                   uint32_t loaderError,
                   bool a3,
                   uint32_t fatalLine,
                   std::string_view fatalFunction)
{
   if (sDynLoadData->fatalError.error) {
      return;
   }

   switch (deviceLocation) {
   case 0:
   {
      if (a3) {
         sDynLoadData->fatalError.msgType = 2u;
         sDynLoadData->fatalError.error = 1603203u;
      } else {
         sDynLoadData->fatalError.msgType = 1u;
         sDynLoadData->fatalError.error = 1603200u;
      }

      sDynLoadData->fatalError.loaderError = loaderError;
      sDynLoadData->fatalError.line = fatalLine;
      sDynLoadData->fatalError.funcName = fatalFunction;
      break;
   }
   case 1:
   {
      if (a3) {
         sDynLoadData->fatalError.msgType = 8u;
         sDynLoadData->fatalError.error = 1603204u;
      } else {
         sDynLoadData->fatalError.msgType = 1u;
         sDynLoadData->fatalError.error = 1603201u;
      }

      sDynLoadData->fatalError.loaderError = loaderError;
      sDynLoadData->fatalError.line = fatalLine;
      sDynLoadData->fatalError.funcName = fatalFunction;
      break;
   }
   case 2:
   {
      if (a3) {
         sDynLoadData->fatalError.msgType = 5u;
         sDynLoadData->fatalError.error = 1604205u;
      } else {
         sDynLoadData->fatalError.msgType = 1u;
         sDynLoadData->fatalError.error = 1602202u;
      }

      sDynLoadData->fatalError.loaderError = loaderError;
      sDynLoadData->fatalError.line = fatalLine;
      sDynLoadData->fatalError.funcName = fatalFunction;
      break;
   }
   default:
      OSPanic("OSDynLoad_DynInit.c", 104, "***Unknown device location error.");
   }
}


/**
 * Clear the dynload fatal error info.
 */
static void
resetFatalErrorInfo()
{
   std::memset(virt_addrof(sDynLoadData->fatalError).get(),
               0,
               sizeof(sDynLoadData->fatalError));
}


/**
 * Report a fatal dynload error to the kernel.
 */
static void
reportFatalError()
{
   StackObject<OSFatalError> error;

   if (!sDynLoadData->fatalError.error) {
      OSPanic("OSDynLoad_DynInit.c", 122,
              "__OSDynLoad_InitFromCoreInit() - gFatalInfo unexpectantly empty!!!.");
   }

   error->errorCode = sDynLoadData->fatalError.error;
   error->internalErrorCode = sDynLoadData->fatalError.loaderError;
   error->messageType = sDynLoadData->fatalError.msgType;
   error->processId = static_cast<uint32_t>(OSGetUPID());
   OSSendFatalError(error,
                    virt_addrof(sDynLoadData->fatalError.funcName),
                    sDynLoadData->fatalError.line);
}

static OSDynLoad_Error
internalAcquire(virt_ptr<const char> name,
                virt_ptr<virt_ptr<RPL_DATA>> outRplData,
                virt_ptr<uint32_t> outNumEntryModules,
                virt_ptr<virt_ptr<virt_ptr<RPL_DATA>>> outEntryModules,
                bool doLoad);

/**
 * Run all the entry points in the given entry points array.
 */
static int32_t
runEntryPoints(uint32_t numEntryModules,
               virt_ptr<virt_ptr<RPL_DATA>> entryModules)
{
   auto error = 0;

   for (auto i = 0u; i < numEntryModules; ++i) {
      auto entry = virt_func_cast<OSDynLoad_RplEntryFn>(entryModules[i]->entryPoint);
      error = cafe::invoke(cpu::this_core::state(),
                           entry,
                           entryModules[i]->handle,
                           OSDynLoad_EntryReason::Loaded);
      if (error) {
         COSError(COSReportModule::Unknown2, fmt::format(
            "*** ERROR: Module \"{}\" returned error code {} ({} from its entrypoint on load.",
            entryModules[i]->moduleName, error, error));
         setFatalErrorInfo2(entryModules[i]->userFileInfo->titleLocation,
                            error, false,
                            44, "__OSDynLoad_RunEntryPoints");
      }

      entryModules[i]->entryPoint = virt_addr { 0 };
   }

   if (entryModules) {
      rplSysHeapFree("RPL_ENTRYPOINTS_ARRAY", entryModules, 4 * numEntryModules);
   }

   return error;
}


/**
 * Load modulePath.
 */
static OSDynLoad_Error
internalAcquire2(virt_ptr<const char> modulePath,
                 virt_ptr<OSDynLoad_ModuleHandle> outModuleHandle,
                 BOOL isCheckLoaded)
{
   StackObject<virt_ptr<RPL_DATA>> rplData;
   StackObject<uint32_t> numEntryModules;
   StackObject<virt_ptr<virt_ptr<RPL_DATA>>> entryModules;
   *rplData = nullptr;
   *numEntryModules = 0u;
   *entryModules = nullptr;

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_SYSHEAP:{} START,{}",
         isCheckLoaded ? "CHECK_LOADED" : "ACQUIRE",
         modulePath));
   }

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "SYSTEM_HEAP:{} START,{}",
         isCheckLoaded ? "CHECK_LOADED" : "ACQUIRE",
         modulePath));
   }

   if (!outModuleHandle) {
      return OSDynLoad_Error::InvalidAcquirePtr;
   }

   if (!modulePath) {
      return OSDynLoad_Error::InvalidModuleNamePtr;
   }

   if (!modulePath[0]) {
      return OSDynLoad_Error::InvalidModuleName;
   }

   resetFatalErrorInfo();

   if (auto error = internalAcquire(modulePath, rplData, numEntryModules,
                                    entryModules, isCheckLoaded)) {
      if (*entryModules) {
         rplSysHeapFree("ENTRYPOINTS_ARRAY", entryModules, isCheckLoaded);
      }

      if (!isCheckLoaded) {
         COSError(COSReportModule::Unknown2, fmt::format(
            "Error: Could not load acquired RPL \"{}\".", modulePath));
      }

      *outModuleHandle = 0u;
      return error;
   }

   *outModuleHandle = (*rplData)->handle;

   if (!isCheckLoaded) {
      if (runEntryPoints(*numEntryModules, *entryModules) != OSDynLoad_Error::OK) {
         if (*outModuleHandle) {
            OSDynLoad_Release(*outModuleHandle);
         }

         *outModuleHandle = 0u;
         return OSDynLoad_Error::RunEntryPointError;
      }
   }

   resetFatalErrorInfo();

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_SYSHEAP:{} END,{}",
         isCheckLoaded ? "CHECK_LOADED" : "ACQUIRE",
         modulePath));
   }

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "SYSTEM_HEAP:{} END,{}",
         isCheckLoaded ? "CHECK_LOADED" : "ACQUIRE",
         modulePath));
   }

   return OSDynLoad_Error::OK;
}


/**
 * Count the number of notify callbacks registered.
 */
static uint32_t
getNotifyCallbackCount()
{
   auto count = 0u;
   for (auto itr = sDynLoadData->notifyCallbacks; itr; itr = itr->next) {
      ++count;
   }

   return count;
}


/**
 * Unload the given module.
 */
static void
unloadModule(virt_ptr<RPL_DATA> rplData,
             virt_ptr<virt_ptr<RPL_DATA>> unloadedModuleList)
{
   // Run the RPL entry point with Unloaded
   if (rplData->entryPoint) {
      auto entry = virt_func_cast<OSDynLoad_RplEntryFn>(rplData->entryPoint);
      cafe::invoke(cpu::this_core::state(),
                   entry,
                   rplData->handle,
                   OSDynLoad_EntryReason::Unloaded);
   }

   if (rplData->notifyData) {
      // TODO: Check for alarms, threads, interrupt callbacks, ipc callbacks
      // which reference this module
   }

   // Release all imported modules
   for (auto i = 0u; i < rplData->importModuleCount; ++i) {
      if (rplData->importModules[i] &&
          rplData->importModules[i] != virt_cast<RPL_DATA *>(virt_addr { 0xFFFFFFFFu })) {
         release(rplData->importModules[i]->handle, unloadedModuleList);
         rplData->importModules[i] = nullptr;
      }
   }

   // Remove from the loaded module list
   auto prev = virt_ptr<RPL_DATA> { nullptr };
   for (auto itr = sDynLoadData->rplDataList; itr; itr = itr->next) {
      if (itr == rplData) {
         break;
      }

      prev = itr;
   }

   if (prev) {
      prev->next = rplData->next;
   } else {
      sDynLoadData->rplDataList = rplData->next;
   }

   // Insert into unloaded module list
   rplData->next = *unloadedModuleList;
   *unloadedModuleList = rplData;
}


/**
 * Free allocated memory for a loaded module.
 */
static void
internalPurge(virt_ptr<RPL_DATA> rpl)
{
   if (!rpl->handle) {
      internal::OSPanic("OSDynLoad_Purge.c", 23,
                        "OSDynLoad_InternalPurge - nonzero handle on purge.");
   }

   if (rpl->loaderHandle) {
      kernel::loaderPurge(rpl->loaderHandle);
      rpl->loaderHandle = nullptr;
   }

   rpl->entryPoint = virt_addr { 0 };
   rpl->codeExports = virt_addr { 0 };
   rpl->dataExports = virt_addr { 0 };
   rpl->numDataExports = virt_addr { 0 };
   rpl->numCodeExports = virt_addr { 0 };

   // Free dataSection
   if (rpl->dataSection && rpl->dynLoadFreeFn) {
      dynLoadHeapFree("RPL_DATA_AREA", rpl->dynLoadFreeFn,
                      rpl->dataSection, rpl->dataSectionSize);
      sDynLoadData->totalAllocatedBytes -= rpl->dataSectionSize;
      sDynLoadData->numAllocations--;
   }

   rpl->dataSectionSize = 0u;

   // Free loadSection
   if (rpl->loadSection && rpl->dynLoadFreeFn) {
      dynLoadHeapFree("RPL_DATA_AREA", rpl->dynLoadFreeFn,
                      rpl->loadSection, rpl->loadSectionSize);
      sDynLoadData->totalAllocatedBytes -= rpl->loadSectionSize;
      sDynLoadData->numAllocations--;
   }

   rpl->loadSectionSize = 0u;

   // Free notifyData
   if (rpl->notifyData) {
      if (rpl->notifyData->name) {
         auto pathStringLength = 0u;
         if (rpl->userFileInfo) {
            pathStringLength = rpl->userFileInfo->pathStringLength;
         }

         rplSysHeapFree("RPL_NOTIFY_NAME",
                        rpl->notifyData->name, pathStringLength);
      }

      rplSysHeapFree("RPL_NOTIFY",
                     rpl->notifyData, sizeof(OSDynLoad_NotifyData));
   }

   rpl->notifyData = nullptr;

   // Free file info
   if (rpl->userFileInfo && rpl->dynLoadFreeFn) {
      dynLoadHeapFree("RPL_FILE_INFO", rpl->dynLoadFreeFn,
                      rpl->userFileInfo, rpl->userFileInfoSize);
      sDynLoadData->totalAllocatedBytes -= rpl->userFileInfoSize;
      sDynLoadData->numAllocations--;
   }

   rpl->userFileInfo = nullptr;
   rpl->userFileInfoSize = 0u;

   // Free section info
   if (rpl->sectionInfo) {
      rplSysHeapFree("RPL_SEC_INFO",
                     rpl->sectionInfo,
                     sizeof(loader::LOADER_SectionInfo) * rpl->sectionInfoCount);
   }

   rpl->sectionInfo = nullptr;
   rpl->sectionInfoCount = 0u;

   // Free imported modules array
   if (rpl->importModules) {
      rplSysHeapFree("RPL_SEC_INFO",
                     rpl->importModules, 4 * rpl->importModuleCount);
   }

   rpl->importModules = nullptr;
   rpl->importModuleCount = 0u;
   rpl->dynLoadFreeFn = nullptr;

   // Free module name
   if (rpl->moduleName) {
      rplSysHeapFree("RPL_NAME",
                     rpl->moduleName,
                     align_up(rpl->moduleNameLen, 4));
   }

   rpl->moduleName = nullptr;
   rpl->moduleNameLen = 0u;

   // And finally.. free the RPL data itself.
   std::memset(rpl.get(), 0, sizeof(RPL_DATA));
   rplSysHeapFree("RPL_DATA", rpl, sizeof(RPL_DATA));
}


/**
 * Release a loaded module.
 */
static void
release(OSDynLoad_ModuleHandle moduleHandle,
        virt_ptr<virt_ptr<RPL_DATA>> unloadedModuleList)
{
   StackObject<virt_ptr<void>> userData1;
   StackObject<uint32_t> refCount;
   StackObject<virt_ptr<RPL_DATA>> rootUnloadedModuleList;

   if (moduleHandle == OSDynLoad_CurrentModuleHandle ||
       !sDynLoadData->rplDataList) {
      return;
   }

   // First we translate and add ref to get the userData1
   if (auto error = OSHandle_TranslateAndAddRef(virt_addrof(sDynLoadData->handleTable),
                                                moduleHandle,
                                                userData1,
                                                nullptr)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "*** OSDynLoad_Release: RPL Module handle {} was not valid@0. (err=0x{:08X})",
         moduleHandle, error));
      return;
   }

   auto rplData = virt_cast<RPL_DATA *>(userData1);

   // Release our just acquired reference
   if (auto error = OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                     moduleHandle,
                                     refCount)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "*** OSDynLoad_Release: RPL Module handle {} was not valid@1. (err=0x{:08X})",
         moduleHandle, error));
      return;
   }

   if (refCount == 0) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "*** OSDynLoad_Release: reference count error on OSDynLoad_ModuleHandle. ({})",
         *refCount));
      return;
   }

   // Now do the actual release
   if (auto error = OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                     moduleHandle,
                                     refCount)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "*** OSDynLoad_Release: RPL Module handle {} was not valid@2. (err=0x{:08X})",
         moduleHandle, error));
      return;
   }

   if (refCount) {
      // Nothing to do, ref count still >0
      return;
   }

   OSLockMutex(sDynLoad_LoaderLock);

   auto isFirstUnloadModule = false;
   if (!unloadedModuleList) {
      unloadedModuleList = rootUnloadedModuleList;
      *rootUnloadedModuleList = nullptr;
      isFirstUnloadModule = true;
   }

   auto notifyCallbackCount = 0u;
   auto notifyCallbackArray = virt_ptr<OSDynLoad_NotifyCallback> { nullptr };

   if (isFirstUnloadModule) {
      if (notifyCallbackCount = getNotifyCallbackCount()) {
         auto allocSize = static_cast<uint32_t>(
            notifyCallbackCount * sizeof(OSDynLoad_NotifyCallback));

         notifyCallbackArray = virt_cast<OSDynLoad_NotifyCallback *>(
            rplSysHeapAlloc("RPL_NOTIFY_ARRAY", allocSize, 4));
         if (notifyCallbackArray) {
            auto notifyCallback = sDynLoadData->notifyCallbacks;
            for (auto i = 0u; notifyCallback; ++i) {
               std::memcpy(virt_addrof(notifyCallbackArray[i]).get(),
                           notifyCallback.get(),
                           sizeof(OSDynLoad_NotifyCallback));

               notifyCallback = notifyCallback->next;
            }
         }
      }
   }

   unloadModule(rplData, unloadedModuleList);

   if (isFirstUnloadModule) {
      // Call notify callbacks for all the unloaded modules and free their
      // notify data
      for (auto rpl = *unloadedModuleList; rpl; rpl = rpl->next) {
         if (notifyCallbackArray) {
            for (auto i = 0u; i < notifyCallbackCount; ++i) {
               cafe::invoke(cpu::this_core::state(),
                            notifyCallbackArray[i].notifyFn,
                            rpl->handle,
                            notifyCallbackArray->userArg1,
                            OSDynLoad_NotifyEvent::Unloaded,
                            rpl->notifyData);
            }
         }

         if (rpl->notifyData->name) {
            auto pathStringLength = 0u;
            if (rpl->userFileInfo) {
               pathStringLength = rpl->userFileInfo->pathStringLength;
            }

            rplSysHeapFree("RPL_NOTIFY",
                           rpl->notifyData->name, pathStringLength);
         }

         rplSysHeapFree("RPL_NOTIFY",
                        rpl->notifyData, sizeof(OSDynLoad_NotifyData));

         rpl->notifyData = nullptr;
      }

      if (notifyCallbackArray) {
         rplSysHeapFree("RPL_NOTIFY_ARRAY",
                        notifyCallbackArray,
                        notifyCallbackCount * sizeof(OSDynLoad_NotifyCallback));
      }

      for (auto rpl = *unloadedModuleList; rpl; rpl = rpl->next) {
         internalPurge(rpl);
      }
   }

   OSUnlockMutex(sDynLoad_LoaderLock);
}


/**
 * Binary search for an export.
 */
static virt_ptr<loader::rpl::Export>
binarySearchExport(virt_ptr<loader::rpl::Export> exports,
                   uint32_t numExports,
                   virt_ptr<const char> name)
{
   auto exportNames = virt_cast<const char *>(exports) - 8;
   auto left = 0u;
   auto right = numExports;

   while (true) {
      auto index = left + (right - left) / 2;
      auto exportName = exportNames.get() + (exports[index].name & 0x7FFFFFFF);
      auto cmpValue = strcmp(name.get(), exportName);
      if (cmpValue == 0) {
         return exports + index;
      } else if (cmpValue < 0) {
         right = index;
      } else {
         left = index + 1;
      }

      if (left >= right) {
         return nullptr;
      }
   }
}


/**
 * Find export sections in an RPL.
 */
static void
findExports(virt_ptr<RPL_DATA> rplData)
{
   for (auto i = 0u; i < rplData->sectionInfoCount; ++i) {
      auto &sectionInfo = rplData->sectionInfo[i];
      if (sectionInfo.type == loader::rpl::SHT_RPL_EXPORTS) {
         if (sectionInfo.flags & loader::rpl::SHF_EXECINSTR) {
            rplData->codeExports = sectionInfo.address + 8;
            rplData->numCodeExports = *virt_cast<uint32_t *>(sectionInfo.address);
         } else {
            rplData->dataExports = sectionInfo.address + 8;
            rplData->numDataExports = *virt_cast<uint32_t *>(sectionInfo.address);
         }
      }
   }
}


/**
 * Find TLS sections in an RPL.
 */
static bool
findTlsSection(virt_ptr<RPL_DATA> rplData)
{
   // Find TLS section
   if (rplData->userFileInfo->fileInfoFlags & loader::rpl::RPL_HAS_TLS) {
      auto tlsAddressStart = virt_addr { 0xFFFFFFFFu };
      auto tlsAddressEnd = virt_addr { 0 };

      for (auto i = 0u; i < rplData->sectionInfoCount; ++i) {
         auto &sectionInfo = rplData->sectionInfo[i];
         if (sectionInfo.flags & loader::rpl::SHF_TLS) {
            if (sectionInfo.address < tlsAddressStart) {
               tlsAddressStart = sectionInfo.address;
            }

            if (sectionInfo.address + sectionInfo.size > tlsAddressEnd) {
               tlsAddressEnd = sectionInfo.address + sectionInfo.size;
            }
         }
      }

      if (tlsAddressStart == virt_addr { 0xFFFFFFFFu } ||
          tlsAddressEnd == virt_addr { 0 }) {
         return false;
      }

      rplData->userFileInfo->tlsAddressStart = tlsAddressStart;
      rplData->userFileInfo->tlsSectionSize =
         static_cast<uint32_t>(tlsAddressEnd - tlsAddressStart);
   }

   return true;
}


/**
 * sSetupPerm
 */
static int32_t
setupPerm(virt_ptr<RPL_DATA> rplData,
          bool updateTlsModuleIndex)
{
   StackObject<loader::LOADER_MinFileInfo> minFileInfo;
   std::memset(minFileInfo.get(), 0, sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->size = static_cast<uint32_t>(sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->version = 4u;
   minFileInfo->outPathStringSize = virt_addrof(minFileInfo->pathStringSize);
   minFileInfo->outNumberOfSections = virt_addrof(rplData->sectionInfoCount);
   minFileInfo->outSizeOfFileInfo = virt_addrof(rplData->userFileInfoSize);

   if (updateTlsModuleIndex) {
      minFileInfo->inoutNextTlsModuleNumber = virt_addrof(sDynLoadData->tlsModuleIndex);
   }

   // Query to get the required buffer sizes
   auto error = kernel::loaderQuery(rplData->loaderHandle, minFileInfo);
   if (error) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - could not query loader about module (err=0x{:08X}).",
         error));

      if (minFileInfo->fatalErr) {
         setFatalErrorInfo(
            minFileInfo->fatalMsgType,
            minFileInfo->fatalErr,
            minFileInfo->error,
            minFileInfo->fatalLine,
            virt_addrof(minFileInfo->fatalFunction));
         reportFatalError();
      } else {
         setFatalErrorInfo2(minFileInfo->fileLocation, error, 1, 162, "sSetupPerm");
         reportFatalError();
      }
   }

   // Allocate section info
   rplData->sectionInfo = virt_cast<loader::LOADER_SectionInfo *>(
      rplSysHeapAlloc("RPL_SEC_INFO",
                      sizeof(loader::LOADER_SectionInfo) * rplData->sectionInfoCount,
                      -4));
   if (!rplData->sectionInfo) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - could not allocate memory to hold section info (err=0x{:08X}).",
         OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(minFileInfo->fileLocation,
                         OSDynLoad_Error::OutOfSysMemory,
                         0, 179, "sSetupPerm");
      reportFatalError();
   }

   minFileInfo->outSectionInfo = rplData->sectionInfo;

   // Allocate file info
   rplData->userFileInfo = virt_cast<loader::LOADER_UserFileInfo *>(
      rplSysHeapAlloc("RPL_FILE_INFO",
                      rplData->userFileInfoSize, 4));

   if (!rplData->userFileInfo) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - could not allocate memory to hold file info (err=0x{:08X}).",
         OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(minFileInfo->fileLocation,
                         OSDynLoad_Error::OutOfSysMemory,
                         0, 196, "sSetupPerm");
      reportFatalError();
   }

   minFileInfo->outFileInfo = rplData->userFileInfo;
   std::memset(rplData->userFileInfo.get(), 0,
               sizeof(loader::LOADER_UserFileInfo));

   // Allocate path string
   if (minFileInfo->pathStringSize) {
      minFileInfo->pathStringBuffer = virt_cast<char *>(
         rplSysHeapAlloc("RPL_NOTIFY_NAME",
                         minFileInfo->pathStringSize, 4));
      if (!minFileInfo->pathStringBuffer) {
         if (isAppDebugLevelNotice()) {
            dumpSystemHeap();
         }

         COSError(COSReportModule::Unknown2, fmt::format(
            "__OSDynLoad_InitFromCoreInit() - could not allocate memory to hold path string (err=0x{:08X}).",
            OSDynLoad_Error::OutOfSysMemory));
         setFatalErrorInfo2(minFileInfo->fileLocation,
                            OSDynLoad_Error::OutOfSysMemory,
                            0, 218, "sSetupPerm");
         reportFatalError();
      }

      std::memset(minFileInfo->pathStringBuffer.get(), 0,
                  minFileInfo->pathStringSize);
   }

   // Query to get the data
   error = kernel::loaderQuery(rplData->loaderHandle, minFileInfo);
   if (error) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - could not get file information (err=0x{:08X}).",
         error));
      setFatalErrorInfo(
         minFileInfo->fatalMsgType,
         minFileInfo->fatalErr,
         minFileInfo->error,
         minFileInfo->fatalLine,
         virt_addrof(minFileInfo->fatalFunction));
      reportFatalError();
   }

   findExports(rplData);

   if (!findTlsSection(rplData)) {
      COSError(COSReportModule::Unknown2,
         "*** Can not find section for TLS data.");
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation,
                         OSDynLoad_Error::TLSSectionNotFound, 1,
                         277, "sSetupPerm");
      reportFatalError();
   }

   return 0;
}


/**
 * Call the initialise default heap function for an RPL.
 *
 * For .rpx this is __preinit_user, for coreinit it is CoreInitDefaultHeap.
 */
static OSDynLoad_Error
initialiseDefaultHeap(virt_ptr<RPL_DATA> rplData,
                      virt_ptr<const char> functionName)
{
   StackObject<virt_addr> funcAddr;
   auto error = OSDynLoad_FindExport(rplData->handle, FALSE, functionName,
                                     funcAddr);
   if (error != OSDynLoad_Error::OK) {
      return error;
   }

   cafe::invoke(cpu::this_core::state(),
                virt_func_cast<OSDynLoad_InitDefaultHeapFn>(*funcAddr),
                virt_addrof(sDynLoadData->preInitMem1Heap),
                virt_addrof(sDynLoadData->preInitForegroundHeap),
                virt_addrof(sDynLoadData->preInitMem2Heap));

   StackObject<OSDynLoad_AllocFn> allocFn;
   StackObject<OSDynLoad_FreeFn> freeFn;
   error = OSDynLoad_GetAllocator(allocFn, freeFn);
   if (error != OSDynLoad_Error::OK || !(*allocFn) || !(*freeFn)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InternalInitDefaultHeap() - {} did not set OSDynLoad "
         "allocator (err=0x{:08X}).",
         functionName, error));
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation, error, 0, 363,
                         "__OSDynLoad_InternalInitDefaultHeap");
      reportFatalError();
   }

   error = OSDynLoad_GetTLSAllocator(allocFn, freeFn);
   if (error != OSDynLoad_Error::OK || !(*allocFn) || !(*freeFn)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InternalInitDefaultHeap() - {} did not set OSDynLoad "
         "TLS allocator (err=0x{:08X}).",
         functionName, error));
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation, error, 0, 363,
                         "__OSDynLoad_InternalInitDefaultHeap");
      reportFatalError();
   }

   return OSDynLoad_Error::OK;
}


/**
 * Resolve a module name.
 *
 * Removes any directories or file extension.
 */
static std::pair<virt_ptr<const char>, uint32_t>
resolveModuleName(virt_ptr<const char> name)
{
   auto str = std::string_view { name.get() };
   auto pos = str.find_last_of("\\/");
   if (pos != std::string_view::npos) {
      str = str.substr(pos);
      name += pos;
   }

   pos = str.find_first_of('.');
   if (pos != std::string_view::npos) {
      str = str.substr(0, pos);
   }

   return { name, static_cast<uint32_t>(str.size()) };
}


/**
 * __OSDynLoad_InitCommon
 */
static OSDynLoad_Error
initialiseCommon()
{
   sDynLoadData->loaderLockName = "{ LoaderLock }";
   OSInitMutexEx(sDynLoad_LoaderLock,
                 virt_addrof(sDynLoadData->loaderLockName));
   OSHandle_InitTable(virt_addrof(sDynLoadData->handleTable));

   // Parse argstr into rpx name
   std::tie(sDynLoadData->rpxName, sDynLoadData->rpxNameLength) = resolveModuleName(getArgStr());
   if (sDynLoadData->rpxNameLength > 63u) {
      sDynLoadData->rpxNameLength = 63u;
   }

   // TODO: OSDynLoad_AddNotifyCallback(MasterAgent_LoadNotify, 0)
   return OSDynLoad_Error::OK;
}


/**
 * __BuildKernelNotify
 */
static OSDynLoad_Error
buildKernelNotify(virt_ptr<RPL_DATA> linkList,
                  virt_ptr<loader::LOADER_LinkInfo> *outLinkInfo,
                  virt_ptr<RPL_DATA> rpl)
{
   auto numModules = 0u;
   *outLinkInfo = nullptr;

   for (auto module = linkList; module; module = module->next) {
      numModules++;
   }

   auto linkInfoSize =
      static_cast<uint32_t>(sizeof(loader::LOADER_LinkModule) * numModules + 8);
   auto linkInfo = virt_cast<loader::LOADER_LinkInfo *>(
      rplSysHeapAlloc("RPL_MODULES_LINK_ARRAY", linkInfoSize, -4));
   if (!linkInfo) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(rpl->userFileInfo->titleLocation,
                         OSDynLoad_Error::OutOfSysMemory, 0, 101,
                         "__BuildKernelNotify");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   std::memset(linkInfo.get(), 0, linkInfoSize);
   linkInfo->numModules = numModules;
   linkInfo->size = linkInfoSize;

   auto moduleIndex = 0u;
   for (auto module = linkList; module; module = module->next) {
      linkInfo->modules[moduleIndex].loaderHandle = module->loaderHandle;
      linkInfo->modules[moduleIndex].entryPoint = virt_addr { 0 };
      moduleIndex++;
   }

   *outLinkInfo = linkInfo;
   return OSDynLoad_Error::OK;
}


/**
 * __OSDynLoad_ExecuteDynamicLink
 */
static OSDynLoad_Error
executeDynamicLink(virt_ptr<RPL_DATA> rpx,
                   virt_ptr<uint32_t> outNumEntryModules,
                   virt_ptr<virt_ptr<virt_ptr<RPL_DATA>>> outEntryModules,
                   virt_ptr<RPL_DATA> rpl)
{
   StackObject<loader::LOADER_MinFileInfo> minFileInfo;
   auto linkInfo = virt_ptr<loader::LOADER_LinkInfo> { nullptr };
   auto error = OSDynLoad_Error::OK;

   *outNumEntryModules = 0u;
   *outEntryModules = nullptr;

   std::memset(minFileInfo.get(), 0,
               sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->size = static_cast<uint32_t>(sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->version = 4u;
   if (error = buildKernelNotify(sDynLoadData->linkingRplList, &linkInfo, rpl)) {
      return error;
   }

   // Call the loader to link the rpx
   error = static_cast<OSDynLoad_Error>(kernel::loaderLink(nullptr,
                                                           minFileInfo,
                                                           linkInfo,
                                                           linkInfo->size));
   if (error) {
      COSError(COSReportModule::Unknown2,
               "*** Kernel refused to link RPLs.");

      if (minFileInfo->fatalErr) {
         setFatalErrorInfo(
            minFileInfo->fatalMsgType,
            minFileInfo->fatalErr,
            minFileInfo->error,
            minFileInfo->fatalLine,
            virt_addrof(minFileInfo->fatalFunction));
      }

      error = OSDynLoad_Error::LoaderError;
   } else {
      for (auto i = 0u; i < linkInfo->numModules; ++i) {
         auto &linkModule = linkInfo->modules[i];
         auto rplData = virt_ptr<RPL_DATA> { nullptr };

         for (rplData = sDynLoadData->linkingRplList; rplData; rplData = rplData->next) {
            if (rplData->loaderHandle == linkModule.loaderHandle) {
               break;
            }
         }

         if (!rplData) {
            OSPanic("OSDynLoad_Acquire.c", 110,
                    "*** Error in dynamic linking.  linked module not found.");
            error = OSDynLoad_Error::ModuleNotFound;
         } else {
            // Allocate and fill out the notify data structure
            rplData->entryPoint = linkModule.entryPoint;
            rplData->notifyData = virt_cast<OSDynLoad_NotifyData *>(
               rplSysHeapAlloc("RPL_NOTIFY",
                               sizeof(OSDynLoad_NotifyData), 4));
            if (!rplData->notifyData) {
               if (isAppDebugLevelNotice()) {
                  dumpSystemHeap();
               }

               setFatalErrorInfo2(rpl->userFileInfo->titleLocation,
                                  OSDynLoad_Error::OutOfSysMemory,
                                  0, 174, "__OSDynLoad_ExecuteDynamicLink");
               error = OSDynLoad_Error::OutOfSysMemory;
               continue;
            }

            auto notifyData = rplData->notifyData;
            std::memset(notifyData.get(), 0,
                        sizeof(OSDynLoad_NotifyData));

            notifyData->name = rplData->userFileInfo->pathString;
            rplData->userFileInfo->pathString = nullptr;

            notifyData->textAddr = linkModule.textAddr;
            notifyData->textSize = linkModule.textSize;
            notifyData->textOffset = linkModule.textOffset;

            notifyData->dataAddr = linkModule.dataAddr;
            notifyData->dataSize = linkModule.dataSize;
            notifyData->dataOffset = linkModule.dataOffset;

            notifyData->readAddr = linkModule.dataAddr;
            notifyData->readSize = linkModule.dataSize;
            notifyData->readOffset = linkModule.dataOffset;

            if (!isAppDebugLevelNotice()) {
               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "{}: TEXT {}:{} DATA {}:{} LOAD {}:{}",
                  rplData->moduleName,
                  linkModule.textAddr,
                  linkModule.textAddr + linkModule.textSize,
                  linkModule.dataAddr,
                  linkModule.dataAddr + linkModule.dataSize,
                  linkModule.loadAddr,
                  linkModule.loadAddr + linkModule.loadSize));
            } else {
               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},TEXT,start,=\"{}\"",
                  rplData->moduleName,
                  linkModule.textAddr));

               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},TEXT,end,=\"{}\"",
                  rplData->moduleName,
                  linkModule.textAddr + linkModule.textSize));

               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},DATA,start,=\"{}\"",
                  rplData->moduleName,
                  linkModule.dataAddr));

               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},DATA,end,=\"{}\"",
                  rplData->moduleName,
                  linkModule.dataAddr + linkModule.dataSize));

               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},LOAD,start,=\"{}\"",
                  rplData->moduleName,
                  linkModule.loadAddr));

               COSInfo(COSReportModule::Unknown2, fmt::format(
                  "RPL_LAYOUT:{},LOAD,end,=\"{}\"",
                  rplData->moduleName,
                  linkModule.loadAddr + linkModule.loadSize));
            }
         }
      }
   }

   if (!error) {
      // Allocate and fill up the entry modules array
      auto entryModules = virt_ptr<virt_ptr<RPL_DATA>> { nullptr };
      sDynLoadData->modules =
         virt_cast<virt_ptr<RPL_DATA> *>(
            rplSysHeapAlloc("RPL_MODULES_ARRAY",
                            4 * linkInfo->numModules, -4));

      if (!sDynLoadData->modules) {
         if (isAppDebugLevelNotice()) {
            dumpSystemHeap();
         }

         sDynLoadData->modules = nullptr;
         sDynLoadData->numModules = 0u;
      } else {
         entryModules = virt_cast<virt_ptr<RPL_DATA> *>(
            rplSysHeapAlloc("RPL_ENTRYPOINTS_ARRAY",
                            4 * linkInfo->numModules, -4));

         if (!entryModules) {
            if (isAppDebugLevelNotice()) {
               dumpSystemHeap();
            }

            rplSysHeapFree("RPL_MODULES_ARRAY",
                           sDynLoadData->modules,
                           4 * linkInfo->numModules);
            sDynLoadData->modules = nullptr;
            sDynLoadData->numModules = 0u;
         }
      }

      *outEntryModules = entryModules;

      auto itr = sDynLoadData->linkingRplList;
      auto prev = sDynLoadData->linkingRplList;
      sDynLoadData->numModules = 0u;
      while (true) {
         if (sDynLoadData->modules) {
            OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), itr->handle);
            sDynLoadData->modules[sDynLoadData->numModules] = itr;

            if (itr != rpx) {
               entryModules[(*outNumEntryModules)++] = itr;
            }
         }

         itr = itr->next;
         sDynLoadData->numModules++;
         if (!itr) {
            break;
         }

         prev = itr;
      }

      if (sDynLoadData->rplDataList &&
         (sDynLoadData->rplDataList->userFileInfo->fileInfoFlags & loader::rpl::RPL_IS_RPX)) {
         prev->next = sDynLoadData->rplDataList->next;
         sDynLoadData->rplDataList->next = sDynLoadData->linkingRplList;
      } else {
         prev->next = sDynLoadData->rplDataList;
         sDynLoadData->rplDataList = sDynLoadData->linkingRplList;
      }

      sDynLoadData->linkingRplList = nullptr;
   }

   rplSysHeapFree("RPL_MODULES_LINK_ARRAY", linkInfo, 0);
   return error;
}


/**
 * Prepare an RPL for loading.
 */
static OSDynLoad_Error
prepareLoad(virt_ptr<RPL_DATA> *ptrRplData,
            OSDynLoad_AllocFn dynLoadAlloc,
            virt_ptr<loader::LOADER_MinFileInfo> *ptrMinFileInfo)
{
   StackObject<virt_ptr<void>> allocPtr;
   auto rplData = *ptrRplData;
   auto minFileInfo = *ptrMinFileInfo;

   // Allocate section info
   rplData->sectionInfo = virt_cast<loader::LOADER_SectionInfo *>(
      rplSysHeapAlloc("RPL_SEC_INFO",
                      rplData->sectionInfoCount * sizeof(loader::LOADER_SectionInfo),
                      -4));
   if (!rplData->sectionInfo) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(minFileInfo->fileLocation,
                         OSDynLoad_Error::OutOfSysMemory, 0,
                         620, "sPrepareLoad");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   minFileInfo->outSectionInfo = rplData->sectionInfo;

   // Allocate file info
   if (auto error = dynLoadHeapAlloc("RPL_FILE_INFO", dynLoadAlloc,
                                     rplData->userFileInfoSize, 4, allocPtr)) {
      setFatalErrorInfo2(minFileInfo->fileLocation, error, 0,
                         637, "sPrepareLoad");
      return error;
   }

   rplData->userFileInfo = virt_cast<loader::LOADER_UserFileInfo *>(*allocPtr);

   if (isAppDebugLevelNotice()) {
      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_LAYOUT:{},FILE,start,=\"{}\"",
         rplData->moduleName,
         rplData->userFileInfo));

      COSInfo(COSReportModule::Unknown2, fmt::format(
         "RPL_LAYOUT:{},FILE,end,=\"{}\"",
         rplData->moduleName,
         virt_cast<virt_addr>(rplData->userFileInfo) + rplData->userFileInfoSize));
   }

   minFileInfo->outFileInfo = rplData->userFileInfo;
   sDynLoadData->numAllocations++;
   sDynLoadData->totalAllocatedBytes += rplData->userFileInfoSize;
   std::memset(rplData->userFileInfo.get(), 0, rplData->userFileInfoSize);

   // Allocate path string buffer
   if (minFileInfo->pathStringSize) {
      minFileInfo->pathStringBuffer = virt_cast<char *>(
         rplSysHeapAlloc("RPL_NOTIFY_NAME", minFileInfo->pathStringSize, 4));
      if (!minFileInfo->pathStringBuffer) {
         if (isAppDebugLevelNotice()) {
            dumpSystemHeap();
         }

         setFatalErrorInfo2(minFileInfo->fileLocation,
                            OSDynLoad_Error::OutOfSysMemory, 0,
                            666, "sPrepareLoad");
         return OSDynLoad_Error::OutOfSysMemory;
      }

      std::memset(minFileInfo->pathStringBuffer.get(), 0,
                  minFileInfo->pathStringSize);
   }

   if (minFileInfo->fileInfoFlags & loader::rpl::RPL_FLAG_4) {
      // Query the loader for file info
      if (auto error = kernel::loaderQuery(rplData->loaderHandle, minFileInfo)) {
         if (minFileInfo->fatalErr) {
            setFatalErrorInfo(
               minFileInfo->fatalMsgType,
               error,
               minFileInfo->error,
               minFileInfo->fatalLine,
               virt_addrof(minFileInfo->fatalFunction));
         }

         return OSDynLoad_Error::InvalidRPL;
      }
   } else {
      // Allocate data section
      if (minFileInfo->dataSize) {
         if (auto error = dynLoadHeapAlloc("RPL_DATA_AREA",
                                           dynLoadAlloc,
                                           minFileInfo->dataSize,
                                           minFileInfo->dataAlign,
                                           virt_addrof(rplData->dataSection))) {
            setFatalErrorInfo2(minFileInfo->fileLocation, error, false,
                               705, "sPrepareLoad");
            return error;
         }

         if (!kernel::validateAddressRange(virt_cast<virt_addr>(rplData->dataSection),
                                           minFileInfo->dataSize) ||
             !align_check(virt_cast<virt_addr>(rplData->dataSection),
                          minFileInfo->dataAlign)) {
            setFatalErrorInfo2(minFileInfo->fileLocation,
                               OSDynLoad_Error::InvalidAllocatedPtr, true,
                               701, "sPrepareLoad");
            return OSDynLoad_Error::InvalidAllocatedPtr;
         }

         rplData->dataSectionSize = minFileInfo->dataSize;
         minFileInfo->dataBuffer = rplData->dataSection;
         sDynLoadData->numAllocations++;
         sDynLoadData->totalAllocatedBytes += minFileInfo->dataSize;
      }

      // Allocate load section
      if (minFileInfo->loadSize) {
         if (auto error = dynLoadHeapAlloc("RPL_LOAD_INFO_AREA",
                                           dynLoadAlloc,
                                           minFileInfo->loadSize,
                                           minFileInfo->loadAlign,
                                           virt_addrof(rplData->loadSection))) {
            setFatalErrorInfo2(minFileInfo->fileLocation, error, false,
                               748, "sPrepareLoad");
            return error;
         }

         if (!kernel::validateAddressRange(virt_cast<virt_addr>(rplData->loadSection),
                                           minFileInfo->loadSize) ||
             !align_check(virt_cast<virt_addr>(rplData->loadSection),
                          minFileInfo->loadAlign)) {
            setFatalErrorInfo2(minFileInfo->fileLocation,
                               OSDynLoad_Error::InvalidAllocatedPtr, true,
                               752, "sPrepareLoad");
            return OSDynLoad_Error::InvalidAllocatedPtr;
         }

         rplData->loadSectionSize = minFileInfo->loadSize;
         minFileInfo->loadBuffer = rplData->loadSection;
         sDynLoadData->numAllocations++;
         sDynLoadData->totalAllocatedBytes += minFileInfo->loadSize;
      }

      // Run the loader setup
      if (auto error = kernel::loaderSetup(rplData->loaderHandle, minFileInfo)) {
         if (minFileInfo->fatalErr) {
            setFatalErrorInfo(
               minFileInfo->fatalMsgType,
               error,
               minFileInfo->error,
               minFileInfo->fatalLine,
               virt_addrof(minFileInfo->fatalFunction));
         }

         return OSDynLoad_Error::LoaderError;
      }
   }

   rplSysHeapFree("RPL_TEMP_DATA", minFileInfo,
                  sizeof(loader::LOADER_MinFileInfo));
   *ptrMinFileInfo = nullptr;

   findExports(rplData);

   if (!findTlsSection(rplData)) {
      COSError(COSReportModule::Unknown2,
               "*** Can not find section for TLS data.");
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation,
                         OSDynLoad_Error::TLSSectionNotFound, 0,
                         831, "sPrepareLoad");
      return OSDynLoad_Error::TLSSectionNotFound;
   }

   rplData->next = sDynLoadData->linkingRplList;
   sDynLoadData->linkingRplList = rplData;
   *ptrRplData = nullptr;
   return doImports(rplData);
}


/**
 * Release all importing modules.
 */
static void
releaseImports()
{
   auto inLinkingList =
      [](virt_ptr<RPL_DATA> rpl)
      {
         for (auto itr = sDynLoadData->linkingRplList; itr; itr = itr->next) {
            if (rpl == itr) {
               return true;
            }
         }

         return false;
      };

   for (auto module = sDynLoadData->linkingRplList; module; module = module->next) {
      for (auto i = 0u; i < module->importModuleCount; ++i) {
         auto importModule = module->importModules[i];
         if (!importModule) {
            continue;
         }

         if (inLinkingList(importModule)) {
            OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                             importModule->handle,
                             nullptr);
         } else {
            OSDynLoad_Release(importModule->handle);
         }
      }
   }
}


/**
 * Acquire a module.
 */
static OSDynLoad_Error
internalAcquire(virt_ptr<const char> name,
                virt_ptr<virt_ptr<RPL_DATA>> outRplData,
                virt_ptr<uint32_t> outNumEntryModules,
                virt_ptr<virt_ptr<virt_ptr<RPL_DATA>>> outEntryModules,
                bool doLoad)
{
   auto error = OSDynLoad_Error::OK;

   // Resolve module name
   auto[moduleName, moduleNameLength] = resolveModuleName(name);
   if (!moduleNameLength) {
      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         OSDynLoad_Error::EmptyModuleName, 0,
         949, "__OSDynLoad_InternalAcquire");
      return OSDynLoad_Error::EmptyModuleName;
   }

   // Allocate rpl name
   auto rplNameLength = align_up(moduleNameLength + 1, 4);
   auto rplName = virt_cast<char *>(
      rplSysHeapAlloc("RPL_NAME", rplNameLength, 4));
   if (!rplName) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         OSDynLoad_Error::OutOfSysMemory, 0,
         964, "__OSDynLoad_InternalAcquire");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   for (auto i = 0u; i < moduleNameLength; ++i) {
      rplName[i] = static_cast<char>(tolower(moduleName[i]));
   }
   rplName[moduleNameLength] = char { 0 };

   auto rplNameCleanup =
      gsl::finally([&]() {
         if (rplName) {
            rplSysHeapFree("RPL_NAME", rplName, rplNameLength);
         }
      });

   OSLockMutex(sDynLoad_LoaderLock);
   auto unlock =
      gsl::finally([&]() {
         OSUnlockMutex(sDynLoad_LoaderLock);
      });

   // We cannot call acquire whilst inside a module entry point
   if (sDynLoadData->inModuleEntryPoint) {
      error = OSDynLoad_Error::InModuleEntryPoint;
      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         error, true, 995, "__OSDynLoad_InternalAcquire");
      return error;
   }

   // Check if we already loaded this module
   for (auto rpl = sDynLoadData->rplDataList; rpl; rpl = rpl->next) {
      if (strcmp(rpl->moduleName.get(), rplName.get()) == 0) {
         OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), rpl->handle);
         *outRplData = rpl;
         return OSDynLoad_Error::OK;
      }
   }

   // Check if we are already loading this module
   for (auto rpl = sDynLoadData->linkingRplList; rpl; rpl = rpl->next) {
      if (strcmp(rpl->moduleName.get(), rplName.get()) == 0) {
         OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), rpl->handle);
         *outRplData = rpl;
         return OSDynLoad_Error::OK;
      }
   }

   // Verify we're not loading coreinit, that'd be a massive fuckup
   if (strcmp(rplName.get(), "coreinit") == 0) {
      COSError(COSReportModule::Unknown2, "*********");
      COSError(COSReportModule::Unknown2, fmt::format(
         "Error: Trying to load \"{}\".", rplName));
      COSError(COSReportModule::Unknown2, "*********");
      OSPanic("OSDynLoad_Acquire.c", 1035, "core library conflict.");
   }

   // Get the currently set allocator functions
   StackObject<OSDynLoad_AllocFn> dynLoadAlloc;
   StackObject<OSDynLoad_FreeFn> dynLoadFree;
   error = OSDynLoad_GetAllocator(dynLoadAlloc, dynLoadFree);
   if (error || !*dynLoadAlloc || !*dynLoadFree) {
      if (!error) {
         error = OSDynLoad_Error::InvalidAllocatorPtr;
      }

      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         error, false, 1047, "__OSDynLoad_InternalAcquire");
      return error;
   }

   // Allocate the notify callback array if needed
   auto firstLinkingRpl = !sDynLoadData->linkingRplList;
   auto notifyCallbackCount = 0u;
   auto notifyCallbackArray = virt_ptr<OSDynLoad_NotifyCallback> { nullptr };
   auto notifyCallbackArrayCleanup =
      gsl::finally([&]() {
         if (notifyCallbackArray) {
            rplSysHeapFree("RPL_NOTIFY_ARRAY",
                           notifyCallbackArray,
                           notifyCallbackCount * sizeof(OSDynLoad_NotifyCallback));
         }
      });

   if (firstLinkingRpl) {
      notifyCallbackCount = getNotifyCallbackCount();
      auto allocSize = static_cast<uint32_t>(notifyCallbackCount * sizeof(OSDynLoad_NotifyCallback));

      notifyCallbackArray = virt_cast<OSDynLoad_NotifyCallback *>(
         rplSysHeapAlloc("RPL_NOTIFY_ARRAY",
                         static_cast<uint32_t>(notifyCallbackCount * sizeof(OSDynLoad_NotifyCallback)),
                         4));
      if (!notifyCallbackArray) {
         setFatalErrorInfo2(
            sDynLoadData->rpxData->userFileInfo->titleLocation,
            OSDynLoad_Error::OutOfSysMemory, false,
            1077, "__OSDynLoad_InternalAcquire");
         return OSDynLoad_Error::OutOfSysMemory;
      }

      auto notifyCallback = sDynLoadData->notifyCallbacks;
      for (auto i = 0u; notifyCallback; ++i) {
         std::memcpy(virt_addrof(notifyCallbackArray[i]).get(),
                     notifyCallback.get(),
                     sizeof(OSDynLoad_NotifyCallback));

         notifyCallback = notifyCallback->next;
      }
   }

   // Allocate rpl data
   auto rplData = virt_cast<RPL_DATA *>(
      rplSysHeapAlloc("RPL_DATA", sizeof(RPL_DATA), 4));
   if (!rplData) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         OSDynLoad_Error::OutOfSysMemory, false,
         1105, "__OSDynLoad_InternalAcquire");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   auto rplDataCleanup =
      gsl::finally([&]() {
         if (rplData) {
            if (rplData->handle) {
               StackObject<uint32_t> handleRefCount;
               while (!OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                        rplData->handle,
                                        handleRefCount) && *handleRefCount);

               internalPurge(rplData);
            }

            rplSysHeapFree("RPL_DATA", rplData, sizeof(RPL_DATA));
         }
      });

   std::memset(rplData.get(), 0, sizeof(RPL_DATA));

   // Allocate min file info
   auto minFileInfo = virt_cast<loader::LOADER_MinFileInfo *>(
      rplSysHeapAlloc("RPL_TEMP_DATA",
                      sizeof(loader::LOADER_MinFileInfo), 4));
   if (!minFileInfo) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(
         sDynLoadData->rpxData->userFileInfo->titleLocation,
         OSDynLoad_Error::OutOfSysMemory, false,
         1131, "__OSDynLoad_InternalAcquire");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   auto minFileInfoCleanup =
      gsl::finally([&]() {
         if (minFileInfo) {
            rplSysHeapFree("RPL_TEMP_DATA", minFileInfo,
                           sizeof(loader::LOADER_MinFileInfo));
         }
      });

   // Do loader prep
   StackObject<loader::LOADER_Handle> kernelHandle;
   std::memset(minFileInfo.get(), 0, sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->version = 4u;
   minFileInfo->size = static_cast<uint32_t>(sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->outKernelHandle = kernelHandle;
   minFileInfo->moduleNameBufferLen = moduleNameLength;
   minFileInfo->moduleNameBuffer = rplName;
   minFileInfo->inoutNextTlsModuleNumber = virt_addrof(sDynLoadData->tlsModuleIndex);
   minFileInfo->outPathStringSize = virt_addrof(minFileInfo->pathStringSize);
   minFileInfo->outNumberOfSections = virt_addrof(rplData->sectionInfoCount);
   minFileInfo->outSizeOfFileInfo = virt_addrof(rplData->userFileInfoSize);

   if (auto error = kernel::loaderPrep(minFileInfo)) {
      if (minFileInfo->fatalErr) {
         setFatalErrorInfo(
            minFileInfo->fatalMsgType,
            minFileInfo->fatalErr,
            minFileInfo->error,
            minFileInfo->fatalLine,
            virt_addrof(minFileInfo->fatalFunction));
      }

      return static_cast<OSDynLoad_Error>(error);
   }

   auto kernelHandleCleanup =
      gsl::finally([&]() {
         if (*kernelHandle) {
            kernel::loaderPurge(*kernelHandle);
         }
      });

   // Assign tls header
   if (sDynLoadData->tlsModuleIndex > sDynLoadData->tlsHeader) {
      if (sDynLoadData->tlsModuleIndex > MaxTlsModuleIndex) {
         COSError(COSReportModule::Unknown2, fmt::format(
            "*** Too many RPLs have tls data; maximum 32k, needed {}",
            sDynLoadData->tlsModuleIndex));
         setFatalErrorInfo2(
            minFileInfo->fileLocation,
            OSDynLoad_Error::TLSTooManyModules, false,
            1178, "__OSDynLoad_InternalAcquire");

         return OSDynLoad_Error::TLSTooManyModules;
      }

      sDynLoadData->tlsHeader =
         std::min<uint32_t>(sDynLoadData->tlsModuleIndex + 8,
                              MaxTlsModuleIndex);
   }

   // Allocate a DynLoad handle for the new RPL
   if (auto error = OSHandle_Alloc(virt_addrof(sDynLoadData->handleTable),
                                   rplData, nullptr,
                                   virt_addrof(rplData->handle))) {
      setFatalErrorInfo2(minFileInfo->fileLocation, error, false,
                         1195, "__OSDynLoad_InternalAcquire");
      return static_cast<OSDynLoad_Error>(error);
   }

   // Prepare load
   *outRplData = rplData;
   rplData->dynLoadFreeFn = *dynLoadFree;
   rplData->loaderHandle = *kernelHandle;
   rplData->moduleName = rplName;
   rplData->moduleNameLen = moduleNameLength;
   *kernelHandle = nullptr;
   rplName = nullptr;

   error = prepareLoad(&rplData, *dynLoadAlloc, &minFileInfo);
   if (firstLinkingRpl) {
      if (error) {
         // If prepareLoad returned an error then we need to clean up rplData
         // in the case that prepareLoad did not clean it up itself
         if (!rplData && *outRplData) {
            StackObject<uint32_t> handleRefCount;
            while (!OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                     (*outRplData)->handle,
                                     handleRefCount) && *handleRefCount);
         }
      } else {
         error = executeDynamicLink(nullptr,
                                    outNumEntryModules,
                                    outEntryModules,
                                    *outRplData);
      }

      releaseImports();

      // Release any remaining RPLs still in the linking list
      while (sDynLoadData->linkingRplList) {
         auto rpl = sDynLoadData->linkingRplList;
         sDynLoadData->linkingRplList = rpl->next;
         rpl->next = nullptr;

         if (rpl->handle) {
            StackObject<uint32_t> handleRefCount;
            while(!OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                    rpl->handle,
                                    handleRefCount));
         }
      }
   }

   return error;
}


/**
 * __OSDynLoad_DoImports
 */
static OSDynLoad_Error
doImports(virt_ptr<RPL_DATA> rplData)
{
   // Get the section header string table
   auto shstrndx = rplData->userFileInfo->shstrndx;
   if (!rplData->userFileInfo->shstrndx) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "Error: Could not get section string table index for \"{}\".",
         rplData->moduleName));
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation,
                         OSDynLoad_Error::InvalidShStrNdx, true,
                         391, "__OSDynLoad_DoImports");
      return OSDynLoad_Error::InvalidShStrNdx;
   }

   auto shStrSection = virt_cast<char *>(rplData->sectionInfo[shstrndx].address);
   if (!shStrSection) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "Error: Could not get section string table for \"{}\".",
         rplData->moduleName));
      setFatalErrorInfo2(rplData->userFileInfo->titleLocation,
                         OSDynLoad_Error::InvalidShStrSection, true,
                         383, "__OSDynLoad_DoImports");
      return OSDynLoad_Error::InvalidShStrSection;
   }

   // Count the number of imported modules
   auto numImportModules = 0u;
   for (auto i = 0u; i < rplData->sectionInfoCount; ++i) {
      if (rplData->sectionInfo[i].address &&
          rplData->sectionInfo[i].name &&
          (rplData->sectionInfo[i].flags & loader::rpl::SHF_ALLOC) &&
          rplData->sectionInfo[i].type == loader::rpl::SHT_RPL_IMPORTS) {
         numImportModules++;
      }
   }

   if (!numImportModules) {
      return OSDynLoad_Error::OK;
   }

   // Allocate imported modules array
   rplData->importModuleCount = numImportModules;
   rplData->importModules = virt_cast<virt_ptr<RPL_DATA> *>(
      rplSysHeapAlloc("RPL_SEC_INFO",
                      sizeof(virt_ptr<RPL_DATA>) * numImportModules, 4));
   if (!rplData->importModules) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      setFatalErrorInfo2(rplData->userFileInfo->titleLocation,
                         OSDynLoad_Error::OutOfSysMemory, false,
                         423, "__OSDynLoad_DoImports");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   // Acquire all imported modules
   auto importModuleIdx = 0u;
   for (auto i = 0u; i < rplData->sectionInfoCount; ++i) {
      if (rplData->sectionInfo[i].address &&
          rplData->sectionInfo[i].name &&
          (rplData->sectionInfo[i].flags & loader::rpl::SHF_ALLOC) &&
          rplData->sectionInfo[i].type == loader::rpl::SHT_RPL_IMPORTS) {
         auto name = shStrSection + rplData->sectionInfo[i].name;

         if (isAppDebugLevelNotice()) {
            COSInfo(COSReportModule::Unknown2, fmt::format(
               "RPL_SYSHEAP:IMPORT START,{}", name));
            COSInfo(COSReportModule::Unknown2, fmt::format(
               "SYSTEM_HEAP:IMPORT START,{}", name));
         }

         auto error =
            internalAcquire(name + 9,
                            virt_addrof(rplData->importModules[importModuleIdx]),
                            0, 0, 0);

         if (isAppDebugLevelNotice()) {
            COSInfo(COSReportModule::Unknown2, fmt::format(
               "RPL_SYSHEAP:IMPORT END,{}", name));
            COSInfo(COSReportModule::Unknown2, fmt::format(
               "SYSTEM_HEAP:IMPORT END,{}", name));
         }

         if (error) {
            COSError(COSReportModule::Unknown2, fmt::format(
               "Error: Could not load imported RPL \"{}\".", name));
            return error;
         }

         auto importModule = rplData->importModules[importModuleIdx];
         if (!importModule->entryPoint) {
            // Ensure rplData comes after importModule in the linking list
            auto itr = virt_ptr<RPL_DATA> { nullptr };
            auto prev = virt_ptr<RPL_DATA> { nullptr };
            for (itr = sDynLoadData->linkingRplList; itr != rplData; itr = itr->next) {
               prev = itr;
            }

            while (itr = itr->next) {
               if (itr == importModule) {
                  if (prev) {
                     prev->next = rplData->next;
                  } else {
                     sDynLoadData->linkingRplList = rplData->next;
                  }

                  rplData->next = importModule->next;
                  importModule->next = rplData;
                  break;
               }
            }
         }

         ++importModuleIdx;
      }
   }

   rplSysHeapFree("RPL_SEC_INFO", rplData->sectionInfo,
                  rplData->sectionInfoCount * sizeof(loader::LOADER_SectionInfo));
   rplData->sectionInfo = nullptr;
   return OSDynLoad_Error::OK;
}

} // namespace internal


/**
 * Registers a callback to be called when an RPL is loaded or unloaded.
 */
OSDynLoad_Error
OSDynLoad_AddNotifyCallback(OSDynLoad_NotifyCallbackFn notifyFn,
                            virt_ptr<void> userArg1)
{
   if (!notifyFn) {
      return OSDynLoad_Error::InvalidAllocatorPtr;
   }

   auto notifyCallback = virt_cast<OSDynLoad_NotifyCallback *>(
      OSAllocFromSystem(sizeof(OSDynLoad_NotifyCallback), 4));
   if (!notifyCallback) {
      return OSDynLoad_Error::OutOfSysMemory;
   }

   OSLockMutex(sDynLoad_LoaderLock);
   notifyCallback->notifyFn = notifyFn;
   notifyCallback->userArg1 = userArg1;
   notifyCallback->next = sDynLoadData->notifyCallbacks;

   sDynLoadData->notifyCallbacks = notifyCallback;
   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::OK;
}


/**
 * Deletes a callback previously registered by OSDynLoad_AddNotifyCallback.
 */
void
OSDynLoad_DelNotifyCallback(OSDynLoad_NotifyCallbackFn notifyFn,
                            virt_ptr<void> userArg1)
{
   if (!notifyFn) {
      return;
   }

   OSLockMutex(sDynLoad_LoaderLock);

   // Find the callback
   auto prevCallback = virt_ptr<OSDynLoad_NotifyCallback> { nullptr };
   auto callback = sDynLoadData->notifyCallbacks;
   while (callback) {
      if (callback->notifyFn == notifyFn && callback->userArg1 == userArg1) {
         break;
      }

      prevCallback = callback;
      callback = callback->next;
   }

   if (callback) {
      // Erase it from linked list
      if (prevCallback) {
         prevCallback->next = callback->next;
      } else {
         sDynLoadData->notifyCallbacks = callback->next;
      }
   }

   OSUnlockMutex(sDynLoad_LoaderLock);

   // Free the callback
   if (callback) {
      OSFreeToSystem(callback);
   }
}


/**
 * Retrieve the allocator functions set by OSDynLoad_SetAllocator.
 */
OSDynLoad_Error
OSDynLoad_GetAllocator(virt_ptr<OSDynLoad_AllocFn> outAllocFn,
                       virt_ptr<OSDynLoad_FreeFn> outFreeFn)
{
   OSLockMutex(sDynLoad_LoaderLock);

   if (outAllocFn) {
      *outAllocFn = sDynLoadData->allocFn;
   }

   if (outFreeFn) {
      *outFreeFn = sDynLoadData->freeFn;
   }

   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::OK;
}


/**
 * Set the allocator which controls allocation of RPL segments.
 */
OSDynLoad_Error
OSDynLoad_SetAllocator(OSDynLoad_AllocFn allocFn,
                       OSDynLoad_FreeFn freeFn)
{
   if (!allocFn || !freeFn) {
      return OSDynLoad_Error::InvalidAllocatorPtr;
   }

   OSLockMutex(sDynLoad_LoaderLock);
   sDynLoadData->allocFn = allocFn;
   sDynLoadData->freeFn = freeFn;
   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::OK;
}


/**
 * Retrieve the allocator functions set by OSDynLoad_SetTLSAllocator.
 */
OSDynLoad_Error
OSDynLoad_GetTLSAllocator(virt_ptr<OSDynLoad_AllocFn> outAllocFn,
                          virt_ptr<OSDynLoad_FreeFn> outFreeFn)
{
   OSLockMutex(sDynLoad_LoaderLock);

   if (outAllocFn) {
      *outAllocFn = sDynLoadData->tlsAllocFn;
   }

   if (outFreeFn) {
      *outFreeFn = sDynLoadData->tlsFreeFn;
   }

   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::OK;
}


/**
 * Set the allocator which controls allocation of TLS memory.
 */
OSDynLoad_Error
OSDynLoad_SetTLSAllocator(OSDynLoad_AllocFn allocFn,
                          OSDynLoad_FreeFn freeFn)
{
   if (!allocFn || !freeFn) {
      return OSDynLoad_Error::InvalidAllocatorPtr;
   }

   if (sDynLoadData->tlsAllocLocked) {
      return OSDynLoad_Error::TLSAllocatorLocked;
   }

   OSLockMutex(sDynLoad_LoaderLock);
   sDynLoadData->tlsAllocFn = allocFn;
   sDynLoadData->tlsFreeFn = freeFn;
   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::OK;
}


/**
 * Acquire a module.
 *
 * If a module is already loaded this will increase it's ref count.
 * If a module is not yet loaded it will be loaded.
 */
OSDynLoad_Error
OSDynLoad_Acquire(virt_ptr<const char> modulePath,
                  virt_ptr<OSDynLoad_ModuleHandle> outModuleHandle)
{
   return internal::internalAcquire2(modulePath, outModuleHandle, FALSE);
}


/**
 * Release a module.
 *
 * Decreases ref count of module and deallocates it if ref count hits 0.
 */
void
OSDynLoad_Release(OSDynLoad_ModuleHandle moduleHandle)
{
   internal::release(moduleHandle, nullptr);
}


/**
 * Acquire the module which contains ptr.
 */
OSDynLoad_Error
OSDynLoad_AcquireContainingModule(virt_ptr<void> ptr,
                                  OSDynLoad_SectionType sectionType,
                                  virt_ptr<OSDynLoad_ModuleHandle> outHandle)
{
   if (!outHandle) {
      return OSDynLoad_Error::InvalidAcquirePtr;
   }

   if (!ptr) {
      return OSDynLoad_Error::InvalidContainPtr;
   }

   OSLockMutex(sDynLoad_LoaderLock);
   auto addr = virt_cast<virt_addr>(ptr);

   for (auto itr = sDynLoadData->rplDataList; itr; itr = itr->next) {
      if (!itr->notifyData) {
         continue;
      }

      if (sectionType == OSDynLoad_SectionType::Any ||
          sectionType == OSDynLoad_SectionType::DataOnly) {
         if (addr >= itr->notifyData->dataAddr &&
             addr <  itr->notifyData->dataAddr + itr->notifyData->dataSize) {
            if (itr == sDynLoadData->rpxData) {
               *outHandle = OSDynLoad_CurrentModuleHandle;
            } else {
               OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), itr->handle);
               *outHandle = itr->handle;
            }

            OSUnlockMutex(sDynLoad_LoaderLock);
            return OSDynLoad_Error::OK;
         }
      }

      if (sectionType == OSDynLoad_SectionType::Any ||
          sectionType == OSDynLoad_SectionType::CodeOnly) {
         if (addr >= itr->notifyData->textAddr &&
             addr <  itr->notifyData->textAddr + itr->notifyData->textSize) {
            if (itr == sDynLoadData->rpxData) {
               *outHandle = OSDynLoad_CurrentModuleHandle;
            } else {
               OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), itr->handle);
               *outHandle = itr->handle;
            }

            OSUnlockMutex(sDynLoad_LoaderLock);
            return OSDynLoad_Error::OK;
         }
      }
   }

   OSUnlockMutex(sDynLoad_LoaderLock);
   return OSDynLoad_Error::ContainModuleNotFound;
}


/**
 * Find an export from a library handle.
 */
OSDynLoad_Error
OSDynLoad_FindExport(OSDynLoad_ModuleHandle moduleHandle,
                     BOOL isData,
                     virt_ptr<const char> name,
                     virt_ptr<virt_addr> outAddr)
{
   virt_ptr<RPL_DATA> rplData = nullptr;
   auto exportData = virt_ptr<loader::rpl::Export> { nullptr };
   auto error = OSDynLoad_Error::OK;

   if (moduleHandle == OSDynLoad_CurrentModuleHandle) {
      rplData = sDynLoadData->rpxData;
   } else {
      StackObject<virt_ptr<void>> handleData;

      if (OSHandle_TranslateAndAddRef(virt_addrof(sDynLoadData->handleTable),
                                      moduleHandle,
                                      handleData,
                                      nullptr)) {
         return OSDynLoad_Error::InvalidHandle;
      }

      rplData = virt_cast<RPL_DATA *>(*handleData);
   }

   OSLockMutex(sDynLoad_LoaderLock);

   if (isData) {
      if (!rplData->dataExports) {
         error = OSDynLoad_Error::ModuleHasNoDataExports;
      } else {
         exportData =
            internal::binarySearchExport(
               virt_cast<loader::rpl::Export *>(rplData->dataExports),
               rplData->numDataExports,
               name);
      }
   } else {
      if (!rplData->codeExports) {
         error = OSDynLoad_Error::ModuleHasNoCodeExports;
      } else {
         exportData =
            internal::binarySearchExport(
               virt_cast<loader::rpl::Export *>(rplData->codeExports),
               rplData->numCodeExports,
               name);
      }
   }

   if (error == OSDynLoad_Error::OK) {
      if (!exportData) {
         error = OSDynLoad_Error::ExportNotFound;
      } else if (exportData->value & 0x80000000) {
         error = OSDynLoad_Error::TLSFindExportInvalid;
      } else {
         *outAddr = exportData->value;
      }
   }

   OSUnlockMutex(sDynLoad_LoaderLock);

   if (moduleHandle != OSDynLoad_CurrentModuleHandle) {
      OSHandle_Release(virt_addrof(sDynLoadData->handleTable), moduleHandle,
                       nullptr);
   }

   return error;
}


/**
 * Find an tag from a library handle.
 */
OSDynLoad_Error
OSDynLoad_FindTag(OSDynLoad_ModuleHandle moduleHandle,
                  virt_ptr<const char> tag,
                  virt_ptr<char> buffer,
                  virt_ptr<uint32_t> inoutBufferSize)
{
   return OSDynLoad_Error::InvalidParam;
}


/**
 * Get statistics about the loader memory heap.
 */
OSDynLoad_Error
OSDynLoad_GetLoaderHeapStatistics(virt_ptr<OSDynLoad_LoaderHeapStatistics> stats)
{
   return OSDynLoad_Error::InvalidParam;
}


/**
 * Get the module name for a module handle.
 */
OSDynLoad_Error
OSDynLoad_GetModuleName(OSDynLoad_ModuleHandle moduleHandle,
                        virt_ptr<char> buffer,
                        virt_ptr<uint32_t> inoutBufferSize)
{
   auto error = OSDynLoad_Error::OK;

   if (!moduleHandle) {
      return OSDynLoad_Error::InvalidHandle;
   }

   if (!inoutBufferSize ||
       !kernel::validateAddressRange(virt_cast<virt_addr>(inoutBufferSize), 4)) {
      return OSDynLoad_Error::InvalidParam;
   }

   if (moduleHandle == OSDynLoad_CurrentModuleHandle) {
      auto bufferLength = static_cast<uint32_t>(
         strlen(sDynLoadData->rpxName.get()) + 1);

      if (bufferLength > *inoutBufferSize) {
         *inoutBufferSize = bufferLength;
         error = OSDynLoad_Error::BufferTooSmall;
      } else {
         std::memcpy(buffer.get(),
                     sDynLoadData->rpxName.get(),
                     bufferLength - 1);
         buffer[bufferLength - 1] = char { 0 };
      }
   } else {
      StackObject<virt_ptr<void>> handleUserData1;

      if (OSHandle_TranslateAndAddRef(virt_addrof(sDynLoadData->handleTable),
                                      moduleHandle,
                                      handleUserData1,
                                      nullptr) != OSHandleError::OK) {
         return OSDynLoad_Error::InvalidHandle;
      }

      OSLockMutex(sDynLoad_LoaderLock);
      auto rplData = virt_cast<RPL_DATA *>(*handleUserData1);
      auto bufferLength = rplData->moduleNameLen + 1;

      if (bufferLength > *inoutBufferSize) {
         *inoutBufferSize = bufferLength;
         error = OSDynLoad_Error::BufferTooSmall;
      } else {
         std::memcpy(buffer.get(),
                     rplData->moduleName.get(),
                     bufferLength - 1);
         buffer[bufferLength - 1] = char { 0 };
      }

      OSUnlockMutex(sDynLoad_LoaderLock);
      OSHandle_Release(virt_addrof(sDynLoadData->handleTable), moduleHandle,
                       nullptr);
   }

   return error;
}


/**
 * Return the number of loaded RPLs.
 *
 * Will always return 0 on non-debug builds of CafeOS.
 */
uint32_t
OSDynLoad_GetNumberOfRPLs()
{
   return 0;
}


/**
 * Get the info for count RPLs beginning at first.
 */
uint32_t
OSDynLoad_GetRPLInfo(uint32_t first,
                     uint32_t count,
                     virt_ptr<OSDynLoad_NotifyData> outRplInfos)
{
   if (!count) {
      return 1;
   }

   return 0;
}


/**
 * Check if a module is loaded and return the handle of a dynamic library.
 */
OSDynLoad_Error
OSDynLoad_IsModuleLoaded(virt_ptr<const char> name,
                         virt_ptr<OSDynLoad_ModuleHandle> outHandle)
{
   return internal::internalAcquire2(name, outHandle, TRUE);
}


/**
 * Find a symbol name for the given address.
 *
 * \return
 * Returns the address of the nearest symbol.
 */
virt_addr
OSGetSymbolName(virt_addr address,
                virt_ptr<char> buffer,
                uint32_t bufferSize)
{
   StackObject<uint32_t> symbolDistance;
   StackArray<char, 256> symbolNameBuffer;
   StackArray<char, 256> moduleNameBuffer;

   *buffer = char { 0 };
   if (bufferSize < 16) {
      return address;
   }

   symbolNameBuffer[0] = char { 0 };
   moduleNameBuffer[0] = char { 0 };

   auto error = kernel::findClosestSymbol(address,
                                          symbolDistance,
                                          symbolNameBuffer,
                                          symbolNameBuffer.size(),
                                          moduleNameBuffer,
                                          moduleNameBuffer.size());

   if (error || (!symbolNameBuffer[0] && !moduleNameBuffer[0])) {
      string_copy(buffer.get(), "unknown", bufferSize);
      buffer[bufferSize - 1] = char { 0 };
      return address;
   }

   auto symbolAddress = address - *symbolDistance;
   auto moduleNameLength = strlen(moduleNameBuffer.get());
   auto symbolNameLength = strlen(symbolNameBuffer.get());

   if (moduleNameLength) {
      string_copy(buffer.get(),
                  moduleNameBuffer.get(),
                  bufferSize);

      if (moduleNameLength + 1 >= bufferSize) {
         buffer[bufferSize - 1] = char { 0 };
         return symbolAddress;
      }

      if (symbolNameLength) {
         buffer[moduleNameLength] = char { '|' };
         moduleNameLength++;
      }
   }

   if (symbolNameLength) {
      string_copy(buffer.get() + moduleNameLength,
                  symbolNameBuffer.get(),
                  bufferSize - moduleNameLength);
   }

   return symbolAddress;
}


/**
 * __tls_get_addr
 * Gets the TLS data for tls_index.
 */
virt_ptr<void>
tls_get_addr(virt_ptr<tls_index> index)
{
   if (!sDynLoadData->tlsHeader) {
      internal::COSError(COSReportModule::Unknown2,
                         "*** __OSDynLoad_gTLSHeader not initialized.");
      internal::OSPanic("OSDynLoad_Acquire.c", 299,
                        "__OSDynLoad_gTLSHeader not initialized.");
   }

   if (index->moduleIndex < 0) {
      internal::COSError(COSReportModule::Unknown2,
                         "*** __OSDynLoad_gTLSHeader not initialized.");
      internal::OSPanic("OSDynLoad_Acquire.c", 304,
                        "__OSDynLoad_gTLSHeader not initialized.");
   }

   if (sDynLoadData->tlsHeader <= index->moduleIndex) {
      internal::COSError(COSReportModule::Unknown2,
                         "*** __OSDynLoad_gTLSHeader not initialized.");
      internal::OSPanic("OSDynLoad_Acquire.c", 309,
                        "__OSDynLoad_gTLSHeader not initialized.");
   }

   auto thread = OSGetCurrentThread();
   if (thread->tlsSectionCount <= index->moduleIndex) {
      StackObject<virt_ptr<void>> allocPtr;

      // Allocate new TLS section data
      if (auto error = cafe::invoke(cpu::this_core::state(),
                                    sDynLoadData->tlsAllocFn,
                                    static_cast<uint32_t>(sizeof(OSTLSSection) * sDynLoadData->tlsHeader),
                                    4,
                                    allocPtr)) {
         internal::COSError(COSReportModule::Unknown2,
                            "*** Couldn't allocate internal TLS framework blocks.");
         internal::OSPanic("OSDynLoad_Acquire.c", 317, "out of memory.");
      }

      std::memset(allocPtr->get(),
                  0,
                  sizeof(OSTLSSection) * sDynLoadData->tlsHeader);

      // Copy and free old TLS section data
      if (thread->tlsSectionCount) {
         std::memcpy(allocPtr->get(),
                     thread->tlsSections.get(),
                     sizeof(OSTLSSection) * sDynLoadData->tlsHeader);

         cafe::invoke(cpu::this_core::state(),
                      sDynLoadData->tlsFreeFn,
                      thread->tlsSections);
      }

      thread->tlsSections = virt_cast<OSTLSSection *>(*allocPtr);
      thread->tlsSectionCount = sDynLoadData->tlsHeader;
   }

   if (!thread->tlsSections[index->moduleIndex].data) {
      // Find the module for this tls index
      auto module = virt_ptr<RPL_DATA> { nullptr };
      for (module = sDynLoadData->rplDataList; module; module = module->next) {
         if (module->userFileInfo->tlsModuleIndex == index->moduleIndex) {
            break;
         }
      }

      if (!module ||
          !(module->userFileInfo->fileInfoFlags & loader::rpl::RPL_HAS_TLS) ||
          !module->userFileInfo->tlsAddressStart ||
          !module->userFileInfo->tlsSectionSize) {
         internal::COSError(COSReportModule::Unknown2,
                            "*** Can not find module for TLS data.");
         internal::OSPanic("OSDynLoad_Acquire.c", 343,
                           "Can not find module for TLS data.");
      }

      // Allocate tls image for this module
      StackObject<virt_ptr<void>> allocPtr;
      if (auto error = cafe::invoke(cpu::this_core::state(),
                                    sDynLoadData->tlsAllocFn,
                                    module->userFileInfo->tlsSectionSize,
                                    1u << module->userFileInfo->tlsAlignShift,
                                    allocPtr)) {
         internal::COSError(COSReportModule::Unknown2,
                            "*** Couldn't allocate thread TLS image.");
         internal::OSPanic("OSDynLoad_Acquire.c", 352, "out of memory.");
      }

      std::memcpy(allocPtr->get(),
                  virt_cast<void *>(module->userFileInfo->tlsAddressStart).get(),
                  module->userFileInfo->tlsSectionSize);
      thread->tlsSections[index->moduleIndex].data = *allocPtr;
   }

   return
      virt_cast<void *>(
         virt_cast<virt_addr>(thread->tlsSections[index->moduleIndex].data)
         + index->offset);
}

namespace internal
{

void
dynLoadTlsFree(virt_ptr<OSThread> thread)
{
   if (thread->tlsSectionCount) {
      for (auto i = 0u; i < thread->tlsSectionCount; ++i) {
         cafe::invoke(cpu::this_core::state(),
                      sDynLoadData->tlsFreeFn,
                      thread->tlsSections[i].data);
      }

      cafe::invoke(cpu::this_core::state(),
                   sDynLoadData->tlsFreeFn,
                   virt_cast<void *>(thread->tlsSections));
   }
}

static void
initCoreinitNotifyData(virt_ptr<RPL_DATA> rpl)
{
   StackObject<kernel::Info0> info0;
   kernel::getInfo(kernel::InfoType::Type0, info0, sizeof(kernel::Info0));

   rpl->notifyData = virt_cast<OSDynLoad_NotifyData *>(
      rplSysHeapAlloc("RPL_NOTIFY",
                      sizeof(OSDynLoad_NotifyData), 4));
   if (!rpl) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2,
               fmt::format("sInitNotifyHdr() - mem alloc failed (err=0x{:08X}).",
                           OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(rpl->userFileInfo->titleLocation,
                         OSDynLoad_Error::OutOfSysMemory, 0,
                         326, "sInitNotifyHdr");
      reportFatalError();
   }

   std::memset(rpl->notifyData.get(), 0, sizeof(OSDynLoad_NotifyData));

   // Steal the path string!
   rpl->notifyData->name = rpl->userFileInfo->pathString;
   rpl->userFileInfo->pathString = nullptr;

   // Update section info
   rpl->notifyData->textAddr = info0->coreinit.textAddr;
   rpl->notifyData->textOffset = info0->coreinit.textOffset;
   rpl->notifyData->textSize = info0->coreinit.textSize;

   rpl->notifyData->dataAddr = info0->coreinit.dataAddr;
   rpl->notifyData->dataOffset = info0->coreinit.dataOffset;
   rpl->notifyData->dataSize = info0->coreinit.dataSize;

   rpl->notifyData->readAddr = info0->coreinit.dataAddr;
   rpl->notifyData->readOffset = info0->coreinit.dataOffset;
   rpl->notifyData->readSize = info0->coreinit.dataSize;
   // TODO: MasterAgent_LoadNotify
}


/**
 * __OSDynLoad_InitFromCoreInit
 */
virt_addr
initialiseDynLoad()
{
   resetFatalErrorInfo();
   if (auto error = initialiseCommon()) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "OSDynLoad_InitCommon() failed with err 0x{:08X}\n", error));
      reportFatalError();
   }

   // Allocate rpl data for coreinit
   auto coreinitRplData = virt_cast<RPL_DATA *>(
      rplSysHeapAlloc("RPL_DATA", sizeof(RPL_DATA), 4));
   if (!coreinitRplData) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - mem alloc failed (err=0x{:08X}).",
         OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(0, OSDynLoad_Error::OutOfSysMemory, false,
                         440, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   std::memset(coreinitRplData.get(), 0, sizeof(RPL_DATA));

   // Allocate handle for coreinit
   if (auto handleError = OSHandle_Alloc(virt_addrof(sDynLoadData->handleTable),
                                         coreinitRplData,
                                         nullptr,
                                         virt_addrof(coreinitRplData->handle))) {
      decaf_abort(fmt::format("Unexpected OSHandle_Alloc error = {}",
                              handleError));
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - handle alloc failed (err=0x{:08X}.", handleError));
      setFatalErrorInfo2(0, handleError, 0, 452,
                         "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   // Call setupPerm for coreinit
   sDynLoadData->coreinitModuleName = "coreinit";
   coreinitRplData->moduleName = virt_addrof(sDynLoadData->coreinitModuleName);
   coreinitRplData->moduleNameLen = 8u;
   coreinitRplData->loaderHandle = getCoreinitLoaderHandle();
   coreinitRplData->next = sDynLoadData->rplDataList;
   sDynLoadData->rplDataList = coreinitRplData;
   setupPerm(coreinitRplData, false);

   rplSysHeapFree("RPL_SEC_INFO", coreinitRplData->sectionInfo,
                  coreinitRplData->sectionInfoCount * sizeof(loader::LOADER_SectionInfo));
   coreinitRplData->sectionInfo = nullptr;

   sDynLoadData->tlsAllocLocked = FALSE;
   initialiseDefaultHeap(coreinitRplData,
                         make_stack_string("CoreInitDefaultHeap"));

   // Allocate rpx data
   auto rpxData = virt_cast<RPL_DATA *>(
      rplSysHeapAlloc("RPL_DATA", sizeof(RPL_DATA), 4));
   if (!rpxData) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - mem alloc failed (err=0x{:08X}).",
         OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(0, OSDynLoad_Error::OutOfSysMemory, false,
                         493, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   std::memset(rpxData.get(), 0, sizeof(RPL_DATA));
   sDynLoadData->rpxData = rpxData;

   // Allocate module name
   rpxData->handle = 0xFFFFFFFFu;
   rpxData->moduleName = virt_cast<char *>(
      rplSysHeapAlloc("RPL_NAME", sDynLoadData->rpxNameLength + 1, 4));
   rpxData->moduleNameLen = sDynLoadData->rpxNameLength;
   if (!rpxData->moduleName) {
      if (isAppDebugLevelNotice()) {
         dumpSystemHeap();
      }

      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - mem alloc failed (err=0x{:08X}).",
         OSDynLoad_Error::OutOfSysMemory));
      setFatalErrorInfo2(0, OSDynLoad_Error::OutOfSysMemory, false,
                         514, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   std::memcpy(rpxData->moduleName.get(),
               sDynLoadData->rpxName.get(),
               rpxData->moduleNameLen);
   rpxData->moduleName[rpxData->moduleNameLen] = char { 0 };

   // Call setupPerm for rpx
   setupPerm(rpxData, true);

   if (sDynLoadData->tlsModuleIndex) {
      sDynLoadData->tlsHeader = sDynLoadData->tlsModuleIndex + 8;
   }

   sDynLoadData->linkingRplList = rpxData;
   rpxData->entryPoint = virt_addr { 0u };
   if (auto error = doImports(rpxData)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - main program load failed (err=0x{:08X}).",
         error));
      setFatalErrorInfo2(rpxData->userFileInfo->titleLocation, error, true,
                         540, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   StackObject<uint32_t> numEntryModules;
   StackObject<virt_ptr<virt_ptr<RPL_DATA>>> entryModules;
   if (auto error = executeDynamicLink(rpxData,
                                       numEntryModules, entryModules,
                                       rpxData)) {
      COSError(COSReportModule::Unknown2, fmt::format(
         "__OSDynLoad_InitFromCoreInit() - dynamic link of main program failed (err=0x{:08X}).",
         error));
      setFatalErrorInfo2(rpxData->userFileInfo->titleLocation, error, true,
                         549, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   if (coreinitRplData->userFileInfo->pathString) {
      initCoreinitNotifyData(coreinitRplData);
   }

   kernel::loaderUserGainControl();

   if (decaf::config::debugger::enabled) {
      StackObject<virt_addr> preinitAddr;
      auto error = OSDynLoad_FindExport(rpxData->handle, FALSE,
                                        make_stack_string("__preinit_user"),
                                        preinitAddr);
      if (error != OSDynLoad_Error::OK) {
         *preinitAddr = virt_addr { 0 };
      }

      decaf::debug::notifyEntry(static_cast<uint32_t>(*preinitAddr),
                                static_cast<uint32_t>(rpxData->entryPoint));
   }

   initialiseDefaultHeap(rpxData,
                         make_stack_string("__preinit_user"));

   sDynLoadData->tlsAllocLocked = TRUE;

   if (sDynLoadData->preInitMem1Heap) {
      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1,
                           sDynLoadData->preInitMem1Heap);
   }

   if (sDynLoadData->preInitForegroundHeap) {
      MEMSetBaseHeapHandle(MEMBaseHeapType::FG,
                           sDynLoadData->preInitForegroundHeap);
   }

   if (sDynLoadData->preInitMem2Heap) {
      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2,
                           sDynLoadData->preInitMem2Heap);
   }

   initialiseGhs();
   if (auto error = runEntryPoints(*numEntryModules, *entryModules)) {
      COSError(COSReportModule::Unknown2, fmt::format(
               "__OSDynLoad_InitFromCoreInit() - initialization functions of main program failed (err=0x{:08X}).",
               error));
      reportFatalError();
   }

   if (sDynLoadData->modules) {
      rplSysHeapFree("RPL_MODULES_ARRAY", sDynLoadData->modules,
                     sDynLoadData->numModules);
   }

   sDynLoadData->modules = nullptr;
   sDynLoadData->numModules = 0u;

   if (!rpxData->entryPoint) {
      COSError(COSReportModule::Unknown2,
               "__OSDynLoad_InitFromCoreInit() - main program is NULL.");
      setFatalErrorInfo2(rpxData->userFileInfo->titleLocation,
                         OSDynLoad_Error::InvalidRpxEntryPoint, true,
                         643, "__OSDynLoad_InitFromCoreInit");
      reportFatalError();
   }

   return rpxData->entryPoint;
}


/**
 * Relocate HLE variables to the loaded addresses for a given module.
 */
OSDynLoad_Error
relocateHleLibrary(OSDynLoad_ModuleHandle moduleHandle)
{
   StackObject<virt_ptr<void>> handleData;
   if (OSHandle_TranslateAndAddRef(virt_addrof(sDynLoadData->handleTable),
                                   moduleHandle,
                                   handleData,
                                   nullptr)) {
      return OSDynLoad_Error::InvalidHandle;
   }

   auto rplData = virt_cast<RPL_DATA *>(*handleData);
   cafe::hle::relocateLibrary(
      std::string_view { rplData->moduleName.get(),
                         rplData->moduleNameLen },
      rplData->notifyData->textAddr,
      rplData->notifyData->dataAddr
   );

   return OSDynLoad_Error::OK;
}

} // namespace internal

void
Library::registerDynLoadSymbols()
{
   RegisterFunctionExport(OSDynLoad_AddNotifyCallback);
   RegisterFunctionExport(OSDynLoad_DelNotifyCallback);
   RegisterFunctionExportName("OSDynLoad_AddNofifyCallback", // covfefe
                              OSDynLoad_AddNotifyCallback);

   RegisterFunctionExport(OSDynLoad_GetAllocator);
   RegisterFunctionExport(OSDynLoad_SetAllocator);
   RegisterFunctionExport(OSDynLoad_GetTLSAllocator);
   RegisterFunctionExport(OSDynLoad_SetTLSAllocator);

   RegisterFunctionExport(OSDynLoad_Acquire);
   RegisterFunctionExport(OSDynLoad_AcquireContainingModule);
   RegisterFunctionExport(OSDynLoad_FindExport);
   RegisterFunctionExport(OSDynLoad_FindTag);
   RegisterFunctionExport(OSDynLoad_GetLoaderHeapStatistics);
   RegisterFunctionExport(OSDynLoad_GetModuleName);
   RegisterFunctionExport(OSDynLoad_GetNumberOfRPLs);
   RegisterFunctionExport(OSDynLoad_GetRPLInfo);
   RegisterFunctionExport(OSDynLoad_IsModuleLoaded);
   RegisterFunctionExport(OSDynLoad_Release);

   RegisterDataExportName("OSDynLoad_gLoaderLock", sDynLoad_LoaderLock);

   RegisterFunctionExport(OSGetSymbolName);
   RegisterFunctionExportName("__tls_get_addr", tls_get_addr);

   RegisterDataInternal(sDynLoadData);
}

} // namespace cafe::coreinit
