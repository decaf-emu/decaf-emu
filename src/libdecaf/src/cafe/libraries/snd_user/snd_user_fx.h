#pragma once
#include <libcpu/be2_struct.h>
#include "../sndcore2/sndcore2_enum.h"
namespace cafe::snd_user
{

using AXFXAllocFuncPtr = virt_func_ptr<
      virt_ptr<void>(uint32_t size)
>;

using AXFXFreeFuncPtr = virt_func_ptr<
void(virt_ptr<void> ptr)
>;

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

struct AXFXReverbHi
{
   float       *earlyLine[3];
   uint32_t     earlyPos[3];
   uint32_t     earlyLength;
   uint32_t     earlyMaxLength;
   float        earlyCoef[3];
   float       *preDelayLine[3];
   uint32_t     preDelayPos;
   uint32_t     preDelayLength;
   uint32_t     preDelayMaxLength;
   float       *combLine[3][3];
   uint32_t     combPos[3];
   uint32_t     combLength[3];
   uint32_t     combMaxLength[3];
   float        combCoef[3];
   float       *allpassLine[3][2];
   uint32_t     allpassPos[2];
   uint32_t     allpassLength[2];
   uint32_t     allpassMaxLength[2];
   float       *lastAllpassLine[3];
   uint32_t     lastAllpassPos[3];
   uint32_t     lastAllpassLength[3];
   uint32_t     lastAllpassMaxLength[3];
   float        allpassCoef;
   float        lastLpfOut[3];
   float        lpfCoef;
   uint32_t     active;

   // user params
   uint32_t     earlyMode;        // early reflection mode
   float        preDelayTimeMax;  // max of pre delay time of fused reverb (sec)
   float        preDelayTime;     // pre delay time of fused reverb (sec)
   uint32_t     fusedMode;        // fused reverb mode
   float        fusedTime;        // fused reverb time (sec)
   float        coloration;       // coloration of all-pass filter (0.f - 1.f)
   float        damping;          // damping of timbre  (0.f - 1.f)
   float        crosstalk;        // crosstalk of each channels
   float        earlyGain;        // output gain of early reflection (0.f - 1.f)
   float        fusedGain;        // output gain of fused reverb (0.f - 1.f)

   AXFXBuffers *busIn;
   AXFXBuffers *busOut;
   float        outGain;
   float        sendGain;
};


struct AXFXDelay
{
   // do not touch these!
   int32_t       *line[3];
   uint32_t       curPos[3];
   uint32_t       length[3];
   int32_t        feedbackGain[3];
   int32_t        outGain[3];
   uint32_t       active;

   // user params
   uint32_t       delay[3];       // Delay buffer length in ms per channel
   uint32_t       feedback[3];    // Feedback volume in % per channel
   uint32_t       output[3];      // Output volume in % per channel
};

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> chorus);

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> chorus);

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> chorus);

} // namespace cafe::sndcore2