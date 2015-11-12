#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"

#define GX2_DISPLAY_LIST_ALIGNMENT 0x20

void
GX2BeginDisplayList(void *displayList, uint32_t size);

void
GX2BeginDisplayListEx(void *displayList, uint32_t size, BOOL unk1);

uint32_t
GX2EndDisplayList(void *displayList);

void
GX2DirectCallDisplayList(void *displayList, uint32_t size);

void
GX2CallDisplayList(void *displayList, uint32_t size);

BOOL
GX2GetDisplayListWriteStatus();

BOOL
GX2GetCurrentDisplayList(be_ptr<void> *outDisplayList, be_val<uint32_t> *outSize);

void
GX2CopyDisplayList(void *displayList, uint32_t size);
