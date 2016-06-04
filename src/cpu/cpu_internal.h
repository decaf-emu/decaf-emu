#pragma once
#include "cpu.h"

namespace cpu
{

extern entrypoint_handler gCoreEntryPointHandler;
extern interrupt_handler gInterruptHandler;

void timerEntryPoint();

void
update_rounding_mode(Core *state);

kernel_call_entry *
get_kernel_call(uint32_t id);

namespace this_core
{

void
check_interrupts();

}

}