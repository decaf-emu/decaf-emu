#include "gx2_debug.h"
#include "gx2_event.h"
#include "gx2_surface.h"
#include "gx2_swap.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

static uint32_t
sSwapInterval = 1;

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer,
                               GX2ScanTarget scanTarget)
{
   auto cb_color_frag = latte::CB_COLOR0_FRAG::get(0);
   auto cb_color_base = latte::CB_COLOR0_BASE::get(0)
      .BASE_256B(buffer->surface.image.getAddress() >> 8);

   if (buffer->surface.aa != 0) {
      cb_color_frag = cb_color_frag.BASE_256B(buffer->aaBuffer.getAddress() >> 8);
   }

   GX2InitColorBufferRegs(buffer);

   pm4::write(pm4::DecafCopyColorToScan {
      scanTarget,
      cb_color_base,
      cb_color_frag,
      buffer->surface.width,
      buffer->surface.height,
      buffer->regs.cb_color_size,
      buffer->regs.cb_color_info,
      buffer->regs.cb_color_view,
      buffer->regs.cb_color_mask
   });
}

void
GX2SwapScanBuffers()
{
   // For Debugging
   static uint32_t debugSwapCount = 0;
   internal::writeDebugMarker("SwapScanBuffers", debugSwapCount++);

   gx2::internal::onSwap();
   pm4::write(pm4::DecafSwapBuffers { });
}

BOOL
GX2GetLastFrame(GX2ScanTarget scanTarget,
                GX2Texture *texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget,
                     be_val<float> *gamma)
{
   return FALSE;
}

uint32_t
GX2GetSwapInterval()
{
   return sSwapInterval;
}

void
GX2SetSwapInterval(uint32_t interval)
{
   if (interval != sSwapInterval) {
      if (interval == 0) {
         gLog->warn("Swap interval set to 0!\n");
      }

      sSwapInterval = interval;
      pm4::write(pm4::DecafSetSwapInterval { interval });
   }
}

} // namespace gx2
