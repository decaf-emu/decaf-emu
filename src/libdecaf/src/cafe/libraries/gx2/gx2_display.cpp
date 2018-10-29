#include "gx2.h"
#include "gx2_cbpool.h"
#include "gx2_debug.h"
#include "gx2_debugcapture.h"
#include "gx2_internal_pm4cap.h"
#include "gx2_display.h"
#include "gx2_enum_string.h"
#include "gx2_event.h"
#include "gx2_format.h"
#include "gx2_surface.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;

struct StaticDisplayData
{
   be2_struct<GX2ColorBuffer> tvScanBuffer;
   be2_struct<GX2ColorBuffer> drcScanBuffer;
   be2_val<GX2TVScanMode> tvScanMode;
   be2_val<GX2TVRenderMode> tvRenderMode;
   be2_val<GX2BufferingMode> tvBufferingMode;
   be2_val<GX2DrcRenderMode> drcRenderMode;
   be2_val<GX2BufferingMode> drcBufferingMode;
   be2_val<GX2DRCConnectCallbackFunction> drcConnectCallback;
   be2_val<uint32_t> swapInterval;
};

static virt_ptr<StaticDisplayData>
sDisplayData = nullptr;

static std::pair<unsigned, unsigned>
getTVSize(GX2TVRenderMode mode)
{
   switch (mode) {
   case GX2TVRenderMode::Standard480p:
      return { 640, 480 };
   case GX2TVRenderMode::Wide480p:
      return { 854, 480 };
   case GX2TVRenderMode::Wide720p:
      return { 1280, 720 };
   case GX2TVRenderMode::Unk720p:
      return { 1280, 720 };
   case GX2TVRenderMode::Wide1080p:
      return { 1920, 1080 };
   default:
      decaf_abort(fmt::format("Invalid GX2TVRenderMode {}", to_string(mode)));
   }
}

static unsigned
getBpp(GX2SurfaceFormat format)
{
   auto bpp = internal::getSurfaceFormatBytesPerElement(format);
   decaf_assert(bpp > 0, fmt::format("Unexpected GX2SurfaceFormat {}", to_string(format)));
   return bpp;
}

static unsigned
getNumBuffers(GX2BufferingMode mode)
{
   switch (mode) {
   case GX2BufferingMode::Single:
      return 1;
   case GX2BufferingMode::Double:
      return 2;
   case GX2BufferingMode::Triple:
      return 3;
   default:
      decaf_abort(fmt::format("Invalid GX2BufferingMode {}", to_string(mode)));
   }
}

static void
initialiseScanBuffer(virt_ptr<GX2ColorBuffer> buffer,
                     uint32_t width,
                     uint32_t height,
                     GX2SurfaceFormat format)
{
   std::memset(buffer.get(), 0, sizeof(GX2ColorBuffer));
   buffer->surface.use = GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::Texture;
   buffer->surface.width = width;
   buffer->surface.height = height;
   buffer->surface.mipLevels = 1u;
   buffer->surface.dim = GX2SurfaceDim::Texture2D;
   buffer->surface.swizzle = 0u;
   buffer->surface.depth = 1u;
   buffer->surface.tileMode = GX2TileMode::Default;
   buffer->surface.format = format;
   buffer->surface.mipmaps = nullptr;
   buffer->surface.aa = GX2AAMode::Mode1X;
   buffer->viewFirstSlice = 0u;
   buffer->viewNumSlices = 1u;
   buffer->viewMip = 0u;
   GX2CalcSurfaceSizeAndAlignment(virt_addrof(buffer->surface));
   GX2InitColorBufferRegs(buffer);
   buffer->surface.use |= GX2SurfaceUse::ScanBuffer;
}

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               virt_ptr<uint32_t> outSize,
               virt_ptr<uint32_t> outUnk)
{
   auto bytesPerPixel = internal::getSurfaceFormatBytesPerElement(surfaceFormat);
   auto numBuffers = getNumBuffers(bufferingMode);

   *outSize = 864 * 480 * bytesPerPixel * numBuffers;
   *outUnk = 0u;
}

