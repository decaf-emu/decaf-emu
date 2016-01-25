#include "coreinit.h"
#include "coreinit_core.h"
#include "processor.h"

namespace coreinit
{

uint32_t
OSGetCoreCount()
{
   return gProcessor.getCoreCount();
}

uint32_t
OSGetCoreId()
{
   return gProcessor.getCoreID();
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
Module::registerCoreFunctions()
{
   RegisterKernelFunction(OSGetCoreCount);
   RegisterKernelFunction(OSGetCoreId);
   RegisterKernelFunction(OSGetMainCoreId);
   RegisterKernelFunction(OSIsMainCore);
}

} // namespace coreinit
