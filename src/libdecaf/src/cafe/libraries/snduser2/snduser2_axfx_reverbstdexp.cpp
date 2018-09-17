#include "snduser2.h"
#include "snduser2_axfx_reverbstdexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

static constexpr int32_t
ReverbBaseMemSizeSamples = 0x503;

static constexpr int32_t
MemSizeSamplesTable[] = {
   0x6FD,
   0x7CF,
   0x1B1,
   0x95,
   0x95,
   0x125,
   0xFB,
   0x67,
   0x3B3,
   0x551,
   0x1B1,
   0x89,
   0x4FF,
   0x5FB,
   0x1FD,
   0x95,
   0x5FB,
   0x737,
   0x233,
   0xB3,
   0x71F,
   0x935,
   0x23B,
   0x89,
   0x71F,
   0x935,
   0x23B,
   0xB3,
   0xA3,
   0x13D,
   0x1DF,
   0x281,
   0x31D,
   0x3C7,
   0x463,
};

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStdExp> reverb)
{
   auto delayMemSamples = reverb->preDelaySeconds * static_cast<float>(AXGetInputSamplesPerSec());
   auto numSamples =
      delayMemSamples +
      ReverbBaseMemSizeSamples +
      MemSizeSamplesTable[24] + MemSizeSamplesTable[25] +
      MemSizeSamplesTable[26] + MemSizeSamplesTable[27];

   return 3 * sizeof(int32_t) * numSamples;
}

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbStdExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXReverbStdExpShutdown(virt_ptr<AXFXReverbStdExp> reverb)
{
   decaf_warn_stub();
}

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStdExp> reverb)
{
   decaf_warn_stub();
}

BOOL
AXFXReverbStdExpSettings(virt_ptr<AXFXReverbStdExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbStdExpSettingsUpdate(virt_ptr<AXFXReverbStdExp> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxReverbStdExpSymbols()
{
   RegisterFunctionExport(AXFXReverbStdExpGetMemSize);
   RegisterFunctionExport(AXFXReverbStdExpInit);
   RegisterFunctionExport(AXFXReverbStdExpShutdown);
   RegisterFunctionExport(AXFXReverbStdExpCallback);
   RegisterFunctionExport(AXFXReverbStdExpSettings);
   RegisterFunctionExport(AXFXReverbStdExpSettingsUpdate);
}

} // namespace cafe::snduser2
