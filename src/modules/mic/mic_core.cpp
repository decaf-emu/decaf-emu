#include "mic.h"
#include "mic_core.h"

MICHandle
MICInit(MICHandle type, void *, void *, be_val<int32_t> *result)
{
   *result = -8;
   return -1;
}

void
Mic::registerCoreFunctions()
{
   RegisterKernelFunction(MICInit);
}
