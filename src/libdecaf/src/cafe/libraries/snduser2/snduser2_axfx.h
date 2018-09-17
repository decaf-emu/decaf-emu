#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbMulti;

enum AXFXSampleRate
{
   Rate32khz = 1,
   Rate48khz = 2,
};

struct AXAuxCallbackData
{
   be2_val<uint32_t> channels;
   be2_val<uint32_t> samples;
};
CHECK_OFFSET(AXAuxCallbackData, 0x0, channels);
CHECK_OFFSET(AXAuxCallbackData, 0x4, samples);
CHECK_SIZE(AXAuxCallbackData, 0x8);

struct AXFXBuffers
{
   virt_ptr<int32_t> left;
   virt_ptr<int32_t> right;
   virt_ptr<int32_t> surround;
};
CHECK_OFFSET(AXFXBuffers, 0x00, left);
CHECK_OFFSET(AXFXBuffers, 0x04, right);
CHECK_OFFSET(AXFXBuffers, 0x08, surround);
CHECK_SIZE(AXFXBuffers, 0x0C);


void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXReverbMulti> data,
                          virt_ptr<AXAuxCallbackData> auxData);

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXReverbMulti> reverb,
                      uint32_t unk,
                      AXFXSampleRate sampleRate);

} // namespace cafe::snduser2
