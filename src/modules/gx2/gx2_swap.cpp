#include "gx2_surface.h"
#include "gx2_swap.h"
#include "gpu/pm4_writer.h"

static uint32_t
gSwapInterval = 1;

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget::Value scanTarget)
{
   uint32_t addr256, aaAddr256;
   addr256 = buffer->surface.image.getAddress() >> 8;

   if (buffer->surface.aa != 0) {
      aaAddr256 = buffer->aaBuffer.getAddress() >> 8;
   } else {
      aaAddr256 = 0;
   }

   pm4::write(pm4::DecafCopyColorToScan {
      scanTarget,
      addr256,
      aaAddr256,
      buffer->regs.cb_color_size,
      buffer->regs.cb_color_info,
      buffer->regs.cb_color_view,
      buffer->regs.cb_color_mask
   });
}

void
GX2SwapScanBuffers()
{
   pm4::write(pm4::DecafSwapBuffers{ });
}

BOOL
GX2GetLastFrame(GX2ScanTarget::Value scanTarget, GX2Texture *texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Value scanTarget, be_val<float> *gamma)
{
   return FALSE;
}

void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync)
{
}

uint32_t
GX2GetSwapInterval()
{
   return gSwapInterval;
}

void
GX2SetSwapInterval(uint32_t interval)
{
   gSwapInterval = interval;
}
