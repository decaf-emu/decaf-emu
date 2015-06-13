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
   registerCacheFunctions();
   registerDebugFunctions();
   registerDynLoadFunctions();
   registerEventFunctions();
   registerExpHeapFunctions();
   registerFileSystemFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
   registerMcpFunctions();
   registerMemoryFunctions();
   registerMembaseFunctions();
   registerMessageQueueFunctions();
   registerMutexFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();
}
