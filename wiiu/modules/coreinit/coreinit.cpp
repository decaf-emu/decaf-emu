#include "coreinit.h"
#include "coreinit_memory.h"

CoreInit::CoreInit()
{
   registerCacheFunctions();
   registerDebugFunctions();
   registerDynLoadFunctions();
   registerExpHeapFunctions();
   registerFileSystemFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
   registerMcpFunctions();
   registerMemoryFunctions();
   registerMembaseFunctions();
   registerMessageQueueFunctions();
   registerMutexFunctions();
   registerSaveFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();

   CoreInitDefaultHeap();
}

void CoreInit::initialise()
{
   initialiseMembase();
   initialiseMessageQueues();
   initialiseGHS();
   initialiseSystemInformation();
   initialiseDynLoad();
}
