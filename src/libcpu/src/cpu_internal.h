#pragma once
#include "cpu.h"
#include "cpu_config.h"
#include "mem.h"

#include <array>
#include <condition_variable>
#include <memory>

namespace cpu
{

extern std::array<Core *, 3>
gCore;

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

extern uint32_t
gJitVerifyAddress;

extern std::condition_variable
gTimerCondition;

extern std::thread
gTimerThread;

void
timerEntryPoint();

void
onKernelCall(cpu::Core *core,
             uint32_t id);

bool
initialiseMemory();

namespace this_core
{

void
updateRoundingMode();

} // namespace this_core

} // namespace cpu
