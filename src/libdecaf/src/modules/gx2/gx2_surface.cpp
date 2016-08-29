#include "gx2_addrlib.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gx2_surface.h"
#include "gpu/gpu_tiling.h"
#include "gpu/pm4_writer.h"
#include "common/align.h"
#include "common/log.h"
#include "common/pow.h"

namespace gx2
{

static uint32_t
calcNumLevelsForSize(uint32_t size)
{
   return 32 - clz(size);
}

static uint32_t
calcNumLevels(GX2Surface *surface)
{
   if (surface->mipLevels <= 1) {
      return 1;
   }

   auto levels = std::max(
      calcNumLevelsForSize(surface->width),
      calcNumLevelsForSize(surface->height));

   if (surface->dim == GX2SurfaceDim::Texture3D) {
      levels = std::max(levels, calcNumLevelsForSize(surface->depth));
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
            if (output.tileMode < ADDR_TM_2D_TILED_THIN1) {
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
   AddrComputeHtileInfo(gpu::getAddrLibHandle(), &input, &output);

   depthBuffer->hiZSize = gsl::narrow_cast<uint32_t>(output.htileBytes);

   if (outSize) {
      *outSize = depthBuffer->hiZSize;
   }

   if (outAlignment) {
      *outAlignment = output.baseAlign;
   }
}

void
GX2CalcColorBufferAuxInfo(GX2Surface *surface,
                          be_val<uint32_t> *outSize,
                          be_val<uint32_t> *outAlignment)
{
   gLog->warn("Application called GX2CalcColorBufferAuxInfo");
   *outSize = 2048;
   *outAlignment = 2048;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer,
                  GX2RenderTarget target)
{
   using latte::Register;
   auto reg = [](unsigned id) { return static_cast<Register>(id); };
   auto cb_color_info = colorBuffer->regs.cb_color_info.value();
   auto cb_color_mask = colorBuffer->regs.cb_color_mask.value();
   auto cb_color_size = colorBuffer->regs.cb_color_size.value();
   auto cb_color_view = colorBuffer->regs.cb_color_view.value();

   auto addr = colorBuffer->surface.image.getAddress();
   auto addrTile = 0u;
   auto addrFrag = 0u;

   if (colorBuffer->viewMip) {
      addr = colorBuffer->surface.mipmaps.getAddress();

      if (colorBuffer->viewMip > 1) {
         addr += colorBuffer->surface.mipLevelOffset[colorBuffer->viewMip - 1];
      }
   }

   if (colorBuffer->surface.tileMode >= GX2TileMode::Tiled2DThin1 && colorBuffer->surface.tileMode != GX2TileMode::LinearSpecial) {
      if (colorBuffer->viewMip < ((colorBuffer->surface.swizzle >> 16) & 0xFF)) {
         addr ^= colorBuffer->surface.swizzle & 0xFFFF;
      }
   }

   if (colorBuffer->surface.aa) {
      addrFrag = colorBuffer->aaBuffer.getAddress();
      addrTile = addrFrag + colorBuffer->regs.cmask_offset;
   }

   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_BASE + target * 4), addr >> 8 });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_SIZE + target * 4), cb_color_size.value });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_INFO + target * 4), cb_color_info.value });

   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_TILE + target * 4), addrTile >> 8 });
   pm4::write(pm4::SetContextReg { reg(Register::CB_COLOR0_FRAG + target * 4), addrFrag >> 8 });

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

   auto addr = depthBuffer->surface.image.getAddress();
   auto addrHiZ = depthBuffer->hiZPtr.getAddress();

   if (depthBuffer->viewMip) {
      addr = depthBuffer->surface.mipmaps.getAddress();

      if (depthBuffer->viewMip > 1) {
         addr += depthBuffer->surface.mipLevelOffset[depthBuffer->viewMip - 1];
      }
   }

   if (depthBuffer->surface.tileMode >= GX2TileMode::Tiled2DThin1 && depthBuffer->surface.tileMode != GX2TileMode::LinearSpecial) {
      if (depthBuffer->viewMip < ((depthBuffer->surface.swizzle >> 16) & 0xFF)) {
         addr ^= depthBuffer->surface.swizzle & 0xFFFF;
      }
   }

   uint32_t values2[] = {
      addr >> 8,
      db_depth_info.value,
      addrHiZ >> 8,
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

   // Update cb_color_info
   auto format = internal::getSurfaceFormatColorFormat(colorBuffer->surface.format);
   auto numberType = internal::getSurfaceFormatColorNumberType(colorBuffer->surface.format);

   cb_color_info = cb_color_info
      .FORMAT(format)
      .NUMBER_TYPE(numberType);

   // Update cb_color_size
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
   internal::getSurfaceInfo(&colorBuffer->surface, 0, &output);

   auto pitchTileMax = (output.pitch / latte::MicroTileWidth) - 1;
   auto sliceTileMax = ((output.pitch * output.height) / (latte::MicroTileWidth * latte::MicroTileHeight)) - 1;

   cb_color_size = cb_color_size
      .PITCH_TILE_MAX(pitchTileMax)
      .SLICE_TILE_MAX(sliceTileMax);

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

   // Update db_depth_info
   auto format = internal::getSurfaceFormatDepthFormat(depthBuffer->surface.format);

   db_depth_info = db_depth_info
      .FORMAT(format);

   // Update db_depth_size
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
   internal::getSurfaceInfo(&depthBuffer->surface, 0, &output);

   auto pitchTileMax = (output.pitch / latte::MicroTileWidth) - 1;
   auto sliceTileMax = ((output.pitch * output.height) / (latte::MicroTileWidth * latte::MicroTileHeight)) - 1;

   db_depth_size = db_depth_size
      .PITCH_TILE_MAX(pitchTileMax)
      .SLICE_TILE_MAX(sliceTileMax);

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
      .TILE_SURFACE_ENABLE(!!enable);

   depthBuffer->regs.db_depth_info = db_depth_info;
}

