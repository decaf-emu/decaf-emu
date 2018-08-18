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
#include "cafe/kernel/cafe_kernel_loader.h"
#include "debugger/debugger.h"

namespace cafe::coreinit
{

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
   be2_struct<OSMutex> mutex;

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
   be2_virt_ptr<char> rpxName;
   be2_val<uint32_t> rpxNameLength;

   be2_virt_ptr<RPL_DATA> acquiringRpl;
   be2_virt_ptr<RPL_DATA> linkingRplList;

   be2_val<MEMHeapHandle> preInitMem1Heap;
   be2_val<MEMHeapHandle> preInitMem2Heap;
   be2_val<MEMHeapHandle> preInitForegroundHeap;

   be2_val<uint32_t> numAllocations;
   be2_val<uint32_t> totalAllocatedBytes;

   be2_val<uint32_t> numModules;
   be2_virt_ptr<virt_ptr<RPL_DATA>> modules;
};

static virt_ptr<StaticDynLoadData>
sDynLoadData = nullptr;


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

   OSLockMutex(virt_addrof(sDynLoadData->mutex));
   notifyCallback->notifyFn = notifyFn;
   notifyCallback->userArg1 = userArg1;
   notifyCallback->next = sDynLoadData->notifyCallbacks;

   sDynLoadData->notifyCallbacks = notifyCallback;
   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
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

   OSLockMutex(virt_addrof(sDynLoadData->mutex));

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

   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));

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
   OSLockMutex(virt_addrof(sDynLoadData->mutex));

   if (outAllocFn) {
      *outAllocFn = sDynLoadData->allocFn;
   }

   if (outFreeFn) {
      *outFreeFn = sDynLoadData->freeFn;
   }

   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
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

   OSLockMutex(virt_addrof(sDynLoadData->mutex));
   sDynLoadData->allocFn = allocFn;
   sDynLoadData->freeFn = freeFn;
   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
   return OSDynLoad_Error::OK;
}


/**
 * Retrieve the allocator functions set by OSDynLoad_SetTLSAllocator.
 */
OSDynLoad_Error
OSDynLoad_GetTLSAllocator(virt_ptr<OSDynLoad_AllocFn> outAllocFn,
                          virt_ptr<OSDynLoad_FreeFn> outFreeFn)
{
   OSLockMutex(virt_addrof(sDynLoadData->mutex));

   if (outAllocFn) {
      *outAllocFn = sDynLoadData->tlsAllocFn;
   }

   if (outFreeFn) {
      *outFreeFn = sDynLoadData->tlsFreeFn;
   }

   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
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

   OSLockMutex(virt_addrof(sDynLoadData->mutex));
   sDynLoadData->tlsAllocFn = allocFn;
   sDynLoadData->tlsFreeFn = freeFn;
   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
   return OSDynLoad_Error::OK;
}


OSDynLoad_Error
OSDynLoad_Acquire(virt_ptr<const char> modulePath,
                  virt_ptr<OSDynLoad_ModuleHandle> outModuleHandle)
{
   return OSDynLoad_Error::OutOfMemory;
}


OSDynLoad_Error
OSDynLoad_Release(OSDynLoad_ModuleHandle moduleHandle)
{
   return OSDynLoad_Error::OutOfMemory;
}


OSDynLoad_Error
OSDynLoad_AcquireContainingModule(virt_ptr<void> ptr,
                                  OSDynLoad_SectionType sectionType,
                                  virt_ptr<OSDynLoad_ModuleHandle> outHandle)
{
   return OSDynLoad_Error::OK;
}


