#include "sndcore2.h"
#include "sndcore2_vs.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::sndcore2
{

AXResult
AXGetDRCVSMode(virt_ptr<AXDRCVSMode> outMode)
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
Library::registerVsSymbols()
{
   RegisterFunctionExport(AXGetDRCVSMode);
   RegisterFunctionExport(AXSetDRCVSMode);
   RegisterFunctionExport(AXSetDRCVSDownmixBalance);
   RegisterFunctionExport(AXSetDRCVSLC);
   RegisterFunctionExport(AXSetDRCVSLimiter);
   RegisterFunctionExport(AXSetDRCVSLimiterThreshold);
   RegisterFunctionExport(AXSetDRCVSOutputGain);
   RegisterFunctionExport(AXSetDRCVSSpeakerPosition);
   RegisterFunctionExport(AXSetDRCVSSurroundDepth);
   RegisterFunctionExport(AXSetDRCVSSurroundLevelGain);
}

} // namespace cafe::sndcore2
