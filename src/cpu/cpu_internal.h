#pragma once
#include "cpu.h"

namespace cpu
{

extern entrypoint_handler gCoreEntryPointHandler;
extern interrupt_handler gInterruptHandler;

void
update_rounding_mode(ThreadState *state);

kernel_call_entry *
get_kernel_call(uint32_t id);

}