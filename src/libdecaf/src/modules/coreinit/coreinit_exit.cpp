#include "coreinit.h"
#include "coreinit_exit.h"
#include "kernel/kernel.h"
#include <common/log.h>

namespace coreinit
{

void
OSFatal(const char *message)
{
   gLog->error("Program reported a fatal error: {}", message);
   kernel::exitProcess(-1);
}

void
coreinit_Exit()
{
   gLog->info("Program requested _Exit()");
   kernel::exitProcess(0);
}

void
ghs_exit(int code)
{
   gLog->info("Program requested exit({})", code);
   kernel::exitProcess(code);
}

void
Module::registerExitFunctions()
{
   RegisterKernelFunction(OSFatal);
   RegisterKernelFunctionName("_Exit", coreinit_Exit);
   RegisterKernelFunctionName("exit", ghs_exit);
}

} // namespace coreinit
