#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/debug.h"
#include "modules/coreinit/thread.h"

CoreInit::CoreInit()
{
   registerDebugFunctions();
   registerThreadFunctions();
}