#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_appio.h"
#include "coreinit_bsp.h"
#include "coreinit_device.h"
#include "coreinit_driver.h"
#include "coreinit_dynload.h"
#include "coreinit_exception.h"
#include "coreinit_ghs.h"
#include "coreinit_im.h"
#include "coreinit_interrupts.h"
#include "coreinit_ipcdriver.h"
#include "coreinit_lockedcache.h"
#include "coreinit_mcp.h"
#include "coreinit_memallocator.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "coreinit_systeminfo.h"
#include "coreinit_systemmessagequeue.h"
#include "coreinit_thread.h"
#include "coreinit_time.h"
#include <common/log.h>

#include "cafe/libraries/cafe_hle.h"
#include "cafe/cafe_ppc_interface_invoke.h"

namespace cafe::coreinit
{

static void
rpl_entry(/* no args for coreinit entry point */)
{
   auto coreId = cpu::this_core::id();

   // Always initialise time then system info first, other things depend on it.
   internal::initialiseTime();
   internal::initialiseSystemInfo();

   internal::initialiseIci();
   internal::initialiseSystemMessageQueue();
   internal::initialiseExceptionHandlers();
   internal::initialiseScheduler();
   internal::initialiseThreads();
   internal::initialiseAlarmThread();
   internal::initialiseLockedCache(coreId);
   internal::initialiseMemory();
   internal::initialiseMemHeap();
   internal::initialiseAllocatorStaticData();
   IPCDriverInit();
   IPCDriverOpen();
   internal::initialiseAppIoThreads();
   internal::initialiseDeviceTable();
   bspInitializeShimInterface();
   internal::initialiseMcp();

   auto entryPoint = internal::initialiseDynLoad();

   // registerMemDriver
   // registerCacheDriver
   // registerIpcDriver
   // registerInputDriver
   internal::initialiseIm(); // Actually called from registerInputDriver
   // registerTestDriver
   // registerAcpLoadDriver
   // registerButtonDriver
   // registerClipboardDriver
   // driver on init
   internal::driverOnInit();

   auto entryFunc =
      virt_func_cast<int32_t(uint32_t argc, virt_ptr<void> argv)>(entryPoint);
   auto result = cafe::invoke(cpu::this_core::state(),
                              entryFunc,
                              uint32_t { 0u }, virt_ptr<void> { nullptr });
   ghs_exit(result);
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerAlarmSymbols();
   registerAppIoSymbols();
   registerAtomicSymbols();
   registerAtomic64Symbols();
   registerBspSymbols();
   registerCacheSymbols();
   registerClipboardSymbols();
   registerContextSymbols();
   registerCoreSymbols();
   registerCoroutineSymbols();
   registerCosReportSymbols();
   registerDeviceSymbols();
   registerDriverSymbols();
   registerDynLoadSymbols();
   registerEventSymbols();
   registerExceptionSymbols();
   registerFastMutexSymbols();
   registerFiberSymbols();
   registerFsSymbols();
   registerFsClientSymbols();
   registerFsCmdSymbols();
   registerFsCmdBlockSymbols();
   registerFsDriverSymbols();
   registerFsStateMachineSymbols();
   registerFsaSymbols();
   registerFsaCmdSymbols();
   registerFsaShimSymbols();
   registerGhsSymbols();
   registerHandleSymbols();
   registerImSymbols();
   registerInterruptSymbols();
   registerIosSymbols();
   registerIpcBufPoolSymbols();
   registerIpcDriverSymbols();
   registerLockedCacheSymbols();
   registerMcpSymbols();
   registerMemAllocatorSymbols();
   registerMemBlockHeapSymbols();
   registerMemDefaultHeapSymbols();
   registerMemExpHeapSymbols();
   registerMemFrmHeapSymbols();
   registerMemHeapSymbols();
   registerMemListSymbols();
   registerMemorySymbols();
   registerMemUnitHeapSymbols();
   registerMessageQueueSymbols();
   registerMutexSymbols();
   registerOsReportSymbols();
   registerOverlayArenaSymbols();
   registerRendezvousSymbols();
   registerSchedulerSymbols();
   registerScreenSymbols();
   registerSemaphoreSymbols();
   registerSnprintfSymbols();
   registerSpinLockSymbols();
   registerSystemHeapSymbols();
   registerSystemInfoSymbols();
   registerSystemMessageQueueSymbols();
   registerTaskQueueSymbols();
   registerThreadSymbols();
   registerTimeSymbols();
   registerUserConfigSymbols();
}

} // namespace cafe::coreinit
