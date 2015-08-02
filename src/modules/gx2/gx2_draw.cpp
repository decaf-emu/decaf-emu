#include "gx2.h"
#include "gx2_draw.h"

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
                        float depth,
                        uint8_t stencil)
{
   // TODO: GX2SetClearDepthStencil
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t unk1,
                  GX2ClearFlags::Flags flags)
{
   // TODO: GX2ClearBuffersEx
}

void
GX2SetAttribBuffer(uint32_t unk1,
                   uint32_t unk2,
                   uint32_t unk3,
                   void *buffer)
{
   // TODO: GX2SetAttribBuffer
}

void
GX2DrawEx(GX2PrimitiveMode::Mode mode,
          uint32_t unk1,
          uint32_t unk2,
          uint32_t unk3)
{
   // TODO: GX2DrawEx
}

void GX2::registerDrawFunctions()
{
   RegisterKernelFunction(GX2ClearBuffersEx);
   RegisterKernelFunction(GX2SetClearDepthStencil);
   RegisterKernelFunction(GX2SetAttribBuffer);
   RegisterKernelFunction(GX2DrawEx);
}
