#include "snduser2.h"
#include "snduser2_axfx_reverbstd.h"
#include "snduser2_axfx_reverbstdexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

/**
 * Converts the params from the AXFXReverbStd struct to the AXFXReverbStdExp params.
 */
static void
translateReverbStdUserParamsToExp(virt_ptr<AXFXReverbStd> reverb)
{
   // TODO: Implement translateReverbUserParamsToExp
   decaf_warn_stub();
}

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> reverb)
{
   reverb->reverbExp.preDelaySeconds = reverb->userPreDelaySeconds;
   return AXFXReverbStdExpGetMemSize(virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbStd> reverb)
{
   translateReverbStdUserParamsToExp(reverb);
   return AXFXReverbStdExpInit(virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbStdShutdown(virt_ptr<AXFXReverbStd> reverb)
{
   AXFXReverbStdExpShutdown(virt_addrof(reverb->reverbExp));
   return TRUE;
}

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> reverb)
{
   AXFXReverbStdExpCallback(buffers, virt_addrof(reverb->reverbExp));
}

BOOL
AXFXReverbStdSettings(virt_ptr<AXFXReverbStd> reverb)
{
   translateReverbStdUserParamsToExp(reverb);
   return AXFXReverbStdExpSettings(virt_addrof(reverb->reverbExp));
}

void
Library::registerAxfxReverbStdSymbols()
{
   RegisterFunctionExport(AXFXReverbStdGetMemSize);
   RegisterFunctionExport(AXFXReverbStdInit);
   RegisterFunctionExport(AXFXReverbStdShutdown);
   RegisterFunctionExport(AXFXReverbStdCallback);
   RegisterFunctionExport(AXFXReverbStdSettings);
}

} // namespace cafe::snduser2
