#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "virtual_ptr.h"

namespace gx2
{

#define GX2_DISPLAY_LIST_ALIGNMENT 0x20

void
GX2BeginDisplayList(void *displayList,
                    uint32_t bytes);

void
GX2BeginDisplayListEx(void *displayList,
                      uint32_t bytes,
                      BOOL unk1);

uint32_t
GX2EndDisplayList(void *displayList);

void
GX2DirectCallDisplayList(void *displayList,
                         uint32_t bytes);

void
GX2CallDisplayList(void *displayList,
                   uint32_t bytes);

BOOL
GX2GetDisplayListWriteStatus();

BOOL
GX2GetCurrentDisplayList(be_ptr<void> *outDisplayList,
                         be_val<uint32_t> *outSize);

void
GX2CopyDisplayList(void *displayList,
                   uint32_t bytes);

} // namespace gx2
