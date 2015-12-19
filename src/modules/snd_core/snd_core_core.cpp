#include "snd_core.h"
#include "snd_core_core.h"
#include "utils/wfunc_ptr.h"

static BOOL
gAXInit = FALSE;

void
AXInit()
{
   // TODO: AXInit
   gAXInit = TRUE;
}

BOOL
AXIsInit()
{
   return gAXInit;
}

void
AXInitProfile(AXProfile *profile, uint32_t maxProfiles)
{
   // TODO: AXInitProfile
}

AXResult::Result
AXSetDefaultMixerSelect(uint32_t)
{
   // TODO: AXSetDefaultMixerSelect
   return AXResult::Success;
}

AXResult::Result
AXRegisterAppFrameCallback(AXFrameCallback callback)
{
   // TODO: AXRegisterAppFrameCallback
   return AXResult::Success;
}

int32_t
AXRmtGetSamplesLeft()
{
   // TODO: AXRmtGetSamplesLeft
   return 0;
}

int32_t
AXRmtGetSamples(int32_t, be_val<uint8_t> *buffer, int32_t samples)
{
   // TODO: AXRmtGetSamples
   return 0;
}

int32_t
AXRmtAdvancePtr(int32_t)
{
   // TODO: AXRmtAdvancePtr
   return 0;
}

void
Snd_Core::registerCoreFunctions()
{
   RegisterKernelFunction(AXInit);
   RegisterKernelFunction(AXIsInit);
   RegisterKernelFunction(AXInitProfile);
   RegisterKernelFunction(AXSetDefaultMixerSelect);
   RegisterKernelFunction(AXRegisterAppFrameCallback);
   RegisterKernelFunction(AXRmtGetSamples);
   RegisterKernelFunction(AXRmtGetSamplesLeft);
   RegisterKernelFunction(AXRmtAdvancePtr);
}
