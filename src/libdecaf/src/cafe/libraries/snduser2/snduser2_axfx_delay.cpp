#include "snduser2.h"
#include "snduser2_axfx_delay.h"
#include "snduser2_axfx_hooks.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> delay)
{
   auto samplersPerMS = AXGetInputSamplesPerSec() / 1000;
   auto totalDelayMS =
      delay->userDelayMS[0] + delay->userDelayMS[1] + delay->userDelayMS[2];

   return sizeof(int32_t) * totalDelayMS * samplersPerMS;
}

BOOL
AXFXDelayInit(virt_ptr<AXFXDelay> delay)
{
   auto samplersPerMS = AXGetInputSamplesPerSec() / 1000;
   delay->stateFlags = AXFXDelayStateFlags::Shutdown;

   // Calculate delay buffer size
   for (auto i = 0u; i < 3; ++i) {
      if (!delay->userDelayMS[i]) {
         AXFXDelayShutdown(delay);
         return FALSE;
      }

      delay->delayBufferMaxNumSamples[i] =
         samplersPerMS * delay->userDelayMS[i];
   }

   // Allocate delay buffers
   for (auto i = 0u; i < 3; ++i) {
      delay->delayBuffer[i] =
         virt_cast<uint32_t *>(
            internal::axfxAlloc(4 * delay->delayBufferMaxNumSamples[i]));

      if (!delay->delayBuffer[i]) {
         AXFXDelayShutdown(delay);
         return FALSE;
      }
   }

   // Initialise parameters
   for (auto i = 0u; i < 3; ++i) {
      if (delay->userFeedbackGain[i] > 100 ||
          delay->userOutputGain[i] > 100) {
         AXFXDelayShutdown(delay);
         return FALSE;
      }

      delay->delayBufferSamplePos[i] = 0u;
      delay->feedbackGain[i] = static_cast<int32_t>(128.0f * (delay->userFeedbackGain[i] / 100.0f));
      delay->outputGain[i] = static_cast<int32_t>(128.0f * (delay->userOutputGain[i] / 100.0f));
   }

   return TRUE;
}

void
AXFXDelayShutdown(virt_ptr<AXFXDelay> delay)
{
   delay->stateFlags |= AXFXDelayStateFlags::Shutdown;

   for (auto i = 0u; i < 3; ++i) {
      internal::axfxFree(delay->delayBuffer[i]);
      delay->delayBuffer[i] = nullptr;
   }
}

void
AXFXDelayCallback(virt_ptr<AXFXBuffers> buffers,
                  virt_ptr<AXFXDelay> delay)
{
   // TODO: Implement AXFXDelayCallback
   decaf_warn_stub();
}

void
Library::registerAxfxDelaySymbols()
{
   RegisterFunctionExport(AXFXDelayGetMemSize);
   RegisterFunctionExport(AXFXDelayInit);
   RegisterFunctionExport(AXFXDelayShutdown);
   RegisterFunctionExport(AXFXDelayCallback);
}

} // namespace cafe::snduser2
