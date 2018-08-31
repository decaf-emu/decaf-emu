#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

struct GX2QueryData
{
   UNKNOWN(0x40);
};
CHECK_SIZE(GX2QueryData, 0x40);

void
GX2SampleTopGPUCycle(virt_ptr<int64_t> writeSamplePtr);

void
GX2SampleBottomGPUCycle(virt_ptr<int64_t> writeSamplePtr);

uint64_t
GX2GPUTimeToCPUTime(uint64_t time);

uint32_t
GX2GetGPUTimeout();

void
GX2SetGPUTimeout(uint32_t timeout);

void
GX2QueryBegin(GX2QueryType type,
              virt_ptr<GX2QueryData> data);

void
GX2QueryEnd(GX2QueryType type,
            virt_ptr<GX2QueryData> data);

void
GX2QueryGetOcclusionResult(virt_ptr<GX2QueryData> data,
                           virt_ptr<uint64_t> outResult);

void
GX2QueryBeginConditionalRender(GX2QueryType type,
                               virt_ptr<GX2QueryData> data,
                               BOOL hint,
                               BOOL predicate);
void
GX2QueryEndConditionalRender();

} // namespace cafe::gx2
