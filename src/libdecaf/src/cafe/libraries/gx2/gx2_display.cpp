#include "gx2.h"
#include "gx2_display.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gx2_internal_cbpool.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::gx2
{

struct StaticDisplayData
{
   be2_val<GX2TVScanMode> tvScanMode;
   be2_val<GX2DRCConnectCallbackFunction> drcConnectCallback;
   be2_struct<internal::ScreenBufferInfo> tvBuffer;
   be2_struct<internal::ScreenBufferInfo> drcBuffer;
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

void
GX2SetTVEnable(BOOL enable)
{
}

void
GX2SetDRCEnable(BOOL enable)
{
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
GX2SetTVBuffer(virt_ptr<void> buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode)
{
   unsigned width, height;
   std::tie(width, height) = getTVSize(tvRenderMode);

   sDisplayData->tvBuffer.buffer = buffer;
   sDisplayData->tvBuffer.size = size;
   sDisplayData->tvBuffer.tvRenderMode = tvRenderMode;
   sDisplayData->tvBuffer.surfaceFormat = surfaceFormat;
   sDisplayData->tvBuffer.bufferingMode = bufferingMode;
   sDisplayData->tvBuffer.width = width;
   sDisplayData->tvBuffer.height = height;
   /*
   void AVMSetTVScale(uint32_t width, uint32_t height);
   void AVMSetTVBufferAttr(GX2BufferingMode bufferingMode, GX2TVRenderMode tvRenderMode, uint32_t pitch);
   auto pitch = width;
   AVMSetTVScale(width, height);
   AVMSetTVBufferAttr(bufferingMode, tvRenderMode, pitch);
   */
   // bufferingMode is conveniently equal to the number of buffers
   internal::writePM4(latte::pm4::DecafSetBuffer {
      1,
      bufferingMode,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   });
}

void
GX2SetDRCBuffer(virt_ptr<void> buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode)
{
   int width = 854, height = 480;

   sDisplayData->drcBuffer.buffer = buffer;
   sDisplayData->drcBuffer.size = size;
   sDisplayData->drcBuffer.drcRenderMode = drcRenderMode;
   sDisplayData->drcBuffer.surfaceFormat = surfaceFormat;
   sDisplayData->drcBuffer.bufferingMode = bufferingMode;
   sDisplayData->drcBuffer.width = width;
   sDisplayData->drcBuffer.height = height;

   // bufferingMode is conveniently equal to the number of buffers
   internal::writePM4(latte::pm4::DecafSetBuffer {
      0,
      bufferingMode,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   });
}

void
GX2SetTVScale(uint32_t x,
              uint32_t y)
{
}

void
GX2SetDRCScale(uint32_t x,
               uint32_t y)
{
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

BOOL
GX2IsVideoOutReady()
{
   return TRUE;
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

namespace internal
{

void
initialiseDisplay()
{
   sDisplayData->tvScanMode = GX2TVScanMode::P1080;
}

virt_ptr<ScreenBufferInfo>
getTvBufferInfo()
{
   return virt_addrof(sDisplayData->tvBuffer);
}

virt_ptr<ScreenBufferInfo>
getDrcBufferInfo()
{
   return virt_addrof(sDisplayData->drcBuffer);
}

} // namespace internal

void
Library::registerDisplaySymbols()
{
   RegisterFunctionExport(GX2SetTVEnable);
   RegisterFunctionExport(GX2SetDRCEnable);
   RegisterFunctionExport(GX2CalcTVSize);
   RegisterFunctionExport(GX2SetTVBuffer);
   RegisterFunctionExport(GX2SetTVScale);
   RegisterFunctionExport(GX2CalcDRCSize);
   RegisterFunctionExport(GX2SetDRCBuffer);
   RegisterFunctionExport(GX2SetDRCScale);
   RegisterFunctionExport(GX2GetSystemTVScanMode);
   RegisterFunctionExport(GX2GetSystemDRCMode);
   RegisterFunctionExport(GX2GetSystemTVAspectRatio);
   RegisterFunctionExport(GX2IsVideoOutReady);
   RegisterFunctionExport(GX2SetDRCConnectCallback);

   RegisterDataInternal(sDisplayData);
}

} // namespace cafe::gx2
