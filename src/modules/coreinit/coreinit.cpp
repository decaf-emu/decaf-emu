#include "coreinit.h"
#include "coreinit_memheap.h"

CoreInit::CoreInit()
{
}

CoreInit::~CoreInit()
{
   CoreFreeDefaultHeap();
}

void
CoreInit::initialise()
{
   initialiseAlarm();
   initialiseAtomic64();
   initialiseDynLoad();
   initialiseEvent();
   initialiseGHS();
   initialiseLockedCache();
   initialiseMembase();
   initialiseMessageQueues();
   initialiseSchedulerFunctions();
   initialiseShared();
   initialiseSystemInformation();
}

void
CoreInit::RegisterFunctions()
{
   registerAlarmFunctions();
   registerAtomic64Functions();
   registerCoreFunctions();
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
}