uint32_t
GX2GetSurfaceSwizzle(GX2Surface *surface)
{
   return (surface->swizzle >> 8) & 0xff;
}

uint32_t
GX2GetSurfaceSwizzleOffset(GX2Surface *surface,
                           uint32_t level)
{
   if (surface->tileMode < GX2TileMode::Tiled2DThin1 || surface->tileMode == GX2TileMode::LinearSpecial) {
      return 0;
   }

   if (level < ((surface->swizzle >> 16) & 0xFF)) {
      return 0;
   }

   return surface->swizzle & 0xFFFF;
}

void
GX2SetSurfaceSwizzle(GX2Surface *surface,
                     uint32_t swizzle)
{
   surface->swizzle &= 0xFFFF00FF;
   surface->swizzle |= swizzle << 8;
}

uint32_t
GX2GetSurfaceMipPitch(GX2Surface *surface,
                      uint32_t level)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   internal::getSurfaceInfo(surface, level, &info);
   return info.pitch;
}

uint32_t
GX2GetSurfaceMipSliceSize(GX2Surface *surface,
                          uint32_t level)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   internal::getSurfaceInfo(surface, level, &info);
   return internal::calcSliceSize(surface, &info);
}

void
GX2CopySurface(GX2Surface *src,
               uint32_t srcLevel,
               uint32_t srcSlice,
               GX2Surface *dst,
               uint32_t dstLevel,
               uint32_t dstSlice)
{
   if (src->format == GX2SurfaceFormat::INVALID || src->width == 0 || src->height == 0) {
      return;
   }

   if (dst->format == GX2SurfaceFormat::INVALID) {
      return;
   }

   if (src->tileMode == GX2TileMode::LinearSpecial ||
      dst->tileMode == GX2TileMode::LinearSpecial)
   {
      // LinearSpecial surfaces cause the copy to occur on the CPU.  This code
      //  assumes that if the texture was previously written by the GPU, that it
      //  has since been invalidated into CPU memory.
      gx2::internal::copySurface(src, srcLevel, srcSlice,
         dst, dstLevel, dstSlice);
      return;
   }

   auto dstDim = static_cast<latte::SQ_TEX_DIM>(dst->dim.value());
   auto dstFormat = static_cast<latte::SQ_DATA_FORMAT>(dst->format & 0x3f);
   auto dstTileMode = static_cast<latte::SQ_TILE_MODE>(dst->tileMode.value());
   auto dstFormatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
   auto dstNumFormat = latte::SQ_NUM_FORMAT_NORM;
   auto dstForceDegamma = false;
   auto dstPitch = dst->pitch;
   auto dstDepth = dst->depth;
   auto dstSamples = 0u;

   if (dst->format & GX2AttribFormatFlags::SIGNED) {
      dstFormatComp = latte::SQ_FORMAT_COMP_SIGNED;
   }

   if (dst->format & GX2AttribFormatFlags::SCALED) {
      dstNumFormat = latte::SQ_NUM_FORMAT_SCALED;
   } else if (dst->format & GX2AttribFormatFlags::INTEGER) {
      dstNumFormat = latte::SQ_NUM_FORMAT_INT;
   }

   if (dst->format & GX2AttribFormatFlags::DEGAMMA) {
      dstForceDegamma = true;
   }

   if (dstFormat >= latte::FMT_BC1 && dstFormat <= latte::FMT_BC5) {
      dstPitch *= 4;
   }

   if (dstDim == latte::SQ_TEX_DIM_CUBEMAP) {
      dstDepth /= 6;
   }

   if (dst->aa == GX2AAMode::Mode2X) {
      dstSamples = 2;
   } else if (dst->aa == GX2AAMode::Mode4X) {
      dstSamples = 4;
   } else if (dst->aa == GX2AAMode::Mode8X) {
      dstSamples = 8;
   }

   auto srcDim = static_cast<latte::SQ_TEX_DIM>(src->dim.value());
   auto srcFormat = static_cast<latte::SQ_DATA_FORMAT>(src->format & 0x3f);
   auto srcTileMode = static_cast<latte::SQ_TILE_MODE>(src->tileMode.value());
   auto srcFormatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
   auto srcNumFormat = latte::SQ_NUM_FORMAT_NORM;
   auto srcForceDegamma = false;
   auto srcPitch = src->pitch;
   auto srcDepth = src->depth;
   auto srcSamples = 0u;

   if (src->format & GX2AttribFormatFlags::SIGNED) {
      srcFormatComp = latte::SQ_FORMAT_COMP_SIGNED;
   }

   if (src->format & GX2AttribFormatFlags::SCALED) {
      srcNumFormat = latte::SQ_NUM_FORMAT_SCALED;
   } else if (src->format & GX2AttribFormatFlags::INTEGER) {
      srcNumFormat = latte::SQ_NUM_FORMAT_INT;
   }

   if (src->format & GX2AttribFormatFlags::DEGAMMA) {
      srcForceDegamma = true;
   }

   if (srcFormat >= latte::FMT_BC1 && srcFormat <= latte::FMT_BC5) {
      srcPitch *= 4;
   }

   if (srcDim == latte::SQ_TEX_DIM_CUBEMAP) {
      srcDepth /= 6;
   }

   if (src->aa == GX2AAMode::Mode2X) {
      srcSamples = 2;
   } else if (src->aa == GX2AAMode::Mode4X) {
      srcSamples = 4;
   } else if (src->aa == GX2AAMode::Mode8X) {
      srcSamples = 8;
   }

   pm4::write(pm4::DecafCopySurface{
      dst->image.getAddress(),
      dst->mipmaps.getAddress(),
      dstLevel,
      dstSlice,
      dstPitch,
      dst->width,
      dst->height,
      dstDepth,
      dstSamples,
      dstDim,
      dstFormat,
      dstNumFormat,
      dstFormatComp,
      dstForceDegamma ? 1u : 0u,
      dstTileMode,
      src->image.getAddress(),
      src->mipmaps.getAddress(),
      srcLevel,
      srcSlice,
      srcPitch,
      src->width,
      src->height,
      srcDepth,
      srcSamples,
      srcDim,
      srcFormat,
      srcNumFormat,
      srcFormatComp,
      srcForceDegamma ? 1u : 0u,
      srcTileMode
   });
}

void
GX2ExpandDepthBuffer(GX2DepthBuffer *buffer)
{
   // We do not implement HiZ, so no need to do anything here
}

} // namespace gx2
