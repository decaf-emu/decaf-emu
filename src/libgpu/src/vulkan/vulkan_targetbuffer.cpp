#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "latte/latte_formats.h"

namespace vulkan
{

SurfaceObject *
Driver::getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                       latte::CB_COLORN_SIZE cb_color_size,
                       latte::CB_COLORN_INFO cb_color_info,
                       bool discardData)
{
   auto baseAddress = phys_addr((cb_color_base.BASE_256B() << 8) & 0xFFFFF800);
   auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX();
   auto slice_tile_max = cb_color_size.SLICE_TILE_MAX();

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto format = latte::getColorBufferDataFormat(cb_color_info.FORMAT(), cb_color_info.NUMBER_TYPE());
   auto tileMode = latte::getArrayModeTileMode(cb_color_info.ARRAY_MODE());

   auto surfaceDesc = SurfaceInfo {
      baseAddress, pitch, pitch, height, 1, 0, latte::SQ_TEX_DIM::DIM_2D,
      format.format, format.numFormat, format.formatComp, format.degamma,
      false, tileMode
   };
   return getSurface(surfaceDesc, discardData);
}

SurfaceObject *
Driver::getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                       latte::DB_DEPTH_SIZE db_depth_size,
                       latte::DB_DEPTH_INFO db_depth_info,
                       bool discardData)
{
   auto baseAddress = phys_addr((db_depth_base.BASE_256B() << 8) & 0xFFFFF800);
   auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX();
   auto slice_tile_max = db_depth_size.SLICE_TILE_MAX();

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto format = latte::getDepthBufferDataFormat(db_depth_info.FORMAT());
   auto tileMode = latte::getArrayModeTileMode(db_depth_info.ARRAY_MODE());

   auto surfaceDesc = SurfaceInfo {
      baseAddress, pitch, pitch, height, 1, 0, latte::SQ_TEX_DIM::DIM_2D,
      format.format, format.numFormat, format.formatComp, format.degamma,
      true, tileMode
   };
   return getSurface(surfaceDesc, discardData);
}

} // namespace vulkan

#endif // DECAF_VULKAN
