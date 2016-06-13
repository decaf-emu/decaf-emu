#include "coreinit.h"
#include "coreinit_core.h"
#include "libcpu/cpu.h"

namespace coreinit
{

/**
 * Returns the number of cores, should always be 3.
 */
uint32_t
OSGetCoreCount()
{
   return 3;
}


/**
 * Returns the ID of the core currently executing this thread.
 */
uint32_t
OSGetCoreId()
{
   return cpu::this_core::id();
}


/**
 * Returns the ID of the main core.
 */
uint32_t
OSGetMainCoreId()
{
   return 1;
}


/**
 * Returns true if the current core is the main core.
 */
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
