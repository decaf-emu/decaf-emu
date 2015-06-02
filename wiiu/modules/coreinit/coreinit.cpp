#include "coreinit.h"

CoreInit::CoreInit()
{
   registerDebugFunctions();
   registerMemoryFunctions();
   registerMutexFunctions();
   registerSystemInfoFunctions();
   registerThreadFunctions();
}
