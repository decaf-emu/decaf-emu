#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

// BindFlagsToString__20_GX2RResourceTrackerSF18_GX2RResourceFlags
// UsageFlagsToString__20_GX2RResourceTrackerSF18_GX2RResourceFlags
namespace GX2RResourceFlags
{
enum Value : uint32_t
{
   // GX2R_BIND_*
   BindTexture       = 1 << 0,
   BindColorBuffer   = 1 << 1,
   BindDepthBuffer   = 1 << 2,
   BindScanBuffer    = 1 << 3,
   BindVertexBuffer  = 1 << 4,
   BindIndexBuffer   = 1 << 5,
   BindUniformBlock  = 1 << 6,
   BindShaderProgram = 1 << 7,
   BindStreamOutput  = 1 << 8,
   BindDisplayList   = 1 << 9,
   BindGSRing        = 1 << 10,

   // GX2R_USAGE_*
   UsageCpuRead      = 1 << 11,
   UsageCpuWrite     = 1 << 12,
   UsageGpuRead      = 1 << 13,
   UsageGpuWrite     = 1 << 14,
   UsageDmaRead      = 1 << 15,
   UsageDmaWrite     = 1 << 16,
   UsageForceMEM1    = 1 << 17,
   UsageForceMEM2    = 1 << 18,
};
}

#pragma pack(push, 1)

struct GX2RBuffer
{
   be_val<GX2RResourceFlags::Value> flags;
   be_val<uint32_t> elemSize;
   be_val<uint32_t> elemCount;
   be_ptr<void> buffer;
};
CHECK_SIZE(GX2RBuffer, 0x10);
CHECK_OFFSET(GX2RBuffer, 0x00, flags);
CHECK_OFFSET(GX2RBuffer, 0x04, elemSize);
CHECK_OFFSET(GX2RBuffer, 0x08, elemCount);
CHECK_OFFSET(GX2RBuffer, 0x0C, buffer);

#pragma pack(pop)

using GX2RAllocFuncPtr = wfunc_ptr<void *, GX2RResourceFlags::Value, uint32_t, uint32_t>;
using GX2RFreeFuncPtr = wfunc_ptr<void, GX2RResourceFlags::Value, void *>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn, GX2RFreeFuncPtr freeFn);

uint32_t
GX2RGetBufferAlignment(GX2RResourceFlags::Value flags);

uint32_t
GX2RGetBufferAllocationSize(GX2RBuffer *buffer);

BOOL
GX2RCreateBuffer(GX2RBuffer *buffer);

void
GX2RDestroyBufferEx(GX2RBuffer *buffer, GX2RResourceFlags::Value flags);

void *
GX2RLockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags::Value flags);

void
GX2RUnlockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags::Value flags);
