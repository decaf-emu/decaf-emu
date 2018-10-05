#include "snduser2.h"
#include "snduser2_axfx_delayexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelayExp> delay)
{
   auto samplersPerMS = AXGetInputSamplesPerSec() / 1000.0f;
   auto perChannelSamples = delay->userDelayMS * samplersPerMS;
   auto numSamples = 3 * perChannelSamples;
   return static_cast<int32_t>(sizeof(int32_t) * numSamples);
}

BOOL
AXFXDelayExpInit(virt_ptr<AXFXDelayExp> delay)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXDelayExpShutdown(virt_ptr<AXFXDelayExp> delay)
{
   decaf_warn_stub();
}

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelayExp> delay)
{
   decaf_warn_stub();
}

BOOL
AXFXDelayExpSettings(virt_ptr<AXFXDelayExp> delay)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXDelayExpSettingsUpdate(virt_ptr<AXFXDelayExp> delay)
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxDelayExpSymbols()
{
   RegisterFunctionExport(AXFXDelayExpGetMemSize);
   RegisterFunctionExport(AXFXDelayExpInit);
   RegisterFunctionExport(AXFXDelayExpShutdown);
   RegisterFunctionExport(AXFXDelayExpCallback);
   RegisterFunctionExport(AXFXDelayExpSettings);
   RegisterFunctionExport(AXFXDelayExpSettingsUpdate);
}

} // namespace cafe::snduser2
