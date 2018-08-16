#include "gx2.h"
#include "gx2_counter.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include <cstring>

namespace cafe::gx2
{

BOOL
GX2InitCounterInfo(virt_ptr<GX2CounterInfo> info,
                   uint32_t unk0,
                   uint32_t unk1)
{
   decaf_warn_stub();
   return TRUE;
}

void
GX2ResetCounterInfo(virt_ptr<GX2CounterInfo> info)
{
   decaf_warn_stub();
   std::memset(info.getRawPointer(), 0, sizeof(GX2CounterInfo));
}

uint64_t
GX2GetCounterResult(virt_ptr<GX2CounterInfo> info,
                    uint32_t unk0)
{
   decaf_warn_stub();
   return 0;
}

void
Library::registerCounterSymbols()
{
   RegisterFunctionExport(GX2InitCounterInfo);
   RegisterFunctionExport(GX2ResetCounterInfo);
   RegisterFunctionExport(GX2GetCounterResult);
}

} // namespace cafe::gx2
