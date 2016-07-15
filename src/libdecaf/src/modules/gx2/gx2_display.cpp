#include "common/decaf_assert.h"
#include "gx2_display.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

const static GX2TVScanMode
sTvScanMode = GX2TVScanMode::P1080;

static std::pair<unsigned, unsigned>
getTVSize(GX2TVRenderMode mode)
{
   switch (mode) {
   case GX2TVRenderMode::Standard480p:
      return { 640, 480 };
   case GX2TVRenderMode::Wide480p:
      return { 704, 480 };
   case GX2TVRenderMode::Wide720p:
      return { 1280, 720 };
   case GX2TVRenderMode::Wide1080p:
      return { 1920, 1080 };
   default:
      decaf_abort(fmt::format("Invalid GX2TVRenderMode {}", enumAsString(mode)));
   }
}

static unsigned
getBpp(GX2SurfaceFormat format)
{
   auto bpp = internal::getSurfaceFormatBytesPerElement(format);
   decaf_assert(bpp > 0, fmt::format("Unexpected GX2SurfaceFormat {}", enumAsString(format)));
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
      decaf_abort(fmt::format("Invalid GX2BufferingMode {}", enumAsString(mode)));
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
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut)
{
   unsigned width, height;
   std::tie(width, height) = getTVSize(tvRenderMode);

   auto bytesPerPixel = getBpp(surfaceFormat);
   auto numBuffers = getNumBuffers(bufferingMode);

   *size = width * height * bytesPerPixel * numBuffers;
   *unkOut = 0;
}

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   auto bytesPerPixel = internal::getSurfaceFormatBytesPerElement(surfaceFormat);
   auto numBuffers = getNumBuffers(bufferingMode);

   *size = 864 * 480 * bytesPerPixel * numBuffers;
   *unkOut = 0;
}

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode)
{
   unsigned width, height;
   std::tie(width, height) = getTVSize(tvRenderMode);

   // bufferingMode is conveniently equal to the number of buffers
   pm4::write(pm4::DecafSetBuffer {
      1,
      bufferingMode,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   });
}

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode)
{
   int drcWidth = 854, drcHeight = 480;

   // bufferingMode is conveniently equal to the number of buffers
   pm4::write(pm4::DecafSetBuffer {
      0,
      bufferingMode,
      static_cast<uint32_t>(drcWidth),
      static_cast<uint32_t>(drcHeight)
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
   return sTvScanMode;
}

GX2DrcRenderMode
GX2GetSystemDRCMode()
{
   return GX2DrcRenderMode::Single;
}

GX2AspectRatio
GX2GetSystemTVAspectRatio()
{
   switch (sTvScanMode) {
   case GX2TVScanMode::None:
   case GX2TVScanMode::I480:
   case GX2TVScanMode::P480:
      return GX2AspectRatio::Normal;
   case GX2TVScanMode::P720:
   case GX2TVScanMode::I1080:
   case GX2TVScanMode::P1080:
      return GX2AspectRatio::Widescreen;
   default:
      decaf_abort(fmt::format("Invalid GX2TVScanMode {}", enumAsString(sTvScanMode)));
   }
}

} // namespace gx2
