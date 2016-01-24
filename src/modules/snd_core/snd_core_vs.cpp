#include "snd_core.h"
#include "snd_core_vs.h"

namespace snd_core
{

AXResult::Result
AXGetDRCVSMode(be_val<AXDRCVSMode> *mode)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSMode(AXDRCVSMode mode)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSDownmixBalance(AXDRCOutput output,
                         float balance)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSLC(AXDRCVSLC lc)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSLimiter(BOOL limit)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSLimiterThreshold(float threshold)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSOutputGain(AXDRCOutput output,
                     float gain)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSSpeakerPosition(AXDRCOutput output,
                          AXDRCVSSpeakerPosition pos)
{
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSSurroundDepth(AXDRCOutput output,
                        float depth)
{
   return AXResult::Success;
}

AXResult::Result
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
