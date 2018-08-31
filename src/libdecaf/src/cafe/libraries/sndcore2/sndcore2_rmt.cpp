#include "sndcore2.h"
#include "sndcore2_rmt.h"
#include <cafe/libraries/cafe_hle_stub.h>

namespace cafe::sndcore2
{

int32_t
AXRmtGetSamplesLeft()
{
   decaf_warn_stub();
   return 0;
}

int32_t
AXRmtGetSamples(int32_t,
                virt_ptr<uint8_t> buffer,
                int32_t samples)
{
   decaf_warn_stub();
   return 0;
}

int32_t
AXRmtAdvancePtr(int32_t numSamples)
{
   decaf_warn_stub();
   return 0;
}

void
Library::registerRmtSymbols()
{
   RegisterFunctionExport(AXRmtGetSamplesLeft);
   RegisterFunctionExport(AXRmtGetSamples);
   RegisterFunctionExport(AXRmtAdvancePtr);
}

} // namespace cafe::sndcore2
