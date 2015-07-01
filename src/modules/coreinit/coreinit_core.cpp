#include "coreinit.h"
#include "coreinit_core.h"
#include "thread.h"

uint32_t
OSGetCoreCount()
{
   return 3;
}

uint32_t
OSGetCoreId()
{
   return Thread::getCurrentThread()->getCoreID();
}

uint32_t
OSGetMainCoreId()
{
   return 1;
}

BOOL
OSIsMainCore()
{
   return OSGetCoreId() == OSGetMainCoreId();
}

void
CoreInit::registerCoreFunctions()
{
   RegisterKernelFunction(OSGetCoreCount);
   RegisterKernelFunction(OSGetCoreId);
   RegisterKernelFunction(OSGetMainCoreId);
   RegisterKernelFunction(OSIsMainCore);
}
