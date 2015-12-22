#include "mic.h"
#include "mic_core.h"

MICHandle
MICInit(MICHandle type, void *, void *, be_val<uint32_t> *result)
{
   *result = 0;
   return type;
}

void
Mic::registerCoreFunctions()
{
   RegisterKernelFunction(MICInit);
}
