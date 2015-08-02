#pragma once
#include <cstdint>

namespace GX2ClearFlags
{
enum Flags : uint32_t
{
   // Probably flags of what buffers to clear!!
};
}

struct GX2ColorBuffer;
struct GX2DepthBuffer;

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t unk1,
                  GX2ClearFlags::Flags unk2);
