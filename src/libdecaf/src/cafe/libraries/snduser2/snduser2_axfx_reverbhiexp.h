#pragma once
#include "snduser2_axfx.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbHiExp
{
   UNKNOWN(0x114);

   //! Reverb Pre-Delay in seconds
   be2_val<float> preDelaySeconds;

   UNKNOWN(0x30);
};
CHECK_OFFSET(AXFXReverbHiExp, 0x114, preDelaySeconds);
CHECK_SIZE(AXFXReverbHiExp, 0x148);

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHiExp> reverb);

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHiExp> reverb);

void
AXFXReverbHiExpShutdown(virt_ptr<AXFXReverbHiExp> reverb);

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbHiExp> reverb);

BOOL
AXFXReverbHiExpSettings(virt_ptr<AXFXReverbHiExp> reverb);

BOOL
AXFXReverbHiExpSettingsUpdate(virt_ptr<AXFXReverbHiExp> reverb);

} // namespace cafe::snduser2
