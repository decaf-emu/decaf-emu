#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_counter Counter
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2CounterInfo
{
   UNKNOWN(0x4A0);
};
CHECK_SIZE(GX2CounterInfo, 0x4A0);

#pragma pack(pop)

BOOL
GX2InitCounterInfo(virt_ptr<GX2CounterInfo> info,
                   uint32_t unk0,
                   uint32_t unk1);

void
GX2ResetCounterInfo(virt_ptr<GX2CounterInfo> info);

uint64_t
GX2GetCounterResult(virt_ptr<GX2CounterInfo> info,
                    uint32_t unk0);

/** @} */

} // namespace cafe::gx2
