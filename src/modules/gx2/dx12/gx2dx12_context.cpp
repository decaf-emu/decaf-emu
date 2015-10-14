#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_context.h"
#include "dx12_state.h"

void
GX2Init(be_val<uint32_t> *attributes)
{
   // TODO: GX2Init set current thread as the graphics thread
   auto log = gLog->debug();
   log << "GX2Init attributes: ";

   while (attributes && *attributes) {
      uint32_t attrib = *attributes;
      log << " " << attrib;
      ++attributes;
   }

   dx::initialise();
}

void
GX2Shutdown()
{
   // TODO: GX2Shutdown
}

void
GX2Flush()
{
   // TODO: GX2Flush
}

void
GX2Invalidate(GX2InvalidateMode::Mode mode, void *buffer, uint32_t size)
{
   // TODO: GX2Invalidate
}

void
GX2SetupContextState(GX2ContextState *state)
{
   GX2SetupContextStateEx(state, TRUE);
}

void
GX2SetupContextStateEx(GX2ContextState *state, BOOL unk1)
{
   state->displayListSize = 0x300;
   GX2BeginDisplayListEx(reinterpret_cast<GX2DisplayList*>(&state->displayList), state->displayListSize, unk1);

   memcpy(state->stateStore, &gDX.state, sizeof(gDX.state));
   gDX.activeContextState = state;
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
   // Sync the previous state storage
   if (gDX.activeContextState) {
      memcpy(gDX.activeContextState->stateStore, &gDX.state, sizeof(gDX.state));
   }

   // Restore the newly assigned state storage
   if (state) {
      memcpy(&gDX.state, state->stateStore, sizeof(gDX.state));
   }

   // Save the assigned state storage
   gDX.activeContextState = state;
}

#endif