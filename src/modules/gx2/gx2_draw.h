#pragma once
#include <cstdint>

namespace GX2ClearFlags
{
enum Flags : uint32_t
{
   // Probably flags of what buffers to clear!!
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
GX2SetAttribBuffer(uint32_t unk1, uint32_t unk2, uint32_t unk3, void *buffer);

void
GX2DrawEx(GX2PrimitiveMode::Mode mode, uint32_t unk1, uint32_t unk2, uint32_t unk3);