#include "gx2.h"
#include "gx2_draw.h"

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t unk1,
                  GX2ClearFlags::Flags flags)
{
}

void GX2::registerDrawFunctions()
{
   RegisterKernelFunction(GX2ClearBuffersEx);
}
