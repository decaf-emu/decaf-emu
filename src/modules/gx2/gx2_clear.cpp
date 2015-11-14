#include "gx2_clear.h"
#include "gx2_surface.h"
#include "gpu/pm4_writer.h"
#include "utils/bit_cast.h"

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red,
              float green,
              float blue,
              float alpha)
{
   // TODO: GX2ClearColor
}

void
GX2ClearDepthStencil(GX2DepthBuffer *depthBuffer,
                     GX2ClearFlags::Value clearFlags)
{
   // TODO: GX2ClearDepthStencil
}

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags::Value clearFlags)
{
   depthBuffer->depthClear = depth;
   depthBuffer->stencilClear = stencil;
   GX2ClearDepthStencil(depthBuffer, clearFlags);
}

void
GX2ClearBuffers(GX2ColorBuffer *colorBuffer,
                GX2DepthBuffer *depthBuffer,
                float red, float green, float blue, float alpha,
                GX2ClearFlags::Value clearFlags)
{
   GX2ClearColor(colorBuffer, red, green, blue, alpha);
   GX2ClearDepthStencil(depthBuffer, clearFlags);
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags::Value clearFlags)
{
   GX2ClearColor(colorBuffer, red, green, blue, alpha);
   GX2ClearDepthStencilEx(depthBuffer, depth, stencil, clearFlags);
}

void
GX2SetClearDepth(GX2DepthBuffer *depthBuffer,
                 float depth)
{
   depthBuffer->depthClear = depth;
   pm4::write(pm4::SetContextReg { latte::Register::DB_DEPTH_CLEAR, bit_cast<uint32_t>(depth) });
}

void
GX2SetClearStencil(GX2DepthBuffer *depthBuffer,
                   uint8_t stencil)
{
   depthBuffer->stencilClear = stencil;
   pm4::write(pm4::SetContextReg { latte::Register::DB_STENCIL_CLEAR, stencil });
}


void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
                        float depth,
                        uint8_t stencil)
{
   uint32_t values[] = { stencil, bit_cast<uint32_t>(depth) };
   depthBuffer->depthClear = depth;
   depthBuffer->stencilClear = stencil;
   pm4::write(pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, { values } });
}
