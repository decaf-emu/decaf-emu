#include "snduser2.h"
#include "snduser2_axfx_reverbhi.h"
#include "snduser2_axfx_reverbhiexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

/**
 * Converts the params from the AXFXReverbHi struct to the AXFXReverbHiExp params.
 */
static void
translateReverbHiUserParamsToExp(virt_ptr<AXFXReverbHi> reverb)
{
   // TODO: Implement translateReverbUserParamsToExp
   decaf_warn_stub();
}

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb)
{
   reverb->reverbExp.preDelaySeconds = reverb->userPreDelaySeconds;
   return AXFXReverbHiExpGetMemSize(virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbHiInit(virt_ptr<AXFXReverbHi> reverb)
{
   translateReverbHiUserParamsToExp(reverb);
   return AXFXReverbHiExpInit(virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbHiShutdown(virt_ptr<AXFXReverbHi> reverb)
{
   AXFXReverbHiExpShutdown(virt_addrof(reverb->reverbExp));
   return TRUE;
}

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbHi> reverb)
{
   AXFXReverbHiExpCallback(buffers, virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbHiSettings(virt_ptr<AXFXReverbHi> reverb)
{
   translateReverbHiUserParamsToExp(reverb);
   return AXFXReverbHiExpSettings(virt_addrof(reverb->reverbExp));
}

void
Library::registerAxfxReverbHiSymbols()
{
   RegisterFunctionExport(AXFXReverbHiGetMemSize);
   RegisterFunctionExport(AXFXReverbHiInit);
   RegisterFunctionExport(AXFXReverbHiShutdown);
   RegisterFunctionExport(AXFXReverbHiCallback);
   RegisterFunctionExport(AXFXReverbHiSettings);
}

} // namespace cafe::snduser2
