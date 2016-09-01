#include "mic.h"
#include "mic_core.h"

namespace mic
{

MICHandle
MICInit(uint32_t type, void *, void *, be_val<int32_t> *result)
{
   decaf_warn_stub();

   *result = -8;
   return -1;
}

int
MICOpen(MICHandle handle)
{
   decaf_warn_stub();

   return -8;
}

int
MICGetStatus(MICHandle handle, void *statusOut)
{
   decaf_warn_stub();

   return -2;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(MICInit);
   RegisterKernelFunction(MICOpen);
   RegisterKernelFunction(MICGetStatus);
}

} // namespace mic
