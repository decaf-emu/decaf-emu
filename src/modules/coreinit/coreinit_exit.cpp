#include "coreinit.h"
#include "coreinit_exit.h"
#include "utils/log.h"

void
_Exit()
{
   gLog->info("Program requested exit.");
   _exit(0);  // TODO: pass this up to the UI layer instead
}

void
OSFatal(const char *message)
{
   gLog->error("Program reported a fatal error: {}", message);
   _exit(1);  // TODO: pass this up to the UI layer instead
}

void
CoreInit::registerExitFunctions()
{
   RegisterKernelFunction(_Exit);
   RegisterKernelFunction(OSFatal);
}
