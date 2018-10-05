#pragma once
#include "snduser2_axfx.h"
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbStdExp
{
   UNKNOWN(0xB0);
   be2_val<uint32_t> stateFlags;

   UNKNOWN(4);

   //! Reverb Pre-Delay in seconds
   be2_val<float> preDelaySeconds;

   UNKNOWN(0x2C);
};
CHECK_OFFSET(AXFXReverbStdExp, 0xB0, stateFlags);
CHECK_OFFSET(AXFXReverbStdExp, 0xB8, preDelaySeconds);
CHECK_SIZE(AXFXReverbStdExp, 0xE8);

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStdExp> reverb);

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbStdExp> reverb);

void
AXFXReverbStdExpShutdown(virt_ptr<AXFXReverbStdExp> reverb);

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStdExp> reverb);

BOOL
AXFXReverbStdExpSettings(virt_ptr<AXFXReverbStdExp> reverb);

BOOL
AXFXReverbStdExpSettingsUpdate(virt_ptr<AXFXReverbStdExp> reverb);

} // namespace cafe::snduser2
