#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/gx2/gx2_draw.h"

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
                  GX2ClearFlags::Value flags)
{
}

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red, float green, float blue, float alpha)
{
}

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth,
                       uint8_t stencil,
                       GX2ClearFlags::Value unk2)
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
GX2DrawEx(GX2PrimitiveMode::Value mode,
          uint32_t unk1,
          uint32_t unk2,
          uint32_t unk3)
{
}

void
GX2DrawIndexedEx(GX2PrimitiveMode::Value mode,
   uint32_t numVertices,
   GX2IndexType::Value indexType,
   void *indices,
   uint32_t offset,
   uint32_t numInstances)
{
}

#endif
