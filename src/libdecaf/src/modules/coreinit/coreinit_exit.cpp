#include "coreinit.h"
#include "coreinit_exit.h"
#include "common/log.h"

namespace coreinit
{

// TODO: Do a clean game exit rather than terminating execution.

void
OSFatal(const char *message)
{
   gLog->error("Program reported a fatal error: {}", message);
   _exit(1);
}

void
coreinit_Exit()
{
   gLog->info("Program requested _Exit()");
   _exit(0);
}

void
ghs_exit(int code)
{
   gLog->info("Program requested exit({})", code);
   _exit(code);
}

void
Module::registerExitFunctions()
{
   RegisterKernelFunction(OSFatal);
   RegisterKernelFunctionName("_Exit", coreinit_Exit);
   RegisterKernelFunctionName("exit", ghs_exit);
}

} // namespace coreinit
