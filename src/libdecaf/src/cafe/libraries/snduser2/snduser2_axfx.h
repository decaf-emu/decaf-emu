#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{
using AXFXAllocFuncPtr = virt_func_ptr<virt_ptr<void>(uint32_t size)>;
using AXFXFreeFuncPtr = virt_func_ptr<void(virt_ptr<void> ptr)>;

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

#pragma pack(push, 1)
struct AXFXReverbHi
{
   be2_array<virt_ptr<float>, 3>                         earlyLine;
   be2_array<uint32_t, 3>                                earlyPos;
   be2_val<uint32_t>                                     earlyLength;
   be2_val<uint32_t>                                     earlyMaxLength;
   be2_array<float, 3>                                   earlyCoef;
   be2_array<virt_ptr<float>, 3>                         preDelayLine;
   be2_val<uint32_t>                                     preDelayPos;
   be2_val<uint32_t>                                     preDelayLength;
   be2_val<uint32_t>                                     preDelayMaxLength;
   be2_array<virt_ptr<be2_array<virt_ptr<float>, 3>>, 3> combLine;
   be2_array<uint32_t, 3>                                combPos;
   be2_array<uint32_t, 3>                                combLength;
   be2_array<uint32_t, 3>                                combMaxLength;
   be2_array<float, 3>                                   combCoef;
   be2_array<virt_ptr<be2_array<virt_ptr<float>, 2>>, 3> allpassLine;
   be2_array<uint32_t, 2>                                allpassPos;
   be2_array<uint32_t, 2>                                allpassLength;
   be2_array<uint32_t, 2>                                allpassMaxLength;
   be2_array<virt_ptr<float>, 3>                          lastAllpassLine;
   be2_array<uint32_t, 3>                                lastAllpassPos;
   be2_array<uint32_t, 3>                                lastAllpassLength;
   be2_array<uint32_t, 3>                                lastAllpassMaxLength;
   be2_val<float>                                        allpassCoef;
   be2_array<float, 3>                                   lastLpfOut;
   be2_val<float>                                        lpfCoef;
   be2_val<uint32_t>                                     active;

   // user params
   be2_val<uint32_t>                                     earlyMode;        // early reflection mode
   be2_val<float>                                        preDelayTimeMax;  // max of pre delay time of fused reverb (sec)
   be2_val<float>                                        preDelayTime;     // pre delay time of fused reverb (sec)
   be2_val<uint32_t>                                     fusedMode;        // fused reverb mode
   be2_val<float>                                        fusedTime;        // fused reverb time (sec)
   be2_val<float>                                        coloration;       // coloration of all-pass filter (0.f - 1.f)
   be2_val<float>                                        damping;          // damping of timbre  (0.f - 1.f)
   be2_val<float>                                        crosstalk;        // crosstalk of each channels
   be2_val<float>                                        earlyGain;        // output gain of early reflection (0.f - 1.f)
   be2_val<float>                                        fusedGain;        // output gain of fused reverb (0.f - 1.f)

   virt_ptr<AXFXBuffers>                                 busIn;
   virt_ptr<AXFXBuffers>                                 busOut;
   be2_val<float>                                        outGain;
   be2_val<float>                                        sendGain;
};
CHECK_OFFSET(AXFXReverbHi, 0x00, earlyLine);
CHECK_OFFSET(AXFXReverbHi, 0x0C, earlyPos);
CHECK_OFFSET(AXFXReverbHi, 0x18, earlyLength);
CHECK_OFFSET(AXFXReverbHi, 0x1C, earlyMaxLength);
CHECK_OFFSET(AXFXReverbHi, 0x20, earlyCoef);
CHECK_OFFSET(AXFXReverbHi, 0x2C, preDelayLine);
CHECK_OFFSET(AXFXReverbHi, 0x38, preDelayPos);
CHECK_OFFSET(AXFXReverbHi, 0x3C, preDelayLength);
CHECK_OFFSET(AXFXReverbHi, 0x40, preDelayMaxLength);
CHECK_OFFSET(AXFXReverbHi, 0x44, combLine);
CHECK_OFFSET(AXFXReverbHi, 0x50, combPos);
CHECK_OFFSET(AXFXReverbHi, 0x5C, combLength);
CHECK_OFFSET(AXFXReverbHi, 0x68, combMaxLength);
CHECK_OFFSET(AXFXReverbHi, 0x74, combCoef);
CHECK_OFFSET(AXFXReverbHi, 0x80, allpassLine);
CHECK_OFFSET(AXFXReverbHi, 0x8C, allpassPos);
CHECK_OFFSET(AXFXReverbHi, 0x94, allpassLength);
CHECK_OFFSET(AXFXReverbHi, 0x9C, allpassMaxLength);
CHECK_OFFSET(AXFXReverbHi, 0xA4, lastAllpassLine);
CHECK_OFFSET(AXFXReverbHi, 0xB0, lastAllpassPos);
CHECK_OFFSET(AXFXReverbHi, 0xBC, lastAllpassLength);
CHECK_OFFSET(AXFXReverbHi, 0xC8, lastAllpassMaxLength);
CHECK_OFFSET(AXFXReverbHi, 0xD4, allpassCoef);
CHECK_OFFSET(AXFXReverbHi, 0xD8, lastLpfOut);
CHECK_OFFSET(AXFXReverbHi, 0xE4, lpfCoef);
CHECK_OFFSET(AXFXReverbHi, 0xE8, active);
CHECK_OFFSET(AXFXReverbHi, 0xEC, earlyMode);
CHECK_OFFSET(AXFXReverbHi, 0xF0, preDelayTimeMax);
CHECK_OFFSET(AXFXReverbHi, 0xF4, preDelayTime);
CHECK_OFFSET(AXFXReverbHi, 0xF8, fusedMode);
CHECK_OFFSET(AXFXReverbHi, 0xFC, fusedTime);
CHECK_OFFSET(AXFXReverbHi, 0x100, coloration);
CHECK_OFFSET(AXFXReverbHi, 0x104, damping);
CHECK_OFFSET(AXFXReverbHi, 0x108, crosstalk);
CHECK_OFFSET(AXFXReverbHi, 0x10C, earlyGain);
CHECK_OFFSET(AXFXReverbHi, 0x110, fusedGain);
CHECK_OFFSET(AXFXReverbHi, 0x114, busIn);
CHECK_OFFSET(AXFXReverbHi, 0x118, busOut);
CHECK_OFFSET(AXFXReverbHi, 0x11C, outGain);
CHECK_OFFSET(AXFXReverbHi, 0x120, sendGain);
CHECK_SIZE(AXFXReverbHi, 0x124);

