#include "cafe_loader_entry.h"
#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_link.h"
#include "cafe_loader_prep.h"
#include "cafe_loader_setup.h"
#include "cafe_loader_shared.h"
#include "cafe_loader_log.h"
#include "cafe_loader_query.h"

#include <mutex>

namespace cafe::loader
{

static virt_ptr<kernel::Context> gpLoaderEntry_ProcContext = nullptr;
static int32_t gpLoaderEntry_ProcConfig = -1;
static LOADER_Code gpLoaderEntry_DispatchCode = LOADER_Code::Invalid;
static bool gpLoaderEntry_LoaderIntsAllowed = false;
static kernel::ProcessFlags gProcFlags = kernel::ProcessFlags::get(0);
static uint32_t gProcTitleLoc = 0;
static bool sLoaderInUserMode = true;
static std::mutex sLoaderMutex;

static int32_t
LOADER_Entry(virt_ptr<LOADER_EntryParams> entryParams)
{
   auto error = 0;

   switch (entryParams->dispatch.code) {
   case LOADER_Code::Prep:
      error = internal::LOADER_Prep(entryParams->procId,
                                    entryParams->dispatch.minFileInfo);
      break;
   case LOADER_Code::Setup:
      error = internal::LOADER_Setup(entryParams->procId,
                                     entryParams->dispatch.handle,
                                     FALSE,
                                     entryParams->dispatch.minFileInfo);
      break;
   case LOADER_Code::Purge:
      error = internal::LOADER_Setup(entryParams->procId,
                                     entryParams->dispatch.handle,
                                     TRUE,
                                     entryParams->dispatch.minFileInfo);
      break;
   case LOADER_Code::Link:
      error = internal::LOADER_Link(entryParams->procId,
                                    entryParams->dispatch.linkInfo,
                                    entryParams->dispatch.linkInfoSize,
                                    entryParams->dispatch.minFileInfo);
      break;
   case LOADER_Code::Query:
      error = internal::LOADER_Query(entryParams->procId,
                                     entryParams->dispatch.handle,
                                     entryParams->dispatch.minFileInfo);
      break;
   case LOADER_Code::UserGainControl:
      getGlobalStorage()->userHasControl = TRUE;
      error = 0;
      break;
   default:
      decaf_abort(fmt::format("Unimplemented LOADER_ENTRY code {}",
                              static_cast<int>(entryParams->dispatch.code.value())));
   }

   if (error) {
      if (auto fatalError = internal::LiGetFatalError()) {
         auto minFileInfo = entryParams->dispatch.minFileInfo;
         minFileInfo->error = error;
         minFileInfo->fatalErr = internal::LiGetFatalError();
         minFileInfo->fatalMsgType = internal::LiGetFatalMsgType();
         minFileInfo->fatalLine = internal::LiGetFatalMsgType();
         minFileInfo->fatalFunction = internal::LiGetFatalFunction();
         minFileInfo->fatalFunction[63] = char { 0 };
      }
   }

   return error;
}

int32_t
LoaderStart(BOOL isDispatch,
            virt_ptr<LOADER_EntryParams> entryParams)
{
   std::unique_lock<std::mutex> lock { sLoaderMutex };
   sLoaderInUserMode = true;

   if (isDispatch) {
      return LOADER_Entry(entryParams);
   }

   // Initialise static data
   internal::initialiseStaticDataHeap();
   // Initialise globals
   auto kernelIpcStorage = getKernelIpcStorage();
   gpLoaderEntry_ProcContext = entryParams->procContext;
   gpLoaderEntry_ProcConfig = entryParams->procConfig;
   gpLoaderEntry_DispatchCode = entryParams->dispatch.code;
   gpLoaderEntry_LoaderIntsAllowed = !!entryParams->interruptsAllowed;
   gProcFlags = kernelIpcStorage->processFlags;
   gProcTitleLoc = kernelIpcStorage->procTitleLoc;

   // Initialise loader
   internal::initialiseSharedHeaps();

   // Clear errors
   kernelIpcStorage->fatalErr = 0;
   kernelIpcStorage->fatalMsgType = 0u;
   kernelIpcStorage->fatalLine = 0u;
   kernelIpcStorage->fatalFunction[0] = char { 0 };
   kernelIpcStorage->loaderInitError = 0;
   internal::LiResetFatalError();

   auto error =
      internal::LOADER_Init(kernelIpcStorage->targetProcessId,
                            kernelIpcStorage->numCodeAreaHeapBlocks,
                            kernelIpcStorage->maxCodeSize,
                            kernelIpcStorage->maxDataSize,
                            virt_addrof(kernelIpcStorage->rpxModule),
                            virt_addrof(kernelIpcStorage->loadedModuleList),
                            virt_addrof(kernelIpcStorage->startInfo));
   if (error) {
      if (!internal::LiGetFatalError()) {
         internal::LiSetFatalError(0x1872A7u, 0, 1, "__LoaderStart", 227);
      }

      if (!internal::LiGetFatalError()) {
         internal::LiPanic("entry.c", 239, "***RPX failed loader but didn't generate fatal error information; err={}.", error);
      }

      kernelIpcStorage->loaderInitError = error;
      kernelIpcStorage->fatalLine = internal::LiGetFatalLine();
      kernelIpcStorage->fatalErr = internal::LiGetFatalError();
      kernelIpcStorage->fatalMsgType = internal::LiGetFatalMsgType();
      std::strncpy(virt_addrof(kernelIpcStorage->fatalFunction).getRawPointer(),
                   internal::LiGetFatalFunction().data(),
                   kernelIpcStorage->fatalFunction.size() - 1);
      kernelIpcStorage->fatalFunction[kernelIpcStorage->fatalFunction.size() - 1] = char { 0 };
   }

   if (kernelIpcStorage->targetProcessId == kernel::UniqueProcessId::Root) {
      // TODO: Syscall Loader_FinishInitAndPreload
   } else {
      // TODO: Syscall Loader_ProfileEntry
      // TODO: Syscall Loader_ContinueStartProcess
   }

   return 0;
}

void
lockLoader()
{
   sLoaderMutex.lock();
}

void
unlockLoader()
{
   sLoaderMutex.unlock();
}


virt_ptr<LOADED_RPL>
getLoadedRpx()
{
   return getGlobalStorage()->loadedRpx;
}

virt_ptr<LOADED_RPL>
getLoadedRplLinkedList()
{
   return getGlobalStorage()->firstLoadedRpl;
}

namespace internal
{

uint32_t
getProcTitleLoc()
{
   return gProcTitleLoc;
}

kernel::ProcessFlags
getProcFlags()
{
   return gProcFlags;
}

} // namespace internal

} // namespace cafe::loader
