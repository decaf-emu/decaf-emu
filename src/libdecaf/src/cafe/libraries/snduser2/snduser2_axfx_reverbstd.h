#pragma once
#include "snduser2_axfx.h"
#include "snduser2_axfx_reverbstdexp.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbStd
{
   be2_struct<AXFXReverbStdExp> reverbExp;
   UNKNOWN(0x10);

   //! Reverb Pre-Delay in seconds
   be2_val<float> userPreDelaySeconds;
};
CHECK_SIZE(AXFXReverbStd, 0xFC);

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> chorus);

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbStd> chorus);

BOOL
AXFXReverbStdShutdown(virt_ptr<AXFXReverbStd> chorus);

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> chorus);

BOOL
AXFXReverbStdSettings(virt_ptr<AXFXReverbStd> chorus);

} // namespace cafe::snduser2
