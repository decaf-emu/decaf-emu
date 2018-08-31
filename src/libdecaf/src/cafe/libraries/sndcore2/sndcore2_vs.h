#pragma once
#include "sndcore2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::sndcore2
{

AXResult
AXGetDRCVSMode(virt_ptr<AXDRCVSMode> outMode);

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

} // namespace cafe::sndcore2
