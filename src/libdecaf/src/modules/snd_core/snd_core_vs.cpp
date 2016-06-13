#include "snd_core.h"
#include "snd_core_vs.h"

namespace snd_core
{

AXResult
AXGetDRCVSMode(be_val<AXDRCVSMode> *mode)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSMode(AXDRCVSMode mode)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSDownmixBalance(AXDRCOutput output,
                         float balance)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSLC(AXDRCVSLC lc)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSLimiter(BOOL limit)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSLimiterThreshold(float threshold)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSOutputGain(AXDRCOutput output,
                     float gain)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSSpeakerPosition(AXDRCOutput output,
                          AXDRCVSSpeakerPosition pos)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSSurroundDepth(AXDRCOutput output,
                        float depth)
{
   return AXResult::Success;
}

AXResult
AXSetDRCVSSurroundLevelGain(AXDRCVSSurroundLevelGain gain)
{
   return AXResult::Success;
}

void
Module::registerVSFunctions()
{
   RegisterKernelFunction(AXGetDRCVSMode);
   RegisterKernelFunction(AXSetDRCVSMode);
   RegisterKernelFunction(AXSetDRCVSDownmixBalance);
   RegisterKernelFunction(AXSetDRCVSLC);
   RegisterKernelFunction(AXSetDRCVSLimiter);
   RegisterKernelFunction(AXSetDRCVSLimiterThreshold);
   RegisterKernelFunction(AXSetDRCVSOutputGain);
   RegisterKernelFunction(AXSetDRCVSSpeakerPosition);
   RegisterKernelFunction(AXSetDRCVSSurroundDepth);
   RegisterKernelFunction(AXSetDRCVSSurroundLevelGain);
}

} // namespace snd_core
