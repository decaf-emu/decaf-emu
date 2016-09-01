#include "snd_core.h"
#include "snd_core_vs.h"

namespace snd_core
{

AXResult
AXGetDRCVSMode(be_val<AXDRCVSMode> *mode)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSMode(AXDRCVSMode mode)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSDownmixBalance(AXDRCOutput output,
                         float balance)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSLC(AXDRCVSLC lc)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSLimiter(BOOL limit)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSLimiterThreshold(float threshold)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSOutputGain(AXDRCOutput output,
                     float gain)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSSpeakerPosition(AXDRCOutput output,
                          AXDRCVSSpeakerPosition pos)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSSurroundDepth(AXDRCOutput output,
                        float depth)
{
   decaf_warn_stub();

   return AXResult::Success;
}

AXResult
AXSetDRCVSSurroundLevelGain(AXDRCVSSurroundLevelGain gain)
{
   decaf_warn_stub();

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
