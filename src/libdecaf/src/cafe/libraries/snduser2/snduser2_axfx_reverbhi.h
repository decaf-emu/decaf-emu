#pragma once
#include "snduser2_axfx.h"
#include "snduser2_axfx_reverbhiexp.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbHi
{
   be2_struct<AXFXReverbHiExp> reverbExp;
   UNKNOWN(0x10);

   //! Reverb Pre-Delay in seconds
   be2_val<float> userPreDelaySeconds;

   UNKNOWN(0x4);
};
CHECK_OFFSET(AXFXReverbHi, 0x158, userPreDelaySeconds);
CHECK_SIZE(AXFXReverbHi, 0x160);

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiShutdown(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiSettings(virt_ptr<AXFXReverbHi> reverb);

} // namespace cafe::snduser2
