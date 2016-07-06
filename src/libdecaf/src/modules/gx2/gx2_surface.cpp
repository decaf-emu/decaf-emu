#include "gx2_addrlib.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gx2_surface.h"
#include "gpu/pm4_writer.h"
#include "common/align.h"
#include "common/log.h"
#include "common/pow.h"

namespace gx2
{

static uint32_t
calcNumLevels(GX2Surface *surface)
{
   if (surface->mipLevels <= 1) {
      return 1;
   }

   auto levels = std::max(Log2<uint32_t>(surface->width), Log2<uint32_t>(surface->height));

   if (surface->dim == GX2SurfaceDim::Texture3D) {
      levels = std::max(levels, Log2<uint32_t>(surface->depth));
   }

   return levels;
}

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
   auto isDepthBuffer = !!(surface->use & GX2SurfaceUse::DepthBuffer);
   auto isColorBuffer = !!(surface->use & GX2SurfaceUse::ColorBuffer);
   auto tileModeChanged = false;

   if (surface->tileMode == GX2TileMode::Default) {
      if (surface->dim || surface->aa || isDepthBuffer) {
         if (surface->dim != GX2SurfaceDim::Texture3D || isColorBuffer) {
            surface->tileMode = GX2TileMode::Tiled2DThin1;
         } else {
            surface->tileMode = GX2TileMode::Tiled2DThick;
         }

         tileModeChanged = true;
      } else {
         surface->tileMode = GX2TileMode::LinearAligned;
      }
   }

   surface->mipLevels = std::max<uint32_t>(surface->mipLevels, 1u);
   surface->mipLevels = std::min<uint32_t>(surface->mipLevels, calcNumLevels(surface));

   surface->mipLevelOffset[0] = 0;
   surface->swizzle &= 0xFF00FFFF;

   if (surface->tileMode >= GX2TileMode::Tiled2DThin1 && surface->tileMode != GX2TileMode::LinearSpecial) {
      surface->swizzle |= 0xD0000;
   }

   auto lastTileMode = static_cast<GX2TileMode>(surface->tileMode);
   auto prevSize = 0u;
   auto offset0 = 0u;

   for (auto level = 0u; level < surface->mipLevels; ++level) {
      gx2::internal::getSurfaceInfo(surface, level, &output);

      if (level) {
         auto pad = 0u;

         if (lastTileMode >= GX2TileMode::Tiled2DThin1 && lastTileMode != GX2TileMode::LinearSpecial) {
            if (output.tileMode < ADDR_TM_2D_TILED_THIN1 || output.tileMode == ADDR_TM_LINEAR_SPECIAL) {
               surface->swizzle = (level << 16) | (surface->swizzle & 0xFF00FFFF);
               lastTileMode = static_cast<GX2TileMode>(output.tileMode);

               if (level > 1) {
                  pad = surface->swizzle & 0xFFFF;
               }
            }
         }

         pad += (output.baseAlign - (prevSize % output.baseAlign)) % output.baseAlign;

         if (level == 1) {
            offset0 = pad + prevSize;
         } else {
            surface->mipLevelOffset[level - 1] = pad + prevSize + surface->mipLevelOffset[level - 2];
         }
      } else {
         if (tileModeChanged && surface->width < output.pitchAlign && surface->height < output.heightAlign) {
            if (surface->tileMode == GX2TileMode::Tiled2DThick) {
               surface->tileMode = GX2TileMode::Tiled1DThick;
            } else {
               surface->tileMode = GX2TileMode::Tiled1DThin1;
            }

            gx2::internal::getSurfaceInfo(surface, level, &output);
            surface->swizzle &= 0xFF00FFFF;
            lastTileMode = surface->tileMode;
         }

         surface->imageSize = static_cast<uint32_t>(output.surfSize);
         surface->alignment = output.baseAlign;
         surface->pitch = output.pitch;
      }

      prevSize = static_cast<uint32_t>(output.surfSize);
   }

   if (surface->mipLevels <= 1) {
      surface->mipmapSize = 0;
   } else {
      surface->mipmapSize = prevSize + surface->mipLevelOffset[surface->mipLevels - 2];
   }

   surface->mipLevelOffset[0] = offset0;

