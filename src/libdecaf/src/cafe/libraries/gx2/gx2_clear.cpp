#include "gx2.h"
#include "gx2_clear.h"
#include "gx2_cbpool.h"
#include "gx2_debugcapture.h"
#include "gx2_surface.h"

#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <common/bit_cast.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;

void
GX2ClearColor(virt_ptr<GX2ColorBuffer> colorBuffer,
              float red,
              float green,
              float blue,
              float alpha)
{
   internal::debugCaptureTagGroup(GX2DebugTag::ClearColor,
                                  "{}, {:.2f}, {:.2f}, {:.2f}, {:.2f}",
                                  colorBuffer, red, green, blue, alpha);

   auto address = OSEffectiveToPhysical(virt_cast<virt_addr>(colorBuffer->surface.image));
   auto cb_color_frag = latte::CB_COLORN_FRAG::get(0);
   auto cb_color_base = latte::CB_COLORN_BASE::get(0)
      .BASE_256B(address >> 8);

   if (colorBuffer->surface.aa != 0) {
      auto aaAddress = OSEffectiveToPhysical(virt_cast<virt_addr>(colorBuffer->aaBuffer));
      cb_color_frag = cb_color_frag
         .BASE_256B(aaAddress >> 8);
   }

   GX2InitColorBufferRegs(colorBuffer);

   internal::writePM4(latte::pm4::DecafClearColor {
      red, green, blue, alpha,
      cb_color_base,
      cb_color_frag,
      colorBuffer->regs.cb_color_size,
      colorBuffer->regs.cb_color_info,
      colorBuffer->regs.cb_color_view,
      colorBuffer->regs.cb_color_mask
   });

   internal::debugCaptureTagGroup(GX2DebugTag::ClearColor,
                                  "{}, {:.2f}, {:.2f}, {:.2f}, {:.2f}",
                                  colorBuffer, red, green, blue, alpha);
}

void
DecafClearDepthStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                       GX2ClearFlags clearFlags)
{
   auto addrImage = OSEffectiveToPhysical(virt_cast<virt_addr>(depthBuffer->surface.image));
   auto addrHiZ = OSEffectiveToPhysical(virt_cast<virt_addr>(depthBuffer->hiZPtr));

   auto db_depth_base = latte::DB_DEPTH_BASE::get(0)
      .BASE_256B(addrImage >> 8);

   auto db_depth_htile_data_base = latte::DB_DEPTH_HTILE_DATA_BASE::get(0)
      .BASE_256B(addrHiZ >> 8);

   GX2InitDepthBufferRegs(depthBuffer);

   internal::writePM4(latte::pm4::DecafClearDepthStencil {
      clearFlags,
      db_depth_base,
      db_depth_htile_data_base,
      depthBuffer->regs.db_depth_info,
      depthBuffer->regs.db_depth_size,
      depthBuffer->regs.db_depth_view,
   });
}

void
GX2ClearDepthStencilEx(virt_ptr<GX2DepthBuffer> depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags clearFlags)
{
   internal::debugCaptureTagGroup(GX2DebugTag::ClearDepthStencil,
                                  "{}, {:.2f}, {}, {}",
                                  depthBuffer, depth, stencil, clearFlags);

   uint32_t values[] = {
      stencil,
      bit_cast<uint32_t>(depth)
   };

   internal::writePM4(latte::pm4::SetContextRegs {
      latte::Register::DB_STENCIL_CLEAR,
      gsl::make_span(values)
   });
   DecafClearDepthStencil(depthBuffer, clearFlags);

   internal::debugCaptureTagGroup(GX2DebugTag::ClearDepthStencil,
                                  "{}, {:.2f}, {}, {}",
                                  depthBuffer, depth, stencil, clearFlags);
}

void
GX2ClearBuffersEx(virt_ptr<GX2ColorBuffer> colorBuffer,
                  virt_ptr<GX2DepthBuffer> depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags clearFlags)
{
   internal::debugCaptureTagGroup(
      GX2DebugTag::ClearBuffers,
      "{}, {}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {}, {}",
      colorBuffer, depthBuffer,
      red, green, blue, alpha,
      depth, stencil, clearFlags);

   GX2ClearColor(colorBuffer, red, green, blue, alpha);
   GX2ClearDepthStencilEx(depthBuffer, depth, stencil, clearFlags);

   internal::debugCaptureTagGroup(
      GX2DebugTag::ClearBuffers,
      "{}, {}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {}, {}",
      colorBuffer, depthBuffer,
      red, green, blue, alpha,
      depth, stencil, clearFlags);
}

void
GX2SetClearDepth(virt_ptr<GX2DepthBuffer> depthBuffer,
                 float depth)
{
   depthBuffer->depthClear = depth;
   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::DB_DEPTH_CLEAR,
      bit_cast<uint32_t>(depth)
   });
}

void
GX2SetClearStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                   uint8_t stencil)
{
   depthBuffer->stencilClear = stencil;
   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::DB_STENCIL_CLEAR,
      stencil
   });
}


void
GX2SetClearDepthStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                        float depth,
                        uint8_t stencil)
{
   depthBuffer->depthClear = depth;
   depthBuffer->stencilClear = stencil;

   uint32_t values[] = {
      stencil,
      bit_cast<uint32_t>(depth)
   };

   internal::writePM4(latte::pm4::SetContextRegs {
      latte::Register::DB_STENCIL_CLEAR,
      gsl::make_span(values)
   });
}

void
Library::registerClearSymbols()
{
   RegisterFunctionExport(GX2ClearColor);
   RegisterFunctionExport(GX2ClearDepthStencilEx);
   RegisterFunctionExport(GX2ClearBuffersEx);
   RegisterFunctionExport(GX2SetClearDepth);
   RegisterFunctionExport(GX2SetClearStencil);
   RegisterFunctionExport(GX2SetClearDepthStencil);
}

} // namespace cafe::gx2
