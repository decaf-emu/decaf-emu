#pragma once
#include <common/cbool.h>
#include <common/structsize.h>
#include <cstdint>

namespace gx2
{

#pragma pack(push, 1)

struct GX2CounterInfo
{
   UNKNOWN(0x4A0);
};
CHECK_SIZE(GX2CounterInfo, 0x4A0);

#pragma pack(pop)

BOOL
GX2InitCounterInfo(GX2CounterInfo *info,
                   uint32_t unk0,
                   uint32_t unk1);

void
GX2ResetCounterInfo(GX2CounterInfo *info);

uint64_t
GX2GetCounterResult(GX2CounterInfo *info,
                    uint32_t unk0);

} // namespace gx2
