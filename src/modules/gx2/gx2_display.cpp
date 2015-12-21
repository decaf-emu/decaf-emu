#include "gx2_display.h"
#include "gpu/driver.h"

static int
surfaceBitsPerPixel(GX2SurfaceFormat surfaceFormat)
{
   switch (surfaceFormat) {

   case GX2SurfaceFormat::INVALID:
      return 0;

   case GX2SurfaceFormat::UNORM_BC1:
   case GX2SurfaceFormat::UNORM_BC4:
   case GX2SurfaceFormat::SRGB_BC1:
      return 4;

   case GX2SurfaceFormat::UNORM_R4_G4:
   case GX2SurfaceFormat::UNORM_R8:
   case GX2SurfaceFormat::UNORM_BC2:
   case GX2SurfaceFormat::UNORM_BC3:
   case GX2SurfaceFormat::UNORM_BC5:
   case GX2SurfaceFormat::UINT_R8:
   case GX2SurfaceFormat::SNORM_R8:
   case GX2SurfaceFormat::SNORM_BC4:
   case GX2SurfaceFormat::SNORM_BC5:
   case GX2SurfaceFormat::SINT_R8:
   case GX2SurfaceFormat::SRGB_BC2:
   case GX2SurfaceFormat::SRGB_BC3:
      return 8;

   case GX2SurfaceFormat::UNORM_NV12:
      return 12;

   case GX2SurfaceFormat::UNORM_R4_G4_B4_A4:
   case GX2SurfaceFormat::UNORM_R8_G8:
   case GX2SurfaceFormat::UNORM_R16:
   case GX2SurfaceFormat::UNORM_R5_G6_B5:
   case GX2SurfaceFormat::UNORM_R5_G5_B5_A1:
   case GX2SurfaceFormat::UNORM_A1_B5_G5_R5:
   case GX2SurfaceFormat::UINT_R8_G8:
   case GX2SurfaceFormat::UINT_R16:
   case GX2SurfaceFormat::SNORM_R8_G8:
   case GX2SurfaceFormat::SNORM_R16:
   case GX2SurfaceFormat::SINT_R8_G8:
   case GX2SurfaceFormat::SINT_R16:
   case GX2SurfaceFormat::FLOAT_R16:
      return 16;

   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
   case GX2SurfaceFormat::UNORM_R16_G16:
   case GX2SurfaceFormat::UNORM_R24_X8:
   case GX2SurfaceFormat::UNORM_A2_B10_G10_R10:
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
   case GX2SurfaceFormat::UINT_R16_G16:
   case GX2SurfaceFormat::UINT_R32:
   case GX2SurfaceFormat::UINT_A2_B10_G10_R10:
   case GX2SurfaceFormat::UINT_R10_G10_B10_A2:
   case GX2SurfaceFormat::UINT_X24_G8:
   case GX2SurfaceFormat::UINT_G8_X24:
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
   case GX2SurfaceFormat::SNORM_R16_G16:
   case GX2SurfaceFormat::SNORM_R10_G10_B10_A2:
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
   case GX2SurfaceFormat::SINT_R16_G16:
   case GX2SurfaceFormat::SINT_R32:
   case GX2SurfaceFormat::SINT_R10_G10_B10_A2:
   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
   case GX2SurfaceFormat::FLOAT_R32:
   case GX2SurfaceFormat::FLOAT_R16_G16:
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
   case GX2SurfaceFormat::FLOAT_D24_S8:
   case GX2SurfaceFormat::FLOAT_X8_X24:
      return 32;

   case GX2SurfaceFormat::UNORM_R16_G16_B16_A16:
   case GX2SurfaceFormat::UINT_R16_G16_B16_A16:
   case GX2SurfaceFormat::UINT_R32_G32:
   case GX2SurfaceFormat::SNORM_R16_G16_B16_A16:
   case GX2SurfaceFormat::SINT_R16_G16_B16_A16:
   case GX2SurfaceFormat::SINT_R32_G32:
   case GX2SurfaceFormat::FLOAT_R32_G32:
   case GX2SurfaceFormat::FLOAT_R16_G16_B16_A16:
      return 64;

   case GX2SurfaceFormat::UINT_R32_G32_B32_A32:
   case GX2SurfaceFormat::SINT_R32_G32_B32_A32:
   case GX2SurfaceFormat::FLOAT_R32_G32_B32_A32:
      return 128;
   }

   return 0;
}

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

   const int bitsPerPixel = surfaceBitsPerPixel(surfaceFormat);
   if (!bitsPerPixel) {
      throw std::invalid_argument("Unexpected GX2CalcTVSize surfaceFormat");
   }

   // The values of GX2BufferingMode constants are conveniently equal to
   // the number of buffers each constant specifies.
   const int numBuffers = static_cast<int>(bufferingMode);

   *size = (tvWidth * tvHeight * bitsPerPixel / 8) * numBuffers;
   *unkOut = 0;
}

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   const int bitsPerPixel = surfaceBitsPerPixel(surfaceFormat);
   if (!bitsPerPixel) {
      throw std::invalid_argument("Unexpected GX2CalcDRCSize surfaceFormat");
   }

   const int numBuffers = static_cast<int>(bufferingMode);

   *size = (864 * 480 * bitsPerPixel / 8) * numBuffers;
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