   if (surface->format == GX2SurfaceFormat::UNORM_NV12) {
      auto pad = (surface->alignment - surface->imageSize % surface->alignment) % surface->alignment;
      surface->mipLevelOffset[0] = pad + surface->imageSize;
      surface->imageSize = surface->mipLevelOffset[0] + (surface->imageSize >> 1);
   }
}

void
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer,
                          be_val<uint32_t> *outSize,
                          be_val<uint32_t> *outAlignment)
{
   ADDR_COMPUTE_HTILE_INFO_INPUT input;
   ADDR_COMPUTE_HTILE_INFO_OUTPUT output;

   std::memset(&input, 0, sizeof(ADDR_COMPUTE_HTILE_INFO_INPUT));
   std::memset(&output, 0, sizeof(ADDR_COMPUTE_HTILE_INFO_OUTPUT));

   input.size = sizeof(ADDR_COMPUTE_HTILE_INFO_INPUT);
   output.size = sizeof(ADDR_COMPUTE_HTILE_INFO_OUTPUT);

   input.pitch = depthBuffer->surface.pitch;
   input.height = depthBuffer->surface.height;
   input.numSlices = depthBuffer->surface.depth;
   input.blockWidth = ADDR_HTILE_BLOCKSIZE_8;
   input.blockHeight = ADDR_HTILE_BLOCKSIZE_8;
   AddrComputeHtileInfo(gx2::internal::getAddrLibHandle(), &input, &output);

   depthBuffer->hiZSize = gsl::narrow_cast<uint32_t>(output.htileBytes);

   if (outSize) {
      *outSize = depthBuffer->hiZSize;
   }

   if (outAlignment) {
      *outAlignment = output.baseAlign;
   }
}

