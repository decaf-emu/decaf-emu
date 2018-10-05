#pragma once
#include "snduser2_axfx.h"
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

enum AXFXDelayExpStateFlags
{
   Shutdown    = 1 << 0,
   Initialised = 1 << 1,
};

struct AXFXDelayExp
{
   UNKNOWN(0x34);

   //! State
   be2_val<AXFXDelayExpStateFlags> stateFlags;

   //! User provided parameter for duration of the delay buffer.
   be2_val<float> userDelayMS;

   // Unknown size...
};
CHECK_OFFSET(AXFXDelayExp, 0x34, stateFlags);
CHECK_OFFSET(AXFXDelayExp, 0x38, userDelayMS);
UNKNOWN_SIZE(AXFXDelayExp);

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelayExp> delay);

BOOL
AXFXDelayExpInit(virt_ptr<AXFXDelayExp> delay);

void
AXFXDelayExpShutdown(virt_ptr<AXFXDelayExp> delay);

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelayExp> delay);

BOOL
AXFXDelayExpSettings(virt_ptr<AXFXDelayExp> delay);

BOOL
AXFXDelayExpSettingsUpdate(virt_ptr<AXFXDelayExp> delay);

} // namespace cafe::snduser2
