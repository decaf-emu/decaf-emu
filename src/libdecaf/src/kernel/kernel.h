#pragma once
#include "libcpu/cpu.h"
#include "common/platform_fiber.h"

class TeenyHeap;

namespace coreinit
{
struct OSContext;
}

namespace kernel
{

namespace loader {

struct LoadedModule;

}

struct Fiber;

void
initialise();

void
setGameName(const std::string& name);

void
exitThreadNoLock();

TeenyHeap *
getSystemHeap();

void
setContext(coreinit::OSContext *next);

loader::LoadedModule *
getUserModule();

void
exitProcess(int code);

bool
hasExited();

int
getExitCode();

} // namespace kernel
