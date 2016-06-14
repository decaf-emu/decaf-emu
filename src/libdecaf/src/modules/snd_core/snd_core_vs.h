#pragma once
#include "common/types.h"
#include "snd_core_result.h"
#include "common/be_val.h"

namespace snd_core
{

enum AXDRCVSMode : uint32_t
{
   AX_DRC_VS_MODE_UNKNOWN,
};

enum AXDRCVSSpeakerPosition : uint32_t
{
   AX_DRC_VS_SPEAKER_POS_UNKNOWN,
};

enum AXDRCVSSurroundLevelGain : uint32_t
{
   AX_DRC_VS_SURROUND_LEVEL_GAIN_UNKNOWN,
};

enum AXDRCVSLC : uint32_t
{
   AX_DRC_VS_LC_UNKNOWN,
};

enum AXDRCOutput : uint32_t
{
   AX_DRC_OUTPUT_UNKNOWN,
};

AXResult
AXGetDRCVSMode(be_val<AXDRCVSMode> *mode);

AXResult
AXSetDRCVSMode(AXDRCVSMode mode);

AXResult
AXSetDRCVSDownmixBalance(AXDRCOutput output,
                         float balance);

AXResult
AXSetDRCVSLC(AXDRCVSLC lc);

AXResult
AXSetDRCVSLimiter(BOOL limit);

AXResult
AXSetDRCVSLimiterThreshold(float threshold);

AXResult
AXSetDRCVSOutputGain(AXDRCOutput output,
                     float gain);

AXResult
AXSetDRCVSSpeakerPosition(AXDRCOutput output,
                          AXDRCVSSpeakerPosition pos);

AXResult
AXSetDRCVSSurroundDepth(AXDRCOutput output,
                        float depth);

AXResult
AXSetDRCVSSurroundLevelGain(AXDRCVSSurroundLevelGain gain);

} // namespace snd_core
