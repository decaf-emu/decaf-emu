#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/gx2/gx2_context.h"
#include "modules/gx2/gx2_vsync.h"

void
GX2Init(be_val<uint32_t> *attributes)
{
   _GX2InitVsync();
}

void
GX2Shutdown()
{
}

void
GX2Flush()
{
}

void
GX2Invalidate(GX2InvalidateMode::Value mode,
              void *buffer,
              uint32_t size)
{
}

void
GX2SetupContextState(GX2ContextState *state)
{
   GX2SetupContextStateEx(state, TRUE);
}

void
GX2SetupContextStateEx(GX2ContextState *state,
                       BOOL unk1)
{
   state->displayListSize = 0x300;
   GX2BeginDisplayListEx(reinterpret_cast<GX2DisplayList*>(&state->displayList), state->displayListSize, unk1);
}

void
GX2GetContextStateDisplayList(GX2ContextState *state,
                              be_ptr<uint8_t> *outDisplayList,
                              be_val<uint32_t> *outSize)
{
   *outDisplayList = reinterpret_cast<uint8_t*>(&state->displayList);
   *outSize = state->displayListSize;
}

void
GX2SetContextState(GX2ContextState *state)
{
}

#endif