void
GX2CalcColorBufferAuxInfo(GX2Surface *surface, be_val<uint32_t> *outSize, be_val<uint32_t> *outAlignment)
{
   gLog->warn("Application called GX2CalcColorBufferAuxInfo");
   *outSize = 2048;
   *outAlignment = 2048;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, GX2RenderTarget target)
{
   using latte::Register;
   uint32_t addr256, aaAddr256;
   auto reg = [](unsigned id) { return static_cast<Register>(id); };
   auto cb_color_info = colorBuffer->regs.cb_color_info.value();
   auto cb_color_mask = colorBuffer->regs.cb_color_mask.value();
   auto cb_color_size = colorBuffer->regs.cb_color_size.value();
   auto cb_color_view = colorBuffer->regs.cb_color_view.value();

   addr256 = colorBuffer->surface.image.getAddress() >> 8;
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_BASE + target * 4), addr256 });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_SIZE + target * 4), cb_color_size.value });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_INFO + target * 4), cb_color_info.value });

   if (colorBuffer->surface.aa != 0) {
      aaAddr256 = colorBuffer->aaBuffer.getAddress() >> 8;
   } else {
      aaAddr256 = 0;
   }

   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_TILE + target * 4), aaAddr256 });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_FRAG + target * 4), aaAddr256 });

   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_VIEW + target * 4), cb_color_view.value });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_MASK + target * 4), cb_color_mask.value });
}

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer)
{
   auto db_depth_info = depthBuffer->regs.db_depth_info.value();
   auto db_depth_size = depthBuffer->regs.db_depth_size.value();
   auto db_depth_view = depthBuffer->regs.db_depth_view.value();
   auto db_htile_surface = depthBuffer->regs.db_htile_surface.value();
   auto db_prefetch_limit = depthBuffer->regs.db_prefetch_limit.value();
   auto db_preload_control = depthBuffer->regs.db_preload_control.value();
   auto pa_poly_offset_cntl = depthBuffer->regs.pa_poly_offset_cntl.value();

   uint32_t values1[] = {
      db_depth_size.value,
      db_depth_view.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::DB_DEPTH_SIZE, gsl::as_span(values1) });

   uint32_t values2[] = {
      depthBuffer->surface.image.getAddress(),
      db_depth_info.value,
      depthBuffer->hiZPtr.getAddress(),
   };
   pm4::write(pm4::SetContextRegs { latte::Register::DB_DEPTH_BASE, gsl::as_span(values2) });

   pm4::write(pm4::SetContextReg { latte::Register::DB_HTILE_SURFACE, db_htile_surface.value });
   pm4::write(pm4::SetContextReg { latte::Register::DB_PREFETCH_LIMIT, db_prefetch_limit.value });
   pm4::write(pm4::SetContextReg { latte::Register::DB_PRELOAD_CONTROL, db_preload_control.value });
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_POLY_OFFSET_DB_FMT_CNTL, pa_poly_offset_cntl.value });

   uint32_t values3[] = {
      depthBuffer->stencilClear,
      bit_cast<uint32_t, float>(depthBuffer->depthClear),
   };
   pm4::write(pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, gsl::as_span(values3) });
}

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer)
{
   auto cb_color_info = latte::CB_COLOR0_INFO::get(0);
   auto cb_color_size = latte::CB_COLOR0_SIZE::get(0);

   // Update register values
   auto format = GX2GetSurfaceColorFormat(colorBuffer->surface.format);
   auto pitch = (colorBuffer->surface.pitch / latte::MicroTileWidth) - 1;
   auto slice = ((pitch + 1) * ((colorBuffer->surface.height + latte::MicroTileHeight - 1) / latte::MicroTileHeight)) - 1;

   cb_color_info = cb_color_info
      .FORMAT().set(format);

   cb_color_size = cb_color_size
      .PITCH_TILE_MAX().set(pitch)
      .SLICE_TILE_MAX().set(slice);

   // TODO: Set more regs!

   // Save big endian registers
   std::memset(&colorBuffer->regs, 0, sizeof(colorBuffer->regs));
   colorBuffer->regs.cb_color_info = cb_color_info;
   colorBuffer->regs.cb_color_size = cb_color_size;
}

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer)
{
   auto db_depth_info = latte::DB_DEPTH_INFO::get(0);
   auto db_depth_size = latte::DB_DEPTH_SIZE::get(0);

   // Update register values
   auto format = GX2GetSurfaceDepthFormat(depthBuffer->surface.format);
   auto pitch = (depthBuffer->surface.pitch / latte::MicroTileWidth) - 1;
   auto slice = ((depthBuffer->surface.pitch * depthBuffer->surface.height) / (latte::MicroTileWidth * latte::MicroTileHeight)) - 1;

   db_depth_info = db_depth_info
      .FORMAT().set(format);

   db_depth_size = db_depth_size
      .PITCH_TILE_MAX().set(pitch)
      .SLICE_TILE_MAX().set(slice);

   // TODO: Set more regs!

   // Save big endian registers
   std::memset(&depthBuffer->regs, 0, sizeof(depthBuffer->regs));
   depthBuffer->regs.db_depth_info = db_depth_info;
   depthBuffer->regs.db_depth_size = db_depth_size;
}

void
GX2InitDepthBufferHiZEnable(GX2DepthBuffer *depthBuffer,
                            BOOL enable)
{
   auto db_depth_info = depthBuffer->regs.db_depth_info.value();

   db_depth_info = db_depth_info
      .TILE_SURFACE_ENABLE().set(!!enable);

   depthBuffer->regs.db_depth_info = db_depth_info;
}

uint32_t
GX2GetSurfaceSwizzle(GX2Surface *surface)
{
   return (surface->swizzle >> 8) & 0xff;
}

void
GX2SetSurfaceSwizzle(GX2Surface *surface, uint32_t swizzle)
{
   surface->swizzle &= 0xFFFF00FF;
   surface->swizzle |= swizzle << 8;
}

uint32_t
GX2GetSurfaceMipPitch(GX2Surface *surface, uint32_t level)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   internal::getSurfaceInfo(surface, level, &info);
   return info.pitch;
}

void
GX2CopySurface(GX2Surface *src,
               uint32_t srcLevel,
               uint32_t srcDepth,
               GX2Surface *dst,
               uint32_t dstLevel,
               uint32_t dstDepth)
{
   if (src->format == GX2SurfaceFormat::INVALID || src->width == 0 || src->height == 0) {
      return;
   }

   if (dst->format == GX2SurfaceFormat::INVALID) {
      return;
   }

   gx2::internal::copySurface(src, srcLevel, srcDepth,
                              dst, dstLevel, dstDepth);
}

} // namespace gx2
