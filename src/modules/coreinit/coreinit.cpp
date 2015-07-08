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
   initialiseDynLoad();
   initialiseEvent();
   initialiseGHS();
   initialiseMembase();
   initialiseMessageQueues();
   initialiseSystemInformation();
}

void
CoreInit::RegisterFunctions()
{
   registerAlarmFunctions();
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
   registerSemaphoreFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();
}
