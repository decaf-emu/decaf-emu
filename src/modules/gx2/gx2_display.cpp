#include "gx2_display.h"
#include "gpu/driver.h"

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
   *size = 1920 * 1080 * 4;
   *unkOut = 0;
}

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   *size = 854 * 480 * 4;
   *unkOut = 0;
}

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode)
{
   int tvWidth = 0, tvHeight = 0;

   if (tvRenderMode == GX2TVRenderMode::Standard480p) {
      tvWidth = 640;
      tvHeight = 480;
   } else if (tvRenderMode == GX2TVRenderMode::Wide480p) {
      tvWidth = 704;
      tvHeight = 480;
   } else if (tvRenderMode == GX2TVRenderMode::Wide720p) {
      tvWidth = 1280;
      tvHeight = 720;
   } else if (tvRenderMode == GX2TVRenderMode::Wide1080p) {
      tvWidth = 1920;
      tvHeight = 1080;
   } else {
      throw std::invalid_argument("Unexpected GX2SetTVBuffer tvRenderMode");
   }

   gpu::driver::setTvDisplay(tvWidth, tvHeight);
}

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode)
{
   int drcWidth = 854, drcHeight = 480;
   gpu::driver::setDrcDisplay(drcWidth, drcHeight);
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
