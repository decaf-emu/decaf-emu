#include "gx2.h"
#include "gx2_core.h"

// Initialise graphics system and set current thread as the graphics thread
void
GX2Init(uint32_t *attributes)
{
   auto log = xDebug();
   log << "GX2Init attributes: ";

   while (attributes && *attributes) {
      auto attrib = *attributes;
      log << " " << attrib;
      ++attributes;
   }
}

void
GX2Shutdown()
{

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
}

void
GX2Invalidate(InvalidateMode mode, p32<void> buffer, uint32_t size)
{
}

void
GX2GetContextStateDisplayList(const GX2ContextState* state, p32<be_val<uint32_t>> outDisplayList, p32<be_val<uint32_t>> outSize)
{
   *outDisplayList = make_p32(&state->displayList).value;
   *outSize = state->displayListSize;
}

void GX2::registerCoreFunctions()
{
   RegisterSystemFunction(GX2Init);
   RegisterSystemFunction(GX2Shutdown);
   RegisterSystemFunction(GX2SetupContextState);
   RegisterSystemFunction(GX2SetupContextStateEx);
   RegisterSystemFunction(GX2Invalidate);
}