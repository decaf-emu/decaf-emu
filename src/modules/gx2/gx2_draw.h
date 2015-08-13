#pragma once
#include <cstdint>

namespace GX2ClearFlags
{
enum Flags : uint32_t
{
   Depth = 1,
};
}

namespace GX2PrimitiveMode
{
enum Mode : uint32_t
{
   First = 1,
   Last = 0x94
};
}

struct GX2ColorBuffer;
struct GX2DepthBuffer;

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer, float depth, uint8_t stencil);

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