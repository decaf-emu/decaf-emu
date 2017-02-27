#pragma once
#include "cpu.h"
#include "mem.h"

#include <condition_variable>

namespace cpu
{

extern Core
gCore[3];

extern std::atomic_bool
gRunning;

extern EntrypointHandler
gCoreEntryPointHandler;

extern InterruptHandler
gInterruptHandler;

extern BranchTraceHandler
gBranchTraceHandler;

extern jit_mode
gJitMode;

extern std::condition_variable
gTimerCondition;

extern std::thread
gTimerThread;

bool
hasBreakpoints();

bool
popBreakpoint(ppcaddr_t address);

void
timerEntryPoint();

KernelCallEntry *
getKernelCall(uint32_t id);

namespace this_core
{

void
updateRoundingMode();

} // namespace this_core

} // namespace cpu
