#include "coreinit.h"
#include "coreinit_memory.h"

CoreInit::CoreInit()
{
   registerDebugFunctions();
   registerDynLoadFunctions();
   registerExpHeapFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
   registerMemoryFunctions();
   registerMembaseFunctions();
   registerMutexFunctions();
   registerSpinLockFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();

   CoreInitDefaultHeap();
}

void CoreInit::initialise()
{
   initialiseMembase();
   initialiseGHS();
   initialiseSystemInformation();
   initialiseDynLoad();
}
