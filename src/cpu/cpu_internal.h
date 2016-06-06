#pragma once
#include "cpu.h"
#include <condition_variable>

namespace cpu
{

extern Core
gCore[3];

extern std::atomic_bool
gRunning;

extern entrypoint_handler
gCoreEntryPointHandler;

extern interrupt_handler
gInterruptHandler;

extern std::condition_variable
gTimerCondition;

extern std::thread
gTimerThread;

bool has_breakpoints();

bool pop_breakpoint(ppcaddr_t address);

void timerEntryPoint();

kernel_call_entry *
get_kernel_call(uint32_t id);

namespace this_core
{

void
check_interrupts();

void
update_rounding_mode();

}

}