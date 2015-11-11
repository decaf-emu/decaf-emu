#pragma once
#include "modules/gx2/gx2_displaylist.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "types.h"

#pragma pack(push, 1)

struct GX2ContextState
{
   uint8_t stateStore[0x9804];
   uint32_t displayListSize;
   UNKNOWN(0x9e00 - 0x9808);
   uint8_t displayList[0x300];
};
CHECK_OFFSET(GX2ContextState, 0x9804, displayListSize);
CHECK_OFFSET(GX2ContextState, 0x9e00, displayList);
CHECK_SIZE(GX2ContextState, 0xa100);

#pragma pack(pop)

void
GX2SetupContextState(GX2ContextState *state);

void
GX2SetupContextStateEx(GX2ContextState *state, BOOL unk1);

void
GX2GetContextStateDisplayList(GX2ContextState *state, be_ptr<uint8_t> *outDisplayList, be_val<uint32_t> *outSize);

void
GX2SetContextState(GX2ContextState *state);
