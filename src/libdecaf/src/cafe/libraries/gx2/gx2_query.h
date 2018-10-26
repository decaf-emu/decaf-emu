#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

struct GX2PerfData
{
   UNKNOWN(0x8a0);
};
CHECK_SIZE(GX2PerfData, 0x8a0);

struct GX2QueryData
{
   // Note that these are intentionally host-endian as they
   // are GPU managed which is litte-endian, not big-endian (PPC).
   uint64_t _gpudata[8];
};
CHECK_SIZE(GX2QueryData, 0x40);

void
GX2SampleTopGPUCycle(virt_ptr<int64_t> writeSamplePtr);

void
GX2SampleBottomGPUCycle(virt_ptr<int64_t> writeSamplePtr);

uint64_t
GX2GPUTimeToCPUTime(uint64_t time);

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

bool
GX2PerfMetricEnable(virt_ptr<GX2PerfData> data,
                    GX2PerfType type,
                    uint32_t 	id);

} // namespace cafe::gx2
