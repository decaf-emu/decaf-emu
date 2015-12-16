#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

struct GX2RBuffer
{
   be_val<GX2RResourceFlags> flags;
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

uint32_t
GX2RGetBufferAlignment(GX2RResourceFlags flags);

uint32_t
GX2RGetBufferAllocationSize(GX2RBuffer *buffer);

BOOL
GX2RCreateBuffer(GX2RBuffer *buffer);

void
GX2RDestroyBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags);

void *
GX2RLockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags);

void
GX2RUnlockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags);
