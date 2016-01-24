#pragma once
#include "types.h"
#include "snd_core_result.h"
#include "utils/be_val.h"

namespace snd_core
{

enum AXDRCVSMode : uint32_t
{
   UNKNOWN,
};

enum AXDRCVSSpeakerPosition : uint32_t
{
   UNKNOWN,
};

enum AXDRCVSSurroundLevelGain : uint32_t
{
   UNKNOWN,
};

enum AXDRCVSLC : uint32_t
{
   UNKNOWN,
};

enum AXDRCOutput : uint32_t
{
   UNKNOWN,
};

AXResult::Result
AXGetDRCVSMode(be_val<AXDRCVSMode> *mode);

AXResult::Result
AXSetDRCVSMode(AXDRCVSMode mode);

AXResult::Result
AXSetDRCVSDownmixBalance(AXDRCOutput output,
                         float balance);

AXResult::Result
AXSetDRCVSLC(AXDRCVSLC lc);

AXResult::Result
AXSetDRCVSLimiter(BOOL limit);

AXResult::Result
AXSetDRCVSLimiterThreshold(float threshold);

AXResult::Result
AXSetDRCVSOutputGain(AXDRCOutput output,
                     float gain);

AXResult::Result
AXSetDRCVSSpeakerPosition(AXDRCOutput output,
                          AXDRCVSSpeakerPosition pos);

AXResult::Result
AXSetDRCVSSurroundDepth(AXDRCOutput output,
                        float depth);

AXResult::Result
AXSetDRCVSSurroundLevelGain(AXDRCVSSurroundLevelGain gain);

} // namespace snd_core
