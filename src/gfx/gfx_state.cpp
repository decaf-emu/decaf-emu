#include "../systemtypes.h"
#include "hostlookup.h"
#include "gfx_state.h"

GX2State gGX2State;

struct TextureData {
   void *lol;
};

HostLookupTable<TextureData, GX2ColorBuffer> colorBufferLkp;

GLuint getColorBuffer(GX2ColorBuffer *buffer)
{
   auto x = colorBufferLkp.get(buffer);

   // TODO: gfx::getColorBuffer
   return 0;
}