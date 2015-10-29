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
   initialiseMemory();
   initialiseMembase();
   initialiseMessageQueues();
   initialiseSchedulerFunctions();
   initialiseShared();
   initialiseSystemInformation();
   initialiseThread();
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
   registerExpHeapFunctions();
   registerFastMutexFunctions();
   registerFileSystemFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
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
