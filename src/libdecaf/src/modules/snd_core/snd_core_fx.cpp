#include "snd_core.h"
#include "snd_core_fx.h"

namespace snd_core
{

int32_t
AXFXChorusExpGetMemSize(AXFXChorus *chorus)
{
   return 32;
}

int32_t
AXFXDelayExpGetMemSize(AXFXDelay *chorus)
{
   return 32;
}

int32_t
AXFXReverbHiExpGetMemSize(AXFXReverbHi *chorus)
{
   return 32;
}

int32_t
AXFXReverbStdExpGetMemSize(AXFXReverbStd *chorus)
{
   return 32;
}

void
Module::registerFXFunctions()
{
   RegisterKernelFunction(AXFXChorusExpGetMemSize);
   RegisterKernelFunction(AXFXDelayExpGetMemSize);
   RegisterKernelFunction(AXFXReverbHiExpGetMemSize);
   RegisterKernelFunction(AXFXReverbStdExpGetMemSize);
}

} // namespace snd_core
