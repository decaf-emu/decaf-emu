#pragma once
#include "libcpu/cpu.h"
#include "common/platform_fiber.h"

class TeenyHeap;

namespace coreinit
{
struct OSThread;
}

namespace kernel
{

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
switchThread(coreinit::OSThread *previous, coreinit::OSThread *next);

void
exitProcess(int code);

int
getExitCode();

} // namespace kernel