struct AXFXDelay
{
   // do not touch these!
   be2_array <virt_ptr<int32_t>, 3>  line;
   be2_array<uint32_t, 3>           curPos;
   be2_array<uint32_t, 4>           length;
   be2_array<int32_t, 3>            feedbackGain;
   be2_array<int32_t, 3>            outGain;
   be2_val<uint32_t>                active;

   // user params
   be2_array<uint32_t, 3>           delay;       // Delay buffer length in ms per channel
   be2_array<uint32_t, 3>           feedback;    // Feedback volume in % per channel
   be2_array<uint32_t, 3>           output;      // Output volume in % per channel
};
CHECK_OFFSET(AXFXDelay, 0x00, line);
CHECK_OFFSET(AXFXDelay, 0x0C, curPos);
CHECK_OFFSET(AXFXDelay, 0x18, length);
CHECK_OFFSET(AXFXDelay, 0x28, feedbackGain);
CHECK_OFFSET(AXFXDelay, 0x34, outGain);
CHECK_OFFSET(AXFXDelay, 0x40, active);
CHECK_OFFSET(AXFXDelay, 0x44, delay);
CHECK_OFFSET(AXFXDelay, 0x50, feedback);
CHECK_OFFSET(AXFXDelay, 0x5C, output);
CHECK_SIZE(AXFXDelay, 0x68);
#pragma pack(pop, 1)

struct AXFXChorus;
struct AXFXReverbStd;

struct StaticFxData
{
   be2_val<AXFXAllocFuncPtr> allocFuncPtr;
   be2_val<AXFXFreeFuncPtr> freeFuncPtr;
};

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn);
void
AXFXGetHooks(virt_ptr<AXFXAllocFuncPtr> allocFn,
             virt_ptr<AXFXFreeFuncPtr> freeFn);

int32_t
AXFXChorusGetMemSize(virt_ptr<AXFXChorus> chorus);

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorus> chorus);

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> delay);

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> delay);

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> reverb);

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> reverb);

void
AXFXChorusCallback(virt_ptr<AXFXBuffers> buffers,
                   virt_ptr<AXFXChorus> data,
                   virt_ptr<AXAuxCallbackData> auxData);

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorus> data);

void
AXFXDelayCallback(virt_ptr<AXFXBuffers> buffers,
                  virt_ptr<AXFXDelay> data,
                  virt_ptr<AXAuxCallbackData> auxData);

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelay> data);

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXReverbHi> data,
                     virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                        virt_ptr<AXFXReverbHi> data);

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> data,
                      virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                         virt_ptr<AXFXReverbStd> data);

BOOL
AXFXChorusInit(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXChorusExpInit(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXDelayInit(virt_ptr<AXFXDelay> delay);

BOOL
AXFXDelayExpInit(virt_ptr<AXFXDelay> delay);

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbHi> reverb);

void
AXFXChorusShutdown(virt_ptr<AXFXChorus> chorus);

void
AXFXChorusExpShutdown(virt_ptr<AXFXChorus> chorus);

void
AXFXDelayShutdown(virt_ptr<AXFXDelay> delay);

void
AXFXDelayExpShutdown(virt_ptr<AXFXDelay> delay);

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
AXFXReverbHiSettings();

BOOL
AXFXMultiChReverbInit();

} // namespace cafe::snduser2
