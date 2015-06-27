#pragma once
#include "systemtypes.h"
#include "gx2_displaylist.h"

#pragma pack(push, 1)

struct GX2ContextState
{
   UNKNOWN(0x9804);
   uint32_t displayListSize;
   UNKNOWN(0x9e00 - 0x9808);
   uint8_t displayList[0x300];
};
CHECK_OFFSET(GX2ContextState, 0x9804, displayListSize);
CHECK_OFFSET(GX2ContextState, 0x9e00, displayList);
CHECK_SIZE(GX2ContextState, 0xa100);

#pragma pack(pop)

enum class InvalidateMode : uint32_t
{
   First = 1,
   Last = 0x1ff
};

void
GX2Init(be_val<uint32_t> *attributes);

void
GX2Shutdown();

void
GX2SetupContextState(GX2ContextState* state);

void
GX2SetupContextStateEx(GX2ContextState* state, BOOL unk1);

void
GX2Invalidate(InvalidateMode mode, p32<void> buffer, uint32_t size);

void
GX2GetContextStateDisplayList(const GX2ContextState* state, be_val<uint32_t> *outDisplayList, be_val<uint32_t> *outSize);
