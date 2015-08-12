#pragma once
#include "systemtypes.h"

// BindFlagsToString__20_GX2RResourceTrackerSF18_GX2RResourceFlags
// UsageFlagsToString__20_GX2RResourceTrackerSF18_GX2RResourceFlags
namespace GX2RResourceFlags
{
enum Flags
{
   // GX2R_BIND_*
   BindTexture = 1 << 0,
   BindColorBuffer = 1 << 1,
   BindDepthBuffer = 1 << 2,
   BindScanBuffer = 1 << 3,
   BindVertexBuffer = 1 << 4,
   BindIndexBuffer = 1 << 5,
   BindUniformBlock = 1 << 6,
   BindShaderProgram = 1 << 7,
   BindStreamOutput = 1 << 8,
   BindDisplayList = 1 << 9,
   BindGSRing = 1 << 10,

   // GX2R_USAGE_*
   UsageCpuRead = 1 << 11,
   UsageCpuWrite = 1 << 12,
   UsageGpuRead = 1 << 13,
   UsageGpuWrite = 1 << 14,
   UsageDmaRead = 1 << 15,
   UsageDmaWrite = 1 << 16,
   UsageForceMEM1 = 1 << 17,
   UsageForceMEM2 = 1 << 18,
};
}

using GX2RAllocFuncPtr = wfunc_ptr<p32<void>, GX2RResourceFlags::Flags, uint32_t, uint32_t>;
using GX2RFreeFuncPtr = wfunc_ptr<void, GX2RResourceFlags::Flags, p32<void>>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn, GX2RFreeFuncPtr freeFn);
