#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "gx2_enum.h"

namespace gx2
{

struct GX2QueryData
{
   UNKNOWN(0x40);
};
CHECK_SIZE(GX2QueryData, 0x40);

void
GX2SampleTopGPUCycle(be_val<uint64_t> *result);

void
GX2SampleBottomGPUCycle(be_val<uint64_t> *result);

uint64_t
GX2GPUTimeToCPUTime(uint64_t time);

uint32_t
GX2GetGPUTimeout();

void
GX2SetGPUTimeout(uint32_t timeout);

void
GX2QueryBegin(GX2QueryType type,
              GX2QueryData *data);

void
GX2QueryEnd(GX2QueryType type,
            GX2QueryData *data);

void
GX2QueryGetOcclusionResult(GX2QueryData *data,
                           be_val<uint64_t> *result);

} // namespace gx2
