#include "gx2_draw.h"

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red,
              float green,
              float blue,
              float alpha)
{
}

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags::Value unk2)
{
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags::Value unk2)
{
}

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer)
{
}

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
                        float depth,
                        uint8_t stencil)
{
}

void
GX2SetPrimitiveRestartIndex(uint32_t index)
{
}

void
GX2DrawEx(GX2PrimitiveMode::Value mode,
          uint32_t numVertices,
          uint32_t offset,
          uint32_t numInstances)
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
