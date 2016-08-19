#pragma once
#include "gpu/latte_contextstate.h"
#include "modules/gx2/gx2_displaylist.h"
#include "modules/gx2/gx2_enum.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"
#include "common/types.h"

namespace gx2
{

#pragma pack(push, 1)

// Internal display list is used to create LOAD_ dlist for the shadow state
struct GX2ContextState
{
   static const auto MaxDisplayListSize = 192u;
   latte::ContextState shadowState;
   UNKNOWN(4);
   uint32_t shadowDisplayListSize;
   UNKNOWN(0x9e00 - 0x9808);
   uint32_t shadowDisplayList[MaxDisplayListSize];
};
CHECK_OFFSET(GX2ContextState, 0x0000, shadowState);
CHECK_OFFSET(GX2ContextState, 0x9804, shadowDisplayListSize);
CHECK_OFFSET(GX2ContextState, 0x9e00, shadowDisplayList);
CHECK_SIZE(GX2ContextState, 0xa100);

#pragma pack(pop)

void
GX2SetupContextState(GX2ContextState *state);

void
GX2SetupContextStateEx(GX2ContextState *state, BOOL unk1);

void
GX2GetContextStateDisplayList(GX2ContextState *state, be_ptr<void> *outDisplayList, be_val<uint32_t> *outSize);

void
GX2SetContextState(GX2ContextState *state);

void
GX2SetDefaultState();

namespace internal
{

void
initRegisters();

} // namespace internal

} // namespace gx2
