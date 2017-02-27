#include <common/platform.h>
#include "jit_vmemruntime.h"

#ifndef PLATFORM_WINDOWS

namespace cpu
{

namespace jit
{

void registerUnwindTable(VMemRuntime *runtime, intptr_t jitCallAddr)
{
}

void unregisterUnwindTable()
{
}

} // namespace jit

} // namespace cpu

#endif // PLATFORM_WINDOWS
