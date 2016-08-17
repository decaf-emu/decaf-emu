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
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget scanTarget)
{
   uint32_t addr256, aaAddr256;
   addr256 = buffer->surface.image.getAddress() >> 8;

   if (buffer->surface.aa != 0) {
      aaAddr256 = buffer->aaBuffer.getAddress() >> 8;
   } else {
      aaAddr256 = 0;
   }

   GX2InitColorBufferRegs(buffer);

   pm4::write(pm4::DecafCopyColorToScan {
      scanTarget,
      addr256,
      aaAddr256,
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
GX2GetLastFrame(GX2ScanTarget scanTarget, GX2Texture *texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget, be_val<float> *gamma)
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