static virt_ptr<loader::rpl::Export>
searchExport(virt_ptr<loader::rpl::Export> exports,
             uint32_t numExports,
             virt_ptr<const char> name)
{
   auto exportNames = virt_cast<const char *>(exports) - 8;
   auto left = 0u;
   auto right = numExports;

   while (true) {
      auto index = left + (right - left) / 2;
      auto exportName = exportNames.getRawPointer() + (exports[index].name & 0x7FFFFFFF);
      auto cmpValue = strcmp(name.getRawPointer(), exportName);
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

   OSLockMutex(virt_addrof(sDynLoadData->mutex));

   if (isData) {
      if (!rplData->dataExports) {
         error = OSDynLoad_Error::ModuleHasNoDataExports;
      } else {
         exportData = searchExport(virt_cast<loader::rpl::Export *>(rplData->dataExports),
                                   rplData->numDataExports,
                                   name);
      }
   } else {
      if (!rplData->codeExports) {
         error = OSDynLoad_Error::ModuleHasNoCodeExports;
      } else {
         exportData = searchExport(virt_cast<loader::rpl::Export *>(rplData->codeExports),
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

   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));

   if (moduleHandle != OSDynLoad_CurrentModuleHandle) {
      OSHandle_Release(virt_addrof(sDynLoadData->handleTable), moduleHandle,
                       nullptr);
   }

   return error;
}


OSDynLoad_Error
OSDynLoad_GetModuleName(OSDynLoad_ModuleHandle moduleHandle,
                        virt_ptr<char> buffer,
                        uint32_t bufferSize)
{
   return OSDynLoad_Error::OutOfMemory;
}


/**
 * Check if a module is loaded and return the handle of a dynamic library.
 */
OSDynLoad_Error
OSDynLoad_IsModuleLoaded(virt_ptr<const char> name,
                         virt_ptr<OSDynLoad_ModuleHandle> outHandle)
{
   return OSDynLoad_Error::OutOfMemory;
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
      std::strncpy(buffer.getRawPointer(), "unknown", bufferSize);
      buffer[bufferSize - 1] = char { 0 };
      return address;
   }

   auto symbolAddress = address - *symbolDistance;
   auto moduleNameLength = strlen(moduleNameBuffer.getRawPointer());
   auto symbolNameLength = strlen(symbolNameBuffer.getRawPointer());

   if (moduleNameLength) {
      std::strncpy(buffer.getRawPointer(),
                   moduleNameBuffer.getRawPointer(),
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
      std::strncpy(buffer.getRawPointer() + moduleNameLength,
                   symbolNameBuffer.getRawPointer(),
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
                                    sizeof(OSTLSSection) * sDynLoadData->tlsHeader,
                                    4,
                                    allocPtr)) {
         internal::COSError(COSReportModule::Unknown2,
                            "*** Couldn't allocate internal TLS framework blocks.");
         internal::OSPanic("OSDynLoad_Acquire.c", 317, "out of memory.");
      }

      std::memset(allocPtr->getRawPointer(),
                  0,
                  sizeof(OSTLSSection) * sDynLoadData->tlsHeader);

      // Copy and free old TLS section data
      if (thread->tlsSectionCount) {
         std::memcpy(allocPtr->getRawPointer(),
                     thread->tlsSections.getRawPointer(),
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

      std::memcpy(allocPtr->getRawPointer(),
                  virt_cast<void *>(module->userFileInfo->tlsAddressStart).getRawPointer(),
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

static void
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

      decaf_check(tlsAddressStart != virt_addr { 0xFFFFFFFFu });
      decaf_check(tlsAddressEnd != virt_addr { 0 });

      rplData->userFileInfo->tlsAddressStart = tlsAddressStart;
      rplData->userFileInfo->tlsSectionSize =
         static_cast<uint32_t>(tlsAddressEnd - tlsAddressStart);
   }
}

static int32_t
sSetupPerm(virt_ptr<RPL_DATA> rplData,
           bool updateTlsModuleIndex)
{
   StackObject<loader::LOADER_MinFileInfo> minFileInfo;
   std::memset(minFileInfo.getRawPointer(), 0, sizeof(loader::LOADER_MinFileInfo));
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
   decaf_check(!error);

   rplData->sectionInfo = virt_cast<loader::LOADER_SectionInfo *>(
      OSAllocFromSystem(
         sizeof(loader::LOADER_SectionInfo) * rplData->sectionInfoCount,
         -4));
   decaf_check(rplData->sectionInfo);
   minFileInfo->outSectionInfo = rplData->sectionInfo;

   rplData->userFileInfo = virt_cast<loader::LOADER_UserFileInfo *>(
      OSAllocFromSystem(rplData->userFileInfoSize, 4));
   decaf_check(rplData->userFileInfo);
   minFileInfo->outFileInfo = rplData->userFileInfo;
   std::memset(rplData->userFileInfo.getRawPointer(), 0,
               sizeof(loader::LOADER_UserFileInfo));

   if (minFileInfo->pathStringSize) {
      minFileInfo->pathStringBuffer = virt_cast<char *>(
         OSAllocFromSystem(minFileInfo->pathStringSize, 4));
      decaf_check(minFileInfo->pathStringBuffer);
      std::memset(minFileInfo->pathStringBuffer.getRawPointer(), 0,
                  minFileInfo->pathStringSize);
   }

   // Query to get the data
   error = kernel::loaderQuery(rplData->loaderHandle, minFileInfo);
   decaf_check(!error);

   findExports(rplData);
   findTlsSection(rplData);
   return 0;
}

using InitDefaultHeapFn = virt_func_ptr<
   void(virt_ptr<MEMHeapHandle> mem1,
        virt_ptr<MEMHeapHandle> foreground,
        virt_ptr<MEMHeapHandle> mem2)>;

static OSDynLoad_Error
initialiseDefaultHeap(virt_ptr<RPL_DATA> rplData,
                      virt_ptr<const char> functionName)
{
   StackObject<virt_addr> funcAddr;
   auto error = OSDynLoad_FindExport(rplData->handle, FALSE, functionName, funcAddr);
   if (error != OSDynLoad_Error::OK) {
      return error;
   }

   cafe::invoke(cpu::this_core::state(),
                virt_func_cast<InitDefaultHeapFn>(*funcAddr),
                virt_addrof(sDynLoadData->preInitMem1Heap),
                virt_addrof(sDynLoadData->preInitForegroundHeap),
                virt_addrof(sDynLoadData->preInitMem2Heap));

   StackObject<OSDynLoad_AllocFn> allocFn;
   StackObject<OSDynLoad_FreeFn> freeFn;
   error = OSDynLoad_GetAllocator(allocFn, freeFn);
   if (error != OSDynLoad_Error::OK || !(*allocFn) || !(*freeFn)) {
      decaf_abort(fmt::format("{} did not set OSDynLoad allocator",
                              functionName.getRawPointer()));
   }

   error = OSDynLoad_GetTLSAllocator(allocFn, freeFn);
   if (error != OSDynLoad_Error::OK || !(*allocFn) || !(*freeFn)) {
      decaf_abort(fmt::format("{} did not set OSDynLoad TLS allocator",
                              functionName.getRawPointer()));
   }

   return OSDynLoad_Error::OK;
}

static std::pair<virt_ptr<char>, uint32_t>
resolveModuleName(virt_ptr<char> name)
{
   auto str = std::string_view { name.getRawPointer() };
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
static void
initialiseCommon()
{
   OSInitMutex(virt_addrof(sDynLoadData->mutex));
   OSHandle_InitTable(virt_addrof(sDynLoadData->handleTable));

   // Parse argstr into rpx name
   std::tie(sDynLoadData->rpxName, sDynLoadData->rpxNameLength) = resolveModuleName(getArgStr());
   if (sDynLoadData->rpxNameLength > 63u) {
      sDynLoadData->rpxNameLength = 63u;
   }
}

static OSDynLoad_Error
buildLinkInfo(virt_ptr<RPL_DATA> linkList,
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
      OSAllocFromSystem(linkInfoSize, -4));
   if (!linkInfo) {
      decaf_abort("error");
      // SetFatalErrorInfo2(rpl->fileInfo->titleLocation, 0xBAD1002F, 0, 101, "__BuildKernelNotify");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   std::memset(linkInfo.getRawPointer(), 0, linkInfoSize);
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

// _OSDynLoad_ExecuteDynamicLink
static int32_t
executeDynamicLink(virt_ptr<RPL_DATA> rpx,
                   virt_ptr<uint32_t> outNumEntryModules,
                   virt_ptr<virt_ptr<virt_ptr<RPL_DATA>>> outEntryModules,
                   virt_ptr<RPL_DATA> rpl)
{
   StackObject<loader::LOADER_MinFileInfo> minFileInfo;
   auto linkInfo = virt_ptr<loader::LOADER_LinkInfo> { nullptr };
   auto error = 0;

   *outNumEntryModules = 0u;
   *outEntryModules = nullptr;

   std::memset(minFileInfo.getRawPointer(), 0,
               sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->size = static_cast<uint32_t>(sizeof(loader::LOADER_MinFileInfo));
   minFileInfo->version = 4u;
   if (error = buildLinkInfo(sDynLoadData->linkingRplList, &linkInfo, rpl)) {
      decaf_abort("error");
      return error;
   }

   error = kernel::loaderLink(nullptr, minFileInfo, linkInfo, linkInfo->size);
   if (error) {
      // error
      decaf_abort("error");
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
            // ERROR
            decaf_abort("error");
         }

         rplData->entryPoint = linkModule.entryPoint;
         rplData->notifyData = virt_cast<OSDynLoad_NotifyData *>(
            OSAllocFromSystem(sizeof(OSDynLoad_NotifyData), 4));
         if (rplData->notifyData) {
            auto notifyData = rplData->notifyData;
            std::memset(notifyData.getRawPointer(), 0,
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
         }
      }
   }

   if (!error) {
      sDynLoadData->modules =
         virt_cast<virt_ptr<RPL_DATA> *>(
            OSAllocFromSystem(4 * linkInfo->numModules, -4));
      if (!sDynLoadData->modules) {
         decaf_abort("error");
      }

      auto entryModules =
         virt_cast<virt_ptr<RPL_DATA> *>(
            OSAllocFromSystem(4 * linkInfo->numModules, -4));
      if (!entryModules) {
         decaf_abort("error");
      }

      *outEntryModules = entryModules;

      auto itr = sDynLoadData->linkingRplList;
      auto prev = virt_ptr<RPL_DATA> { nullptr };
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

   OSFreeToSystem(linkInfo);
   return error;
}

static OSDynLoad_Error
doImports(virt_ptr<RPL_DATA> rplData);

static int32_t
sPrepareLoad(virt_ptr<RPL_DATA> *ptrRplData,
             OSDynLoad_AllocFn dynLoadAlloc,
             virt_ptr<loader::LOADER_MinFileInfo> minFileInfo)
{
   auto rplData = *ptrRplData;
   rplData->sectionInfo = virt_cast<loader::LOADER_SectionInfo *>(
      OSAllocFromSystem(
         sizeof(loader::LOADER_SectionInfo) * rplData->sectionInfoCount, -4));
   if (!rplData->sectionInfo) {
      // error
      decaf_abort("error");
   }

   minFileInfo->outSectionInfo = rplData->sectionInfo;

   if (auto error =
         cafe::invoke(
            cpu::this_core::state(),
            dynLoadAlloc,
            rplData->userFileInfoSize,
            4,
            virt_cast<virt_ptr<void> *>(virt_addrof(rplData->userFileInfo)))) {
   }

   minFileInfo->outFileInfo = rplData->userFileInfo;
   sDynLoadData->numAllocations++;
   sDynLoadData->totalAllocatedBytes += rplData->userFileInfoSize;
   std::memset(rplData->userFileInfo.getRawPointer(), 0, rplData->userFileInfoSize);

   if (minFileInfo->pathStringSize) {
      minFileInfo->pathStringBuffer =
         virt_cast<char *>(OSAllocFromSystem(minFileInfo->pathStringSize, 4));
      if (!minFileInfo->pathStringBuffer) {
         // error
         decaf_abort("error");
      }

      std::memset(minFileInfo->pathStringBuffer.getRawPointer(), 0,
                  minFileInfo->pathStringSize);
   }

   if (minFileInfo->fileInfoFlags & loader::rpl::RPL_FLAG_4) {
      if (auto error = kernel::loaderQuery(rplData->loaderHandle, minFileInfo)) {
         // error
         decaf_abort("error");
      }
   } else {
      if (minFileInfo->dataSize) {
         if (auto error =
               cafe::invoke(
                  cpu::this_core::state(),
                  dynLoadAlloc,
                  minFileInfo->dataSize,
                  minFileInfo->dataAlign,
                  virt_addrof(rplData->dataSection))) {
            // error
            decaf_abort("error");
         }

         // validate address space range
         // check_align

         rplData->dataSectionSize = minFileInfo->dataSize;
         minFileInfo->dataBuffer = rplData->dataSection;
         sDynLoadData->numAllocations++;
         sDynLoadData->totalAllocatedBytes += minFileInfo->dataSize;

      }

      if (minFileInfo->loadSize) {
         if (auto error =
               cafe::invoke(
                  cpu::this_core::state(),
                  dynLoadAlloc,
                  minFileInfo->loadSize,
                  minFileInfo->loadAlign,
                  virt_addrof(rplData->loadSection))) {
            // error
            decaf_abort("error");
         }

         // validate address space range
         // check_align

         rplData->loadSectionSize = minFileInfo->loadSize;
         minFileInfo->loadBuffer = rplData->loadSection;
         sDynLoadData->numAllocations++;
         sDynLoadData->totalAllocatedBytes += minFileInfo->loadSize;
      }

      if (auto error = kernel::loaderSetup(rplData->loaderHandle, minFileInfo)) {
         // error
         decaf_abort("error");
      }
   }

   OSFreeToSystem(minFileInfo);

   findExports(rplData);
   findTlsSection(rplData);

   rplData->next = sDynLoadData->linkingRplList;
   sDynLoadData->linkingRplList = rplData;
   *ptrRplData = nullptr;
   return doImports(rplData);
}

static int32_t
sReleaseImports()
{
   for (auto module = sDynLoadData->linkingRplList; module; module = module->next) {
      for (auto i = 0u; i < module->importModuleCount; ++i) {
         module->importModules[i]->handle;
      }
   }

   return 0;
}

static OSDynLoad_Error
internalAcquire(virt_ptr<char> name,
                virt_ptr<virt_ptr<RPL_DATA>> outRplData,
                virt_ptr<uint32_t> outNumEntryModules,
                virt_ptr<virt_ptr<virt_ptr<RPL_DATA>>> outEntryModules,
                bool doLoad)
{
   auto [moduleName, moduleNameLength] = resolveModuleName(name);
   auto rplName =
      virt_cast<char *>(
         OSAllocFromSystem(align_up(moduleNameLength + 1, 4), 4));
   if (!rplName) {
      return OSDynLoad_Error::OutOfSysMemory;
   }

   for (auto i = 0u; i < moduleNameLength; ++i) {
      rplName[i] = static_cast<char>(tolower(moduleName[i]));
   }
   rplName[moduleNameLength] = char { 0 };

   OSLockMutex(virt_addrof(sDynLoadData->mutex));

   if (sDynLoadData->acquiringRpl) {
      // error
      decaf_abort("error");
   }

   for (auto rpl = sDynLoadData->rplDataList; rpl; rpl = rpl->next) {
      if (strcmp(rpl->moduleName.getRawPointer(), rplName.getRawPointer()) == 0) {
         OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), rpl->handle);
         *outRplData = rpl;
         return OSDynLoad_Error::OK;
      }
   }

   for (auto rpl = sDynLoadData->linkingRplList; rpl; rpl = rpl->next) {
      if (strcmp(rpl->moduleName.getRawPointer(), rplName.getRawPointer()) == 0) {
         OSHandle_AddRef(virt_addrof(sDynLoadData->handleTable), rpl->handle);
         *outRplData = rpl;
         return OSDynLoad_Error::OK;
      }
   }

   if (strcmp(rplName.getRawPointer(), "coreinit") == 0) {
      // error
      decaf_abort("error");
   }

   StackObject<OSDynLoad_AllocFn> dynLoadAlloc;
   StackObject<OSDynLoad_FreeFn> dynLoadFree;
   OSDynLoad_GetAllocator(dynLoadAlloc, dynLoadFree);

   auto firstLinkingRpl = !sDynLoadData->linkingRplList;
   if (firstLinkingRpl) {
      // TODO:
      decaf_abort("TODO");
   }

   auto rplData = virt_cast<RPL_DATA *>(OSAllocFromSystem(sizeof(RPL_DATA), 4));
   if (rplData) {
      std::memset(rplData.getRawPointer(), 0, sizeof(RPL_DATA));

      auto minFileInfo =
         virt_cast<loader::LOADER_MinFileInfo *>(
            OSAllocFromSystem(sizeof(loader::LOADER_MinFileInfo), 4));
      if (!minFileInfo) {
         // error
         decaf_abort("error");
      }

      // aka RPL_TEMP_DATA
      std::memset(minFileInfo.getRawPointer(), 0, sizeof(loader::LOADER_MinFileInfo));
      StackObject<loader::LOADER_Handle> kernelHandle;
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
         decaf_abort("error");
      }

      static constexpr auto MaxTlsModuleIndex = uint32_t { 0x7fff };
      if (sDynLoadData->tlsModuleIndex > sDynLoadData->tlsHeader) {
         if (sDynLoadData->tlsModuleIndex > MaxTlsModuleIndex) {
            // error
            decaf_abort("error");
         }

         sDynLoadData->tlsHeader =
            std::min<uint32_t>(sDynLoadData->tlsModuleIndex + 8,
                               MaxTlsModuleIndex);
      }

      if (auto error = OSHandle_Alloc(virt_addrof(sDynLoadData->handleTable),
                                      rplData, nullptr,
                                      virt_addrof(rplData->handle))) {
         // error
         decaf_abort("error");
      }

      *outRplData = rplData;
      rplData->moduleName = rplName;
      rplData->moduleNameLen = moduleNameLength;
      rplData->loaderHandle = *kernelHandle;
      rplData->dynLoadFreeFn = *dynLoadFree;
      auto error = sPrepareLoad(&rplData, *dynLoadAlloc, minFileInfo);

      if (firstLinkingRpl) {
         if (error) {
            decaf_abort("error");
         } else {
            error = executeDynamicLink(nullptr,
                                       outNumEntryModules,
                                       outEntryModules,
                                       *outRplData);
         }

         sReleaseImports();

         while (sDynLoadData->linkingRplList) {
            auto rpl = sDynLoadData->linkingRplList;
            sDynLoadData->linkingRplList = rpl->next;
            rpl->next = nullptr;

            if (rpl->handle) {
               StackObject<uint32_t> handleRefCount;
               OSHandle_Release(virt_addrof(sDynLoadData->handleTable),
                                rpl->handle,
                                handleRefCount);
            }
         }
      }

      if (rplData) {
         // TODO
         decaf_abort("TODO");
      }
   }

   OSUnlockMutex(virt_addrof(sDynLoadData->mutex));
   return OSDynLoad_Error::OK;
}

static OSDynLoad_Error
doImports(virt_ptr<RPL_DATA> rplData)
{
   auto shstrndx = rplData->userFileInfo->shstrndx;
   if (!rplData->userFileInfo->shstrndx) {
      decaf_abort("error");
   }

   auto shStrSection = virt_cast<char *>(rplData->sectionInfo[shstrndx].address);
   if (!shStrSection) {
      decaf_abort("error");
   }

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

   rplData->importModuleCount = numImportModules;
   rplData->importModules = virt_cast<virt_ptr<RPL_DATA> *>(
      OSAllocFromSystem(sizeof(virt_ptr<RPL_DATA>) * numImportModules, 4));
   if (!rplData->importModules) {
      decaf_abort("error");
      return OSDynLoad_Error::OutOfSysMemory;
   }

   auto importModuleIdx = 0u;
   for (auto i = 0u; i < rplData->sectionInfoCount; ++i) {
      if (rplData->sectionInfo[i].address &&
          rplData->sectionInfo[i].name &&
          (rplData->sectionInfo[i].flags & loader::rpl::SHF_ALLOC) &&
          rplData->sectionInfo[i].type == loader::rpl::SHT_RPL_IMPORTS) {
         auto name = shStrSection + rplData->sectionInfo[i].name;
         auto error =
            internalAcquire(name + 9,
                            virt_addrof(rplData->importModules[importModuleIdx]),
                            0, 0, 0);
         if (error) {
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

            while (itr->next) {
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

   OSFreeToSystem(rplData->sectionInfo);
   rplData->sectionInfo = nullptr;
   return OSDynLoad_Error::OK;
}

using OSDynLoad_RplEntryFn = virt_func_ptr<
   int32_t (OSDynLoad_ModuleHandle moduleHandle,
            OSDynLoad_EntryReason reason)>;

int32_t
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
         break;
      }

      entryModules[i]->entryPoint = virt_addr { 0 };
   }

   if (entryModules) {
      OSFreeToSystem(entryModules);
   }

   return error;
}

/**
 * __OSDynLoad_InitFromCoreInit
 */
virt_addr
initialiseDynLoad()
{
   initialiseCommon();

   // Load the coreinit rpl data
   auto coreinitRplData = virt_cast<RPL_DATA *>(OSAllocFromSystem(sizeof(RPL_DATA), 4));
   decaf_check(coreinitRplData);
   std::memset(coreinitRplData.getRawPointer(), 0, sizeof(RPL_DATA));

   if (auto handleError = OSHandle_Alloc(virt_addrof(sDynLoadData->handleTable),
                                         coreinitRplData,
                                         nullptr,
                                         virt_addrof(coreinitRplData->handle))) {
      decaf_abort(fmt::format("Unexpected OSHandle_Alloc error = {}",
                              handleError));
   }

   sDynLoadData->coreinitModuleName = "coreinit";
   coreinitRplData->moduleName = virt_addrof(sDynLoadData->coreinitModuleName);
   coreinitRplData->moduleNameLen = 8u;
   coreinitRplData->loaderHandle = getCoreinitLoaderHandle();
   coreinitRplData->next = sDynLoadData->rplDataList;
   sDynLoadData->rplDataList = coreinitRplData;

   sSetupPerm(coreinitRplData, false);
   OSFreeToSystem(coreinitRplData->sectionInfo);
   coreinitRplData->sectionInfo = nullptr;

   sDynLoadData->tlsAllocLocked = FALSE;
   initialiseDefaultHeap(coreinitRplData,
                         make_stack_string("CoreInitDefaultHeap"));

   // Setup the rpx data
   auto rpxData = virt_cast<RPL_DATA *>(OSAllocFromSystem(sizeof(RPL_DATA), 4));
   decaf_check(rpxData);
   std::memset(rpxData.getRawPointer(), 0, sizeof(RPL_DATA));
   sDynLoadData->rpxData = rpxData;

   rpxData->handle = 0xFFFFFFFFu;
   rpxData->moduleName = virt_cast<char *>(OSAllocFromSystem(sDynLoadData->rpxNameLength + 1, 4));
   rpxData->moduleNameLen = sDynLoadData->rpxNameLength;
   decaf_check(rpxData->moduleName);
   std::memcpy(rpxData->moduleName.getRawPointer(),
               sDynLoadData->rpxName.getRawPointer(),
               rpxData->moduleNameLen);
   rpxData->moduleName[rpxData->moduleNameLen] = char { 0 };
   sSetupPerm(rpxData, true);

   if (sDynLoadData->tlsModuleIndex) {
      sDynLoadData->tlsHeader = sDynLoadData->tlsModuleIndex + 8;
   }

   sDynLoadData->linkingRplList = rpxData;
   rpxData->entryPoint = virt_addr { 0u };
   if (auto error = doImports(rpxData)) {
      decaf_abort("error");
   }

   StackObject<uint32_t> numEntryModules;
   StackObject<virt_ptr<virt_ptr<RPL_DATA>>> entryModules;
   if (auto error = executeDynamicLink(rpxData,
                                       numEntryModules, entryModules,
                                       rpxData)) {
      decaf_abort("error");
   }

   if (coreinitRplData->userFileInfo->pathString) {
      // sInitNotifyHdr for coreinit
   }

   kernel::loaderUserGainControl();

   // Notify the debugger of the RPX entry points
   if (decaf::config::debugger::enabled) {
      StackObject<virt_addr> funcAddr;
      auto error = OSDynLoad_FindExport(rpxData->handle, FALSE,
                                        make_stack_string("__preinit_user"),
                                        funcAddr);
      if (error != OSDynLoad_Error::OK) {
         *funcAddr = virt_addr { 0 };
      }

      debugger::notifyEntry(static_cast<uint32_t>(*funcAddr),
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
      decaf_abort("error");
   }

   if (sDynLoadData->modules) {
      OSFreeToSystem(sDynLoadData->modules);
   }

   sDynLoadData->modules = nullptr;
   sDynLoadData->numModules = 0u;

   if (!rpxData->entryPoint) {
      decaf_abort("error");
   }

   return rpxData->entryPoint;
}

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
      std::string_view { rplData->moduleName.getRawPointer(), rplData->moduleNameLen },
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
   RegisterFunctionExport(OSDynLoad_GetModuleName);
   RegisterFunctionExport(OSDynLoad_IsModuleLoaded);
   RegisterFunctionExport(OSDynLoad_Release);

   RegisterFunctionExport(OSGetSymbolName);
   RegisterFunctionExportName("__tls_get_addr", tls_get_addr);

   RegisterDataInternal(sDynLoadData);
}

} // namespace cafe::coreinit
