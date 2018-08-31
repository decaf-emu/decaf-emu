#ifndef CAFE_SNDCORE2_ENUM_H
#define CAFE_SNDCORE2_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(sndcore2)

ENUM_BEG(AXAuxId, uint32_t)
   ENUM_VALUE(A,                 0)
   ENUM_VALUE(B,                 1)
   ENUM_VALUE(C,                 2)
   ENUM_VALUE(Max,               3)
ENUM_END(AXAuxId)

ENUM_BEG(AXBusType, uint32_t)
   ENUM_VALUE(Main,              0)
   ENUM_VALUE(A,                 1)
   ENUM_VALUE(B,                 2)
   ENUM_VALUE(C,                 3)
   ENUM_VALUE(Max,               4)
ENUM_END(AXBusType)

ENUM_BEG(AXChannels, uint32_t)
   ENUM_VALUE(Left,              0)
   ENUM_VALUE(Right,             1)
   ENUM_VALUE(LeftSurround,      2)
   ENUM_VALUE(RightSurround,     3)
   ENUM_VALUE(Center,            4)
   ENUM_VALUE(Sub,               5)
   ENUM_VALUE(Max,               6)
ENUM_END(AXChannels)

ENUM_BEG(AXDeviceMode, uint32_t)
   // Unknown
ENUM_END(AXDeviceMode)

ENUM_BEG(AXDeviceType, uint32_t)
   ENUM_VALUE(TV,                0)
   ENUM_VALUE(DRC,               1)
   ENUM_VALUE(RMT,               2)  // Classic Controller, Wiimote etc.
   ENUM_VALUE(Max,               3)
ENUM_END(AXDeviceType)

ENUM_BEG(AXDRCOutput, uint32_t)
   // Unknown
ENUM_END(AXDRCOutput)

ENUM_BEG(AXDRCVSLC, uint32_t)
   // Unknown
ENUM_END(AXDRCVSLC)

ENUM_BEG(AXDRCVSMode, uint32_t)
   // Unknown
ENUM_END(AXDRCVSMode)

ENUM_BEG(AXDRCVSSpeakerPosition, uint32_t)
   // Unknown
ENUM_END(AXDRCVSSpeakerPosition)

ENUM_BEG(AXDRCVSSurroundLevelGain, uint32_t)
   // Unknown
ENUM_END(AXDRCVSSurroundLevelGain)

ENUM_BEG(AXInitPipeline, uint32_t)
   ENUM_VALUE(Single,            0)
   ENUM_VALUE(FourStage,         1)
ENUM_END(AXInitPipeline)

ENUM_BEG(AXRendererFreq, uint32_t)
   ENUM_VALUE(Freq32khz,          0)
   ENUM_VALUE(Freq48khz,          1)
ENUM_END(AXRendererFreq)

ENUM_BEG(AXResult, int32_t)
   ENUM_VALUE(Success,           0)
   ENUM_VALUE(InvalidDeviceType, -1)
   ENUM_VALUE(InvalidDRCVSMode,  -13)
   ENUM_VALUE(TooManyCallbacks,  -15)
   ENUM_VALUE(CallbackNotFound,  -16)
   ENUM_VALUE(CallbackInvalid,   -17)
   ENUM_VALUE(VoiceIsRunning,    -18)
   ENUM_VALUE(DelayTooBig,       -19)
ENUM_END(AXResult)

ENUM_BEG(AXVoiceFormat, uint16_t)
   ENUM_VALUE(ADPCM,             0x00)
   ENUM_VALUE(LPCM16,            0x0A)
   ENUM_VALUE(LPCM8,             0x19)
ENUM_END(AXVoiceFormat)

ENUM_BEG(AXVoiceLoop, uint16_t)
   ENUM_VALUE(Disabled,          0)
   ENUM_VALUE(Enabled,           1)
ENUM_END(AXVoiceLoop)

ENUM_BEG(AXRenderer, uint32_t)
   ENUM_VALUE(DSP, 0)
   ENUM_VALUE(CPU, 1)
   ENUM_VALUE(Auto, 2)
ENUM_END(AXRenderer)

ENUM_BEG(AXVoiceSrcType, uint32_t)
   ENUM_VALUE(None, 0)
   ENUM_VALUE(Linear, 1)
   ENUM_VALUE(Unk0, 2)
   ENUM_VALUE(Unk1, 3)
   ENUM_VALUE(Unk2, 4)
ENUM_END(AXVoiceSrcType)

ENUM_BEG(AXVoiceState, uint32_t)
   ENUM_VALUE(Stopped,           0)
   ENUM_VALUE(Playing,           1)
ENUM_END(AXVoiceState)

ENUM_BEG(AXVoiceType, uint16_t)
   ENUM_VALUE(Default, 0)
   ENUM_VALUE(Streaming, 1)
ENUM_END(AXVoiceType)

ENUM_BEG(AXVoiceSrcRatioResult, int32_t)
   ENUM_VALUE(Success,                    0)
   ENUM_VALUE(RatioLessThanZero,          -1)
   ENUM_VALUE(RatioGreaterThanSomething,  -2)
ENUM_END(AXVoiceSrcRatioResult)

ENUM_NAMESPACE_BEG(internal)

ENUM_BEG(AXVoiceSyncBits, uint32_t)
   ENUM_VALUE(SrcType,       1 << 0)

   ENUM_VALUE(State,         1 << 2)
   ENUM_VALUE(Type,          1 << 3)

   ENUM_VALUE(Itd,           1 << 5)
   ENUM_VALUE(ItdTarget,     1 << 6)

   ENUM_VALUE(Ve,            1 << 8)
   ENUM_VALUE(VeDelta,       1 << 9)
   ENUM_VALUE(Addr,          1 << 10)
   ENUM_VALUE(Loop,          1 << 11)
   ENUM_VALUE(LoopOffset,    1 << 12)
   ENUM_VALUE(EndOffset,     1 << 13)
   ENUM_VALUE(CurrentOffset, 1 << 14)
   ENUM_VALUE(Adpcm,         1 << 15)
   ENUM_VALUE(Src,           1 << 16)
   ENUM_VALUE(SrcRatio,      1 << 17)
   ENUM_VALUE(AdpcmLoop,     1 << 18)
   ENUM_VALUE(Lpf,           1 << 19)
   ENUM_VALUE(LpfCoefs,      1 << 20)
   ENUM_VALUE(Biquad,        1 << 21)
   ENUM_VALUE(BiquadCoefs,   1 << 22)
   ENUM_VALUE(RmtOn,         1 << 23)

   ENUM_VALUE(RmtSrc,        1 << 27)
   ENUM_VALUE(RmtIIR,        1 << 28)
   ENUM_VALUE(RmtIIRCoefs0,  1 << 29)
   ENUM_VALUE(RmtIIRCoefs1,  1 << 30)
ENUM_END(AXVoiceSyncBits)

ENUM_NAMESPACE_END(internal)

ENUM_NAMESPACE_END(sndcore2)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_SNDCORE2_ENUM_H
