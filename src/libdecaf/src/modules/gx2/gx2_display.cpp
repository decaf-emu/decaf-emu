#include "gx2_display.h"
#include "gx2_format.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

static bool
getTVSize(GX2TVRenderMode tvRenderMode, int *width_ret, int *height_ret)
{
   switch (tvRenderMode) {
   case GX2TVRenderMode::Standard480p:
      *width_ret = 640;
      *height_ret = 480;
      return true;
   case GX2TVRenderMode::Wide480p:
      *width_ret = 704;
      *height_ret = 480;
      return true;
   case GX2TVRenderMode::Wide720p:
      *width_ret = 1280;
      *height_ret = 720;
      return true;
   case GX2TVRenderMode::Wide1080p:
      *width_ret = 1920;
      *height_ret = 1080;
      return true;
   }

   return false;
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
   int tvWidth, tvHeight;
   if (!getTVSize(tvRenderMode, &tvWidth, &tvHeight)) {
      throw std::invalid_argument("Unexpected GX2CalcTVSize tvRenderMode");
   }

   const int bytesPerPixel = GX2GetSurfaceFormatBytesPerElement(surfaceFormat);
   if (!bytesPerPixel) {
      throw std::invalid_argument("Unexpected GX2CalcTVSize surfaceFormat");
   }

   // The values of GX2BufferingMode constants are conveniently equal to
   // the number of buffers each constant specifies.
   const int numBuffers = static_cast<int>(bufferingMode);

   *size = tvWidth * tvHeight * bytesPerPixel * numBuffers;
   *unkOut = 0;
}

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   const int bytesPerPixel = GX2GetSurfaceFormatBytesPerElement(surfaceFormat);
   if (!bytesPerPixel) {
      throw std::invalid_argument("Unexpected GX2CalcDRCSize surfaceFormat");
   }

   const int numBuffers = static_cast<int>(bufferingMode);

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
   int tvWidth, tvHeight;
   if (!getTVSize(tvRenderMode, &tvWidth, &tvHeight)) {
      throw std::invalid_argument("Unexpected GX2SetTVBuffer tvRenderMode");
   }

   // bufferingMode is conveniently equal to the number of buffers
   pm4::write(pm4::DecafSetBuffer{
      1, 
      bufferingMode, 
      static_cast<uint32_t>(tvWidth), 
      static_cast<uint32_t>(tvHeight)
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
   pm4::write(pm4::DecafSetBuffer{
      0,
      bufferingMode,
      static_cast<uint32_t>(drcWidth),
      static_cast<uint32_t>(drcHeight)
   });
}

void
GX2SetTVScale(uint32_t x, uint32_t y)
{
}

void
GX2SetDRCScale(uint32_t x, uint32_t y)
{
}

GX2TVScanMode
GX2GetSystemTVScanMode()
{
   return GX2TVScanMode::None;
}

GX2DrcRenderMode
GX2GetSystemDRCMode()
{
   return GX2DrcRenderMode::Single;
}

} // namespace gx2
