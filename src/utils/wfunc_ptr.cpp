#include "cpu/cpu.h"
#include "utils/wfunc_ptr.h"

ThreadState *
GetCurrentCoreState()
{
   return &cpu::get_current_core()->state;
}
