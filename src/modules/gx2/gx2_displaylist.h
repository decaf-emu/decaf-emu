#pragma once
#include "be_val.h"
#include "types.h"

struct GX2DisplayList;

#define GX2_DISPLAY_LIST_ALIGNMENT 0x20

void
GX2BeginDisplayListEx(GX2DisplayList *displayList, uint32_t size, BOOL unk1);

void
GX2BeginDisplayList(GX2DisplayList *displayList, uint32_t size);

uint32_t
GX2EndDisplayList(GX2DisplayList *displayList);

void
GX2DirectCallDisplayList(GX2DisplayList *displayList, uint32_t size);

void
GX2CallDisplayList(GX2DisplayList *displayList, uint32_t size);

BOOL
GX2GetDisplayListWriteStatus();

BOOL
GX2GetCurrentDisplayList(be_val<uint32_t> *outDisplayList, be_val<uint32_t> *outSize);

void
GX2CopyDisplayList(GX2DisplayList *displayList, uint32_t size);
