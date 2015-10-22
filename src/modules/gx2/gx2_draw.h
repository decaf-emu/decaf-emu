#pragma once
#include "types.h"

namespace GX2ClearFlags
{
enum Flags : uint32_t
{
   Depth = 0x1,
};
}

namespace GX2PrimitiveMode
{
enum Mode : uint32_t
{
   First = 0x1,
   Triangles = 0x4,
   TriangleStrip = 0x6,
   Quads = 0x13,
   QuadStrip = 0x14,
   Last = 0x94,
};
}

namespace GX2IndexType
{
enum Type : uint32_t
{
   First = 0x0,
   U16 = 0x4,
   Last = 0x9,
};
}

struct GX2ColorBuffer;
struct GX2DepthBuffer;

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer, float depth, uint8_t stencil);

void
GX2ClearColor(GX2ColorBuffer *colorBuffer, 
              float red, float green, float blue,  float alpha);

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags::Flags unk2);

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags::Flags unk2);


void
GX2SetAttribBuffer(uint32_t index, uint32_t size, uint32_t stride, void *buffer);

void
GX2DrawEx(GX2PrimitiveMode::Mode mode, uint32_t numVertices, uint32_t offset, uint32_t numInstances);

void
GX2DrawIndexedEx(GX2PrimitiveMode::Mode mode, 
   uint32_t numVertices, 
   GX2IndexType::Type indexType, 
   void *indices, 
   uint32_t offset, 
   uint32_t numInstances);
