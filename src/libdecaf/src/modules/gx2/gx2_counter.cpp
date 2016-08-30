#include "gx2_counter.h"
#include <cstring>

namespace gx2
{

BOOL
GX2InitCounterInfo(GX2CounterInfo *info,
                   uint32_t unk0,
                   uint32_t unk1)
{
   return TRUE;
}

void
GX2ResetCounterInfo(GX2CounterInfo *info)
{
   std::memset(info, 0, sizeof(GX2CounterInfo));
}

uint64_t
GX2GetCounterResult(GX2CounterInfo *info,
                    uint32_t unk0)
{
   return 0;
}

} // namespace gx2
