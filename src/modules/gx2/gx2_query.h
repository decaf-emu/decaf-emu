#pragma once
#include "types.h"
#include "utils/be_val.h"

namespace gx2
{

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

} // namespace gx2
