#include "coreinit.h"
#include "coreinit_core.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

/**
 * Returns the ID of the core currently executing this thread.
 */
uint32_t
OSGetCoreId()
{
   return cpu::this_core::id();
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
Library::registerCoreSymbols()
{
   RegisterFunctionExport(OSGetCoreCount);
   RegisterFunctionExport(OSGetCoreId);
   RegisterFunctionExport(OSGetMainCoreId);
   RegisterFunctionExport(OSIsMainCore);
}

} // namespace cafe::coreinit
