#pragma once
#include "snd_core_enum.h"

#include <common/be_val.h>
#include <common/cbool.h>

namespace snd_core
{

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
