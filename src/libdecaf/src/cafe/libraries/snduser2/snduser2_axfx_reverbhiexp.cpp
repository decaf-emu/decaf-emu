#include "snduser2.h"
#include "snduser2_axfx_reverbhiexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

static constexpr int32_t
ReverbHiMemSizeSamplesBase = 0xF31;

static constexpr int32_t
ReverbHiMemSizeSamplesTable[] = {
   0x6FD,
   0x7CF,
   0x91D,
   0x1B1,
   0x95,
   0x2F,
   0x49,
   0x43,
   0x95,
   0x125,
   0x1C1,
   0xFB,
   0x67,
   0x2F,
   0x49,
   0x43,
   0x3B3,
   0x551,
   0x5FB,
   0x1B1,
   0x89,
   0x2F,
   0x49,
   0x43,
   0x4FF,
   0x5FB,
   0x7B5,
   0x1FD,
   0x95,
   0x2F,
   0x49,
   0x43,
   0x5FB,
   0x737,
   0x8F9,
   0x233,
   0xB3,
   0x2F,
   0x49,
   0x43,
   0x71F,
   0x935,
   0xA85,
   0x23B,
   0x89,
   0x2F,
   0x49,
   0x43,
   0x71F,
   0x935,
   0xA85,
   0x23B,
   0xB3,
   0x2F,
   0x49,
   0x43,
};

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHiExp> reverb)
{
   auto delaySamples =
      reverb->preDelaySeconds * static_cast<float>(AXGetInputSamplesPerSec());

   auto perChanSamples =
      delaySamples +
      ReverbHiMemSizeSamplesBase +
      ReverbHiMemSizeSamplesTable[48] + ReverbHiMemSizeSamplesTable[49] + ReverbHiMemSizeSamplesTable[50] +
      ReverbHiMemSizeSamplesTable[51] + ReverbHiMemSizeSamplesTable[52];

   auto numSamples =
      perChanSamples * 3 +
      ReverbHiMemSizeSamplesTable[53] + ReverbHiMemSizeSamplesTable[54] + ReverbHiMemSizeSamplesTable[55];

   return static_cast<int32_t>(sizeof(int32_t) * numSamples);
}

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHiExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXReverbHiExpShutdown(virt_ptr<AXFXReverbHiExp> reverb)
{
   decaf_warn_stub();
}

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbHiExp> reverb)
{
   decaf_warn_stub();
}

BOOL
AXFXReverbHiExpSettings(virt_ptr<AXFXReverbHiExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiExpSettingsUpdate(virt_ptr<AXFXReverbHiExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxReverbHiExpSymbols()
{
   RegisterFunctionExport(AXFXReverbHiExpGetMemSize);
   RegisterFunctionExport(AXFXReverbHiExpInit);
   RegisterFunctionExport(AXFXReverbHiExpShutdown);
   RegisterFunctionExport(AXFXReverbHiExpCallback);
   RegisterFunctionExport(AXFXReverbHiExpSettings);
   RegisterFunctionExport(AXFXReverbHiExpSettingsUpdate);
}

} // namespace cafe::snduser2
