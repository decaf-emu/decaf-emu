#pragma once
#include "snduser2_axfx.h"
#include "snduser2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXMultiChReverb
{
   UNKNOWN(0x24);
   be2_val<AXFXReverbType> type;

   //! Samples per second, set to either 48000 or 32000 in AXFXMultiChReverbInit.
   be2_val<uint32_t> samplesPerSecond;
};
CHECK_OFFSET(AXFXMultiChReverb, 0x24, type);
CHECK_OFFSET(AXFXMultiChReverb, 0x28, samplesPerSecond);
UNKNOWN_SIZE(AXFXMultiChReverb);

int32_t
AXFXMultiChReverbGetMemSize(virt_ptr<AXFXMultiChReverb> reverb);

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXMultiChReverb> reverb,
                      AXFXReverbType type,
                      AXFXSampleRate sampleRate);

void
AXFXMultiChReverbShutdown(virt_ptr<AXFXMultiChReverb> reverb);

BOOL
AXFXMultiChReverbParametersPreset(virt_ptr<AXFXMultiChReverb> reverb,
                                  AXFXReverbPreset preset);

BOOL
AXFXMultiChReverbSettingsUpdate(virt_ptr<AXFXMultiChReverb> reverb);

BOOL
AXFXMultiChReverbSettingsUpdateNoReset(virt_ptr<AXFXMultiChReverb> reverb);

void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXMultiChReverb> data,
                          virt_ptr<AXAuxCallbackData> auxData);

} // namespace cafe::snduser2
