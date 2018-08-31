#include "gx2.h"
#include "gx2_debug.h"
#include "gx2_event.h"
#include "gx2_internal_cbpool.h"
#include "gx2_internal_pm4cap.h"
#include "gx2_surface.h"
#include "gx2_swap.h"

#include <common/log.h>

namespace cafe::gx2
{

struct StaticSwapData
{
   be2_val<uint32_t> interval;
};

static virt_ptr<StaticSwapData>
sSwapData = nullptr;

void
GX2CopyColorBufferToScanBuffer(virt_ptr<GX2ColorBuffer> buffer,
                               GX2ScanTarget scanTarget)
{
   auto cb_color_frag = latte::CB_COLORN_FRAG::get(0);
   auto cb_color_base = latte::CB_COLORN_BASE::get(0)
      .BASE_256B(static_cast<uint32_t>(virt_cast<virt_addr>(buffer->surface.image)) >> 8);

   if (buffer->surface.aa != 0) {
      cb_color_frag = cb_color_frag.BASE_256B(static_cast<uint32_t>(virt_cast<virt_addr>(buffer->aaBuffer)) >> 8);
   }

   GX2InitColorBufferRegs(buffer);

   internal::writePM4(latte::pm4::DecafCopyColorToScan {
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

   internal::onSwap();
   internal::writePM4(latte::pm4::DecafSwapBuffers { });
   internal::captureSwap();
}

BOOL
GX2GetLastFrame(GX2ScanTarget scanTarget,
                virt_ptr<GX2Texture> texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget,
                     virt_ptr<float> outGamma)
{
   return FALSE;
}

uint32_t
GX2GetSwapInterval()
{
   return sSwapData->interval;
}

void
GX2SetSwapInterval(uint32_t interval)
{
   if (interval != sSwapData->interval) {
      if (interval == 0) {
         gLog->warn("Swap interval set to 0!\n");
      }

      sSwapData->interval = interval;
      internal::writePM4(latte::pm4::DecafSetSwapInterval { interval });
   }
}

void
Library::registerSwapSymbols()
{
   RegisterFunctionExport(GX2CopyColorBufferToScanBuffer);
   RegisterFunctionExport(GX2SwapScanBuffers);
   RegisterFunctionExport(GX2GetLastFrame);
   RegisterFunctionExport(GX2GetLastFrameGamma);
   RegisterFunctionExport(GX2GetSwapInterval);
   RegisterFunctionExport(GX2SetSwapInterval);

   RegisterDataInternal(sSwapData);
}

} // namespace cafe::gx2
