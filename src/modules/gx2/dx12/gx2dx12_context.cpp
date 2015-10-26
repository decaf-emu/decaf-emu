#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_context.h"
#include "modules/gx2/gx2_vsync.h"
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
   _GX2InitVsync();
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
   // We don't actually use the internal display list...
   //GX2BeginDisplayListEx(reinterpret_cast<GX2DisplayList*>(&state->displayList), state->displayListSize, unk1);

   memcpy(state->stateStore, &gDX.state, sizeof(gDX.state));
   gDX.activeContextState = state;
}

void
GX2GetContextStateDisplayList(GX2ContextState *state,
                              be_ptr<uint8_t> *outDisplayList,
                              be_val<uint32_t> *outSize)
{
   // This is definitely not correct as this method is used by games
   //   to directly invoke the internal display list which configures
   //   the state in a particular context state.  For now we return null
   //   which means that the state is not properly applied when they call
   //   DirectCallDisplayList.
   gLog->warn("Badly implemented GX2GetContextStateDisplayList called.");
   *outDisplayList = nullptr;
   *outSize = 0;
}

void
_GX2SetContextState(GX2ContextState *state)
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

void
GX2SetContextState(GX2ContextState *state)
{
   // This might not be right, it's possible that we need 
   //   to transfer the whole GX2ContextState into the display 
   //   list for later rather than just referring to it...
   DX_DLCALL(_GX2SetContextState, state);
}

#endif