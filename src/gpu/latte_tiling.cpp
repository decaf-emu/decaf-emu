#include "latte_tiling.h"
#include "mesa_r600_tiling.h"

void
untileSurface(GX2Surface *surface, std::vector<uint8_t> &data, uint32_t &rowPitch)
{
   // TODO: This way of untiling (using expandX,expandY) will
   //   only work with BC compressed textures...
   auto cpp = 1;

   switch (surface->format) {
   case GX2SurfaceFormat::UNORM_BC1:
      cpp = 8;
      break;
   case GX2SurfaceFormat::UNORM_BC3:
      cpp = 16;
      break;
   default:
      __debugbreak();
   }

   auto expandX = 4;
   auto expandY = 4;

   auto surfaceWidth = (int)surface->width;
   auto surfaceHeight = (int)surface->height;
   auto surfacePitch = (int)surface->pitch;
   auto surfaceData = reinterpret_cast<uint8_t*>(surface->image.get());
   auto surfaceDataSize = surface->imageSize;
   data.resize(surfaceDataSize);

   mesa::radeon_renderbuffer texture;
   mesa::radeon_renderbuffer::data rbdata;
   texture.has_surface = false;
   texture.cpp = cpp;
   texture.base.Width = surfaceWidth / expandX;
   texture.base.Height = surfaceHeight / expandY;
   texture.pitch = surfacePitch * texture.cpp;
   texture.group_bytes = 256;

   switch (surface->tileMode) {
   case GX2TileMode::Tiled2DThin1:
      texture.num_channels = 2;
      texture.num_banks = 4;
      break;
   default:
      __debugbreak();
   }

   texture.r7xx_bank_op = 0;
   texture.bo = &rbdata;
   texture.bo->flags = RADEON_BO_FLAGS_MACRO_TILE;
   texture.bo->ptr = surfaceData;

   auto outBuf = &data[0];

   for (int y = 0; y < texture.base.Height; y++) {
      for (int x = 0; x < texture.base.Width; x++) {
         auto blockData = r600_ptr_color(&texture, x, y);
         memcpy(outBuf + (y*texture.pitch + x*texture.cpp), blockData, texture.cpp);
      }
   }

   rowPitch = texture.pitch;
}
