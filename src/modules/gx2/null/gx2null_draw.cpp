#include "../gx2.h"
#ifdef GX2_NULL

#include "../gx2_draw.h"

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
   float depth,
   uint8_t stencil)
{
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
   GX2DepthBuffer *depthBuffer,
   float red, float green, float blue, float alpha,
   float depth,
   uint8_t unk1,
   GX2ClearFlags::Flags flags)
{
}

void
GX2SetAttribBuffer(uint32_t unk1,
   uint32_t unk2,
   uint32_t unk3,
   void *buffer)
{
}

void
GX2DrawEx(GX2PrimitiveMode::Mode mode,
   uint32_t unk1,
   uint32_t unk2,
   uint32_t unk3)
{
}

#endif
