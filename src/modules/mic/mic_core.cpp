#include "mic.h"
#include "mic_core.h"

MICHandle
MICInit(uint32_t type, void *, void *, be_val<int32_t> *result)
{
   *result = -8;
   return -1;
}

int
MICOpen(MICHandle handle)
{
   return -8;
}

int
MICGetStatus(MICHandle handle, void *statusOut)
{
   return -2;
}

void
Mic::registerCoreFunctions()
{
   RegisterKernelFunction(MICInit);
   RegisterKernelFunction(MICOpen);
   RegisterKernelFunction(MICGetStatus);
}
