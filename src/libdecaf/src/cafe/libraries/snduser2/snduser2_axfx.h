#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXReverbHi;
struct AXFXReverbMulti;
struct AXFXReverbStd;

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

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> reverb);

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> reverb);

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXReverbHi> data,
                     virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                        virt_ptr<AXFXReverbHi> data);

void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXReverbMulti> data,
                          virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> data,
                      virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                         virt_ptr<AXFXReverbStd> data);

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbHiExpShutdown(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbStdShutdown(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbStdExpShutdown(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiShutdown(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiSettings(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXReverbMulti> reverb,
                      uint32_t unk,
                      AXFXSampleRate sampleRate);

void
AXARTServiceSounds();

virt_ptr<void>
SPGetSoundEntry(virt_ptr<void> table,
                uint32_t index);

} // namespace cafe::snduser2
