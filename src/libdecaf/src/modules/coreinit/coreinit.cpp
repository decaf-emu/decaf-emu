#include "coreinit.h"
#include "coreinit_memheap.h"

namespace coreinit
{

Module::~Module()
{
   CoreFreeDefaultHeap();
}

void
Module::initialise()
{
   // Always initialise clock first
   initialiseClock();

   initialiseAlarm();
   initialiseAppIo();
   initialiseAllocatorFunctions();
   initialiseDynLoad();
   initialiseEvent();
   initialiseExceptions();
   initialiseGHS();
   initialiseGhsTypeInfo();
   initialiseLockedCache();
   initialiseMembase();
   initialiseMessageQueues();
   initialiseSchedulerFunctions();
   initialiseShared();
   initialiseSystemInformation();
   initialiseThreadFunctions();
   initialiseUserConfig();
}

void
Module::RegisterFunctions()
{
   registerAlarmFunctions();
   registerAppIoFunctions();
   registerAllocatorFunctions();
   registerAtomic64Functions();
   registerCoreFunctions();
   registerCoroutineFunctions();
   registerCacheFunctions();
   registerDebugFunctions();
   registerDeviceFunctions();
   registerDynLoadFunctions();
   registerEventFunctions();
   registerExceptionFunctions();
   registerExitFunctions();
   registerExpHeapFunctions();
   registerFastMutexFunctions();
   registerFileSystemFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
   registerGhsTypeInfoFunctions();
   registerInterruptFunctions();
   registerLockedCacheFunctions();
   registerMcpFunctions();
   registerMemoryFunctions();
   registerMembaseFunctions();
   registerMemlistFunctions();
   registerMessageQueueFunctions();
   registerMutexFunctions();
   registerRendezvousFunctions();
   registerSchedulerFunctions();
   registerSemaphoreFunctions();
   registerSharedFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerTaskQueueFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();
   registerUnitHeapFunctions();
}

} // namespace coreinit
