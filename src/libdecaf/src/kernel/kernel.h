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

struct Fiber;

void
initialise();

void
shutdown();

void
setExecutableFilename(const std::string& name);

TeenyHeap *
getSystemHeap();

loader::LoadedModule *
getUserModule();

loader::LoadedModule *
getTLSModule(uint32_t index);

void
exitProcess(int code);

bool
hasExited();

int
getExitCode();

const decaf::GameInfo &
getGameInfo();

/** @} */

} // namespace kernel
