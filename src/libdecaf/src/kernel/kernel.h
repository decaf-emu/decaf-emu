#pragma once
#include "libcpu/cpu.h"
#include <common/platform_fiber.h>
#include "kernel_gameinfo.h"

class TeenyHeap;

namespace kernel
{

/**
* \defgroup kernel Kernel
* @{
*/

namespace loader
{
struct LoadedModule;
}

loader::LoadedModule *
getUserModule();

loader::LoadedModule *
getTLSModule(uint32_t index);

/** @} */

} // namespace kernel
