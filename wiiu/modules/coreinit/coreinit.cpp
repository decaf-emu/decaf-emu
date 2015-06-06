#include "coreinit.h"

CoreInit::CoreInit()
{
   registerDebugFunctions();
   registerDynLoadFunctions();
   registerExpHeapFunctions();
   registerFrameHeapFunctions();
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
   initialiseSystemInformation();
   initialiseDynLoad();
}
