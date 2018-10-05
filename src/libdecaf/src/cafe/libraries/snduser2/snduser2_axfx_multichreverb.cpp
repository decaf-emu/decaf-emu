#include "snduser2.h"
#include "snduser2_axfx_multichreverb.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

int32_t
AXFXMultiChReverbGetMemSize(virt_ptr<AXFXMultiChReverb> reverb)
{
   decaf_warn_stub();
   return 32;
}

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXMultiChReverb> reverb,
                      AXFXReverbType type,
                      AXFXSampleRate sampleRate)
{
   auto samplesPerSecond = 32000;
   if (sampleRate == AXFXSampleRate::Rate48khz) {
      samplesPerSecond = 48000;
   }

   decaf_warn_stub();
   return TRUE;
}

void
AXFXMultiChReverbShutdown(virt_ptr<AXFXMultiChReverb> reverb)
{
   decaf_warn_stub();
}

BOOL
AXFXMultiChReverbParametersPreset(virt_ptr<AXFXMultiChReverb> reverb,
                                  AXFXReverbPreset preset)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXMultiChReverbSettingsUpdate(virt_ptr<AXFXMultiChReverb> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXMultiChReverbSettingsUpdateNoReset(virt_ptr<AXFXMultiChReverb> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXMultiChReverb> data,
                          virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

void
Library::registerAxfxMultiChReverbSymbols()
{
   RegisterFunctionExport(AXFXMultiChReverbGetMemSize);
   RegisterFunctionExport(AXFXMultiChReverbInit);
   RegisterFunctionExport(AXFXMultiChReverbShutdown);
   RegisterFunctionExport(AXFXMultiChReverbParametersPreset);
   RegisterFunctionExport(AXFXMultiChReverbSettingsUpdate);
   RegisterFunctionExport(AXFXMultiChReverbSettingsUpdateNoReset);
   RegisterFunctionExport(AXFXMultiChReverbCallback);
}

} // namespace cafe::snduser2
