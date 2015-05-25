#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/debug.h"

// TODO: Return TRUE so we get OSDebug messages!

BOOL OSIsDebuggerPresent(void)
{
   return FALSE;
}

BOOL OSIsDebuggerInitialized(void)
{
   return FALSE;
}

void CoreInit::registerDebugFunctions()
{
   RegisterSystemFunction(OSIsDebuggerPresent);
   RegisterSystemFunction(OSIsDebuggerInitialized);
}
