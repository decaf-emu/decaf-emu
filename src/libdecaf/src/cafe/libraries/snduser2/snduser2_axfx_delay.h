#pragma once
#include "snduser2_axfx.h"
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

enum AXFXDelayStateFlags
{
   Shutdown    = 1 << 0,
   Initialised = 1 << 1,
};

struct AXFXDelay
{
   //! Buffer to store the delayed samples in.
   be2_array<virt_ptr<uint32_t>, 3> delayBuffer;

   //! Current position of per channel delayBuffer in number of samples.
   be2_array<uint32_t, 3> delayBufferSamplePos;

   //! Size of the per channel delayBuffer in number of samples.
   be2_array<uint32_t, 3> delayBufferMaxNumSamples;

   //! Feedback gain.
   be2_array<int32_t, 3> feedbackGain;

   //! Output gain.
   be2_array<int32_t, 3> outputGain;

   //! State.
   be2_val<AXFXDelayStateFlags> stateFlags;

   //! User provided parameter for duration of the delay.
   be2_array<uint32_t, 3> userDelayMS;

   //! User provided parameter for feedbackGain
   be2_array<uint32_t, 3> userFeedbackGain;

   //! User provided parameter for outputGain
   be2_array<uint32_t, 3> userOutputGain;
};
CHECK_OFFSET(AXFXDelay, 0x00, delayBuffer);
CHECK_OFFSET(AXFXDelay, 0x0C, delayBufferSamplePos);
CHECK_OFFSET(AXFXDelay, 0x18, delayBufferMaxNumSamples);
CHECK_OFFSET(AXFXDelay, 0x3C, stateFlags);
CHECK_OFFSET(AXFXDelay, 0x40, userDelayMS);
CHECK_OFFSET(AXFXDelay, 0x4C, userFeedbackGain);
CHECK_OFFSET(AXFXDelay, 0x58, userOutputGain);
CHECK_SIZE(AXFXDelay, 0x64);

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> delay);

BOOL
AXFXDelayInit(virt_ptr<AXFXDelay> delay);

void
AXFXDelayShutdown(virt_ptr<AXFXDelay> delay);

void
AXFXDelayCallback(virt_ptr<AXFXBuffers> buffers,
                  virt_ptr<AXFXDelay> delay);

} // namespace cafe::snduser2
