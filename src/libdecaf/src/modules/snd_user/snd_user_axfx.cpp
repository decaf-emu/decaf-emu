#include "snd_user.h"
#include "snd_user_axfx.h"

namespace snd_user
{

BOOL
AXFXReverbHiInit()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiShutdown()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiSettings()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXMultiChReverbInit()
{
   decaf_warn_stub();
   return TRUE;
}

void
Module::registerAXFXFunctions()
{
   RegisterKernelFunction(AXFXReverbHiInit);
   RegisterKernelFunction(AXFXReverbHiShutdown);
   RegisterKernelFunction(AXFXReverbHiSettings);
   RegisterKernelFunction(AXFXMultiChReverbInit);
}

} // namespace snd_user