void
GX2CalcTVSize(GX2TVRenderMode tvRenderMode,
              GX2SurfaceFormat surfaceFormat,
              GX2BufferingMode bufferingMode,
              virt_ptr<uint32_t> outSize,
              virt_ptr<uint32_t> outUnk)
{
   unsigned width, height;
   std::tie(width, height) = getTVSize(tvRenderMode);

   auto bytesPerPixel = getBpp(surfaceFormat);
   auto numBuffers = getNumBuffers(bufferingMode);

   *outSize = width * height * bytesPerPixel * numBuffers;
   *outUnk = 0u;
}

void
GX2CopyColorBufferToScanBuffer(virt_ptr<GX2ColorBuffer> buffer,
                               GX2ScanTarget scanTarget)
{
   internal::debugCaptureTagGroup(GX2DebugTag::CopyColorBufferToScanBuffer,
                                  "{}, {}", buffer, scanTarget);

   auto addrImage = OSEffectiveToPhysical(virt_cast<virt_addr>(buffer->surface.image));
   auto cb_color_frag = latte::CB_COLORN_FRAG::get(0);
   auto cb_color_base = latte::CB_COLORN_BASE::get(0)
      .BASE_256B(addrImage >> 8);

   if (buffer->surface.aa != 0) {
      auto addrAA = OSEffectiveToPhysical(virt_cast<virt_addr>(buffer->aaBuffer));
      cb_color_frag = cb_color_frag.BASE_256B(addrAA >> 8);
   }

   // TODO: We should check this function, this was added
   // as a temporary solution to new crashes.
   if (buffer->viewNumSlices == 0) {
      buffer->viewNumSlices = 1u;
   }

   GX2InitColorBufferRegs(buffer);

   internal::writePM4(latte::pm4::DecafCopyColorToScan {
      latte::pm4::ScanTarget(scanTarget),
      cb_color_base,
      cb_color_frag,
      buffer->surface.width,
      buffer->surface.height,
      buffer->regs.cb_color_size,
      buffer->regs.cb_color_info,
      buffer->regs.cb_color_view,
      buffer->regs.cb_color_mask
   });

   internal::debugCaptureTagGroup(GX2DebugTag::CopyColorBufferToScanBuffer,
                                  "{}, {}", buffer, scanTarget);
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

GX2TVScanMode
GX2GetSystemTVScanMode()
{
   return sDisplayData->tvScanMode;
}

GX2DrcRenderMode
GX2GetSystemDRCMode()
{
   return GX2DrcRenderMode::Single;
}

GX2AspectRatio
GX2GetSystemTVAspectRatio()
{
   switch (sDisplayData->tvScanMode) {
   case GX2TVScanMode::None:
   case GX2TVScanMode::I480:
   case GX2TVScanMode::P480:
      return GX2AspectRatio::Normal;
   case GX2TVScanMode::P720:
   case GX2TVScanMode::I1080:
   case GX2TVScanMode::P1080:
      return GX2AspectRatio::Widescreen;
   default:
      decaf_abort(fmt::format("Invalid GX2TVScanMode {}",
                              to_string(sDisplayData->tvScanMode)));
   }
}

uint32_t
GX2GetSwapInterval()
{
   return sDisplayData->swapInterval;
}

BOOL
GX2IsVideoOutReady()
{
   return TRUE;
}

void
GX2SetDRCBuffer(virt_ptr<void> buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode)
{
   constexpr auto width = 854u, height = 480u;

   initialiseScanBuffer(virt_addrof(sDisplayData->drcScanBuffer),
                        width, height,
                        surfaceFormat);
   sDisplayData->drcScanBuffer.surface.image = virt_cast<uint8_t *>(buffer);
   sDisplayData->drcRenderMode = drcRenderMode;
   sDisplayData->drcBufferingMode = bufferingMode;

   // bufferingMode is conveniently equal to the number of buffers
   internal::writePM4(latte::pm4::DecafSetBuffer {
      latte::pm4::ScanTarget::DRC,
      OSEffectiveToPhysical(virt_cast<virt_addr>(buffer)),
      bufferingMode,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   });
}

GX2DRCConnectCallbackFunction
GX2SetDRCConnectCallback(uint32_t id,
                         GX2DRCConnectCallbackFunction callback)
{
   auto old = sDisplayData->drcConnectCallback;
   sDisplayData->drcConnectCallback = callback;

   if (callback) {
      cafe::invoke(cpu::this_core::state(),
                   callback,
                   id,
                   TRUE);
   }

   return old;
}

void
GX2SetDRCEnable(BOOL enable)
{
}

void
GX2SetDRCScale(uint32_t x,
               uint32_t y)
{
}

void
GX2SetSwapInterval(uint32_t interval)
{
   if (interval == sDisplayData->swapInterval) {
      return;
   }

   sDisplayData->swapInterval = interval;
}

void
GX2SetTVBuffer(virt_ptr<void> buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode)
{
   unsigned width, height;
   std::tie(width, height) = getTVSize(tvRenderMode);

   initialiseScanBuffer(virt_addrof(sDisplayData->tvScanBuffer),
                        width, height, surfaceFormat);
   sDisplayData->tvScanBuffer.surface.image = virt_cast<uint8_t *>(buffer);
   sDisplayData->tvRenderMode = tvRenderMode;
   sDisplayData->tvBufferingMode = bufferingMode;

   /*
   auto pitch = width;
   AVMSetTVScale(width, height);
   AVMSetTVBufferAttr(bufferingMode, tvRenderMode, pitch);
   */

   // bufferingMode is conveniently equal to the number of buffers
   internal::writePM4(latte::pm4::DecafSetBuffer {
      latte::pm4::ScanTarget::TV,
      OSEffectiveToPhysical(virt_cast<virt_addr>(buffer)),
      bufferingMode,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   });
}

void
GX2SetTVEnable(BOOL enable)
{
}

void
GX2SetTVScale(uint32_t x,
              uint32_t y)
{
}

void
GX2SwapScanBuffers()
{
   // For Debugging
   static uint32_t debugSwapCount = 0;
   internal::debugCaptureTagGroup(GX2DebugTag::SwapScanBuffers);

   internal::onSwap();
   internal::writePM4(latte::pm4::DecafSwapBuffers { });

   internal::debugCaptureTagGroup(GX2DebugTag::SwapScanBuffers);
   internal::captureSwap();
   internal::debugCaptureSwap(virt_addrof(sDisplayData->tvScanBuffer.surface),
                              virt_addrof(sDisplayData->drcScanBuffer.surface));
}

namespace internal
{

void
initialiseDisplay()
{
   sDisplayData->tvScanMode = GX2TVScanMode::P1080;
   sDisplayData->tvRenderMode = GX2TVRenderMode::Disabled;
   sDisplayData->drcRenderMode = GX2DrcRenderMode::Disabled;
   sDisplayData->swapInterval = 1u;
}

virt_ptr<GX2Surface>
getTvScanBuffer()
{
   return virt_addrof(sDisplayData->tvScanBuffer.surface);
}

virt_ptr<GX2Surface>
getDrcScanBuffer()
{
   return virt_addrof(sDisplayData->drcScanBuffer.surface);
}

} // namespace internal

void
Library::registerDisplaySymbols()
{
   RegisterFunctionExport(GX2CalcDRCSize);
   RegisterFunctionExport(GX2CalcTVSize);
   RegisterFunctionExport(GX2CopyColorBufferToScanBuffer);
   RegisterFunctionExport(GX2GetLastFrame);
   RegisterFunctionExport(GX2GetLastFrameGamma);
   RegisterFunctionExport(GX2GetSystemTVScanMode);
   RegisterFunctionExport(GX2GetSystemDRCMode);
   RegisterFunctionExport(GX2GetSystemTVAspectRatio);
   RegisterFunctionExport(GX2GetSwapInterval);
   RegisterFunctionExport(GX2IsVideoOutReady);
   RegisterFunctionExport(GX2SetDRCBuffer);
   RegisterFunctionExport(GX2SetDRCConnectCallback);
   RegisterFunctionExport(GX2SetDRCEnable);
   RegisterFunctionExport(GX2SetDRCScale);
   RegisterFunctionExport(GX2SetSwapInterval);
   RegisterFunctionExport(GX2SetTVBuffer);
   RegisterFunctionExport(GX2SetTVEnable);
   RegisterFunctionExport(GX2SetTVScale);
   RegisterFunctionExport(GX2SwapScanBuffers);

   RegisterDataInternal(sDisplayData);
}

} // namespace cafe::gx2
