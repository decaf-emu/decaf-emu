#include "gx2_clear.h"
#include "gx2_surface.h"
#include "gpu/pm4_writer.h"
#include "common/bit_cast.h"

namespace gx2
{

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red,
              float green,
              float blue,
              float alpha)
{
   auto cb_color_frag = latte::CB_COLOR0_FRAG::get(0);
   auto cb_color_base = latte::CB_COLOR0_BASE::get(0)
      .BASE_256B(colorBuffer->surface.image.getAddress() >> 8);

   if (colorBuffer->surface.aa != 0) {
      cb_color_frag = cb_color_frag.BASE_256B(colorBuffer->aaBuffer.getAddress() >> 8);
   }

   GX2InitColorBufferRegs(colorBuffer);

   pm4::write(pm4::DecafClearColor {
      red, green, blue, alpha,
      cb_color_base,
      cb_color_frag,
      colorBuffer->regs.cb_color_size,
      colorBuffer->regs.cb_color_info,
      colorBuffer->regs.cb_color_view,
      colorBuffer->regs.cb_color_mask
   });
}

void
DecafClearDepthStencil(GX2DepthBuffer *depthBuffer,
                       GX2ClearFlags clearFlags)
{
   auto db_depth_base = latte::DB_DEPTH_BASE::get(0)
      .BASE_256B(depthBuffer->surface.image.getAddress() >> 8);

   auto db_depth_htile_data_base = latte::DB_DEPTH_HTILE_DATA_BASE::get(0)
      .BASE_256B(depthBuffer->hiZPtr.getAddress() >> 8);

   GX2InitDepthBufferRegs(depthBuffer);

   pm4::write(pm4::DecafClearDepthStencil {
      clearFlags,
      db_depth_base,
      db_depth_htile_data_base,
      depthBuffer->regs.db_depth_info,
      depthBuffer->regs.db_depth_size,
      depthBuffer->regs.db_depth_view,
   });
}

void
GX2ClearDepthStencil(GX2DepthBuffer *depthBuffer,
                     GX2ClearFlags clearFlags)
{
   uint32_t values[] = {
      depthBuffer->stencilClear,
      bit_cast<uint32_t>(depthBuffer->depthClear)
   };

   pm4::write(pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, gsl::as_span(values) });
   DecafClearDepthStencil(depthBuffer, clearFlags);
}

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags clearFlags)
{
   uint32_t values[] = {
      stencil,
      bit_cast<uint32_t>(depth)
   };

   pm4::write(pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, gsl::as_span(values) });
   DecafClearDepthStencil(depthBuffer, clearFlags);
}

void
GX2ClearBuffers(GX2ColorBuffer *colorBuffer,
                GX2DepthBuffer *depthBuffer,
                float red, float green, float blue, float alpha,
                GX2ClearFlags clearFlags)
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
                  GX2ClearFlags clearFlags)
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
   depthBuffer->depthClear = depth;
   depthBuffer->stencilClear = stencil;

   uint32_t values[] = {
      stencil,
      bit_cast<uint32_t>(depth)
   };

   pm4::write(pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, gsl::as_span(values) });
}

} // namespace gx2
