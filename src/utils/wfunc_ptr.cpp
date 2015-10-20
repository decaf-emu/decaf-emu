#include "processor.h"
#include "utils/wfunc_ptr.h"

ThreadState *
GetCurrentFiberState()
{
   return &gProcessor.getCurrentFiber()->state;
}
