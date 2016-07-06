#ifndef SND_CORE_ENUM_H
#define SND_CORE_ENUM_H

#include "common/enum_start.h"

ENUM_NAMESPACE_BEG(snd_core)

ENUM_BEG(AXDeviceMode, uint32_t)
   // Unknown
ENUM_END(AXDeviceMode)

ENUM_BEG(AXDeviceType, uint32_t)
   // Unknown
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

ENUM_BEG(AXResult, int32_t)
   ENUM_VALUE(Success,           0)
   ENUM_VALUE(InvalidDeviceType, -1)
   ENUM_VALUE(InvalidDRCVSMode,  -13)
ENUM_END(AXResult)

ENUM_BEG(AXVoiceLoop, uint16_t)
   ENUM_VALUE(Disabled,          0)
   ENUM_VALUE(Enabled,           1)
ENUM_END(AXVoiceLoop)

ENUM_BEG(AXVoiceSrcType, uint32_t)
ENUM_END(AXVoiceSrcType)

ENUM_BEG(AXVoiceState, uint32_t)
   ENUM_VALUE(Stopped,           0)
   ENUM_VALUE(Playing,           1)
ENUM_END(AXVoiceState)

ENUM_BEG(AXVoiceType, uint32_t)
ENUM_END(AXVoiceType)

ENUM_BEG(AXVoiceSrcRatioResult, int32_t)
   ENUM_VALUE(Success,                    0)
   ENUM_VALUE(RatioLessThanZero,          -1)
   ENUM_VALUE(RatioGreaterThanSomething,  -2)
ENUM_END(AXVoiceSrcRatioResult)

ENUM_NAMESPACE_END(snd_core)

#include "common/enum_end.h"

#endif // ifdef SND_CORE_ENUM_H
