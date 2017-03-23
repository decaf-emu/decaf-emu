#pragma once
#include "libcpu/cpu.h"
#include <common/platform_fiber.h>
#include "kernel_gameinfo.h"

class TeenyHeap;

namespace coreinit
{
struct OSContext;
}

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

void
exitThreadNoLock();

TeenyHeap *
getSystemHeap();

void
setContext(coreinit::OSContext *next);

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
