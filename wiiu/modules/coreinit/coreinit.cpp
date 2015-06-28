#include "coreinit.h"
#include "coreinit_memory.h"

CoreInit::CoreInit()
{
   CoreInitDefaultHeap();
}

CoreInit::~CoreInit()
{
   CoreFreeDefaultHeap();
}

void
CoreInit::initialise()
{
   initialiseMembase();
   initialiseMessageQueues();
   initialiseGHS();
   initialiseSystemInformation();
   initialiseDynLoad();
}

void
CoreInit::RegisterFunctions()
{
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
   registerMessageQueueFunctions();
   registerMutexFunctions();
   registerSemaphoreFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();
}
