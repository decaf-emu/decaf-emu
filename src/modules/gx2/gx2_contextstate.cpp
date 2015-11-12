#include "gpu/pm4_writer.h"
#include "gx2.h"
#include "gx2_contextstate.h"
#include <utility>

static GX2ContextState *
gActiveContext = nullptr;

static std::pair<uint32_t, uint32_t> ConfigRegisterRange[] = {
   { 0x300, 6 },
   { 0x900, 0x48 },
   { 0x980, 0x48 },
   { 0xA00, 0x48 },
   { 0x310, 0xC },
   { 0x542, 1 },
   { 0x235, 1 },
   { 0x232, 2 },
   { 0x23A, 1 },
   { 0x256, 1 },
   { 0x60C, 1 },
   { 0x5C5, 1 },
   { 0x2C8, 1 },
   { 0x363, 1 },
   { 0x404, 2 },
};

static std::pair<uint32_t, uint32_t> ContextRegisterRange[] = {
   { 0, 2 },
   { 3, 3 },
   { 0xA, 4 },
   { 0x10, 0x38 },
   { 0x50, 0x34 },
   { 0x8E, 4 },
   { 0x94, 0x40 },
   { 0x100, 9 },
   { 0x10C, 3 },
   { 0x10F, 0x60 },
   { 0x185, 0xA },
   { 0x191, 0x27 },
   { 0x1E0, 9 },
   { 0x200, 1 },
   { 0x202, 7 },
   { 0xE0, 0x20 },
   { 0x210, 0x29 },
   { 0x250, 0x34 },
   { 0x290, 1 },
   { 0x292, 2 },
   { 0x2A1, 1 },
   { 0x2A5, 1 },
   { 0x2A8, 2 },
   { 0x2AC, 3 },
   { 0x2CA, 1 },
   { 0x2CC, 1 },
   { 0x2CE, 1 },
   { 0x300, 9 },
   { 0x30C, 1 },
   { 0x312, 1 },
   { 0x316, 2 },
   { 0x343, 2 },
   { 0x349, 3 },
   { 0x34C, 2 },
   { 0x351, 1 },
   { 0x37E, 6 },
   { 0x2B4, 3 },
   { 0x2B8, 3 },
   { 0x2BC, 3 },
   { 0x2C0, 3 },
   { 0x2C8, 1 },
   { 0x29B, 1 },
   { 0x8C, 1 },
   { 0xD5, 1 },
   { 0x284, 0xC },
};

void
GX2SetupContextState(GX2ContextState *state)
{
   GX2SetupContextStateEx(state, TRUE);
}

void
GX2LoadState(GX2ContextState *state)
{
   pm4::write(pm4::LoadConfigReg { state->shadowState.config, { ConfigRegisterRange } });
   pm4::write(pm4::LoadContextReg { state->shadowState.context, { ContextRegisterRange } });
   // TODO: Rest of shadowState
}

void
GX2SetupContextStateEx(GX2ContextState *state, BOOL unk1)
{
   GX2BeginDisplayList(state->shadowDisplayList, state->shadowDisplayListSize);
   GX2LoadState(state);
   GX2EndDisplayList(state->shadowDisplayList);
}

void
GX2GetContextStateDisplayList(GX2ContextState *state, be_ptr<void> *outDisplayList, be_val<uint32_t> *outSize)
{
   if (outDisplayList) {
      *outDisplayList = state->shadowDisplayList;
   }

   if (outSize) {
      *outSize = state->shadowDisplayListSize;
   }
}

void
GX2SetContextState(GX2ContextState *state)
{
   gActiveContext = state;

   if (GX2GetDisplayListWriteStatus()) {
      GX2CopyDisplayList(state->shadowDisplayList, state->shadowDisplayListSize);
   } else {
      GX2CallDisplayList(state->shadowDisplayList, state->shadowDisplayListSize);
   }
}
