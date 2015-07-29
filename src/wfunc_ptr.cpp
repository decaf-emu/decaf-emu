#include "wfunc_ptr.h"
#include "processor.h"

ThreadState *GetCurrentFiberState() {
   return &gProcessor.getCurrentFiber()->state;
}
