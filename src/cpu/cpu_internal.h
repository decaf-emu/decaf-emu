#pragma once
#include "cpu.h"

namespace cpu
{

extern entrypoint_handler gCoreEntryPointHandler;
extern interrupt_handler gInterruptHandler;

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