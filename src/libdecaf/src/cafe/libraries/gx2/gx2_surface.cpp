#include "gx2.h"
#include "gx2_addrlib.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gx2_cbpool.h"
#include "gx2_surface.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <common/align.h>
#include <common/log.h>
#include <common/pow.h>
#include <gsl.h>
#include <libgpu/gpu_tiling.h>
#include <libgpu/latte/latte_enum_sq.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;

static uint32_t
calcNumLevelsForSize(uint32_t size)
{
   return 32 - clz(size);
}

static uint32_t
calcNumLevels(virt_ptr<GX2Surface> surface)
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

static void
computeAuxInfo(virt_ptr<GX2ColorBuffer> colorBuffer,
               virt_ptr<uint32_t> outSize,
               virt_ptr<uint32_t> outAlignment,
               bool updateRegisters)
{
   // TODO: Implement once we have AddrComputeCmaskInfo, AddrComputeFmaskInfo
   if (outSize) {
      *outSize = 2048u;
   }
   if (outAlignment) {
      *outAlignment = 2048u;
   }
}

void
GX2CalcSurfaceSizeAndAlignment(virt_ptr<GX2Surface> surface)
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

   surface->mipLevelOffset[0] = 0u;
   surface->swizzle &= 0xFF00FFFF;

   if (surface->tileMode >= GX2TileMode::Tiled2DThin1 && surface->tileMode != GX2TileMode::LinearSpecial) {
      surface->swizzle |= 0xD0000;
   }

   auto lastTileMode = static_cast<GX2TileMode>(surface->tileMode);
   auto prevSize = 0u;
   auto offset0 = 0u;

   for (auto level = 0u; level < surface->mipLevels; ++level) {
      internal::getSurfaceInfo(surface.get(), level, &output);

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

            internal::getSurfaceInfo(surface.get(), level, &output);
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
      surface->mipmapSize = 0u;
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
GX2CalcDepthBufferHiZInfo(virt_ptr<GX2DepthBuffer> depthBuffer,
                          virt_ptr<uint32_t> outSize,
                          virt_ptr<uint32_t> outAlignment)
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
GX2CalcColorBufferAuxInfo(virt_ptr<GX2ColorBuffer> colorBuffer,
                          virt_ptr<uint32_t> outSize,
                          virt_ptr<uint32_t> outAlignment)
{
   decaf_warn_stub();
   computeAuxInfo(colorBuffer, outSize, outAlignment, false);
   colorBuffer->aaSize = *outSize;
}

void
GX2SetColorBuffer(virt_ptr<GX2ColorBuffer> colorBuffer,
                  GX2RenderTarget target)
{
   using latte::Register;
   auto reg = [](unsigned id) { return static_cast<Register>(id); };
   auto cb_color_info = colorBuffer->regs.cb_color_info.value();
   auto cb_color_mask = colorBuffer->regs.cb_color_mask.value();
   auto cb_color_size = colorBuffer->regs.cb_color_size.value();
   auto cb_color_view = colorBuffer->regs.cb_color_view.value();

   auto addr = static_cast<uint32_t>(
      OSEffectiveToPhysical(virt_cast<virt_addr>(colorBuffer->surface.image)));
   auto addrTile = 0u;
   auto addrFrag = 0u;

   if (colorBuffer->viewMip) {
      addr = static_cast<uint32_t>(
         OSEffectiveToPhysical(virt_cast<virt_addr>(colorBuffer->surface.mipmaps)));

      if (colorBuffer->viewMip > 1) {
         addr += colorBuffer->surface.mipLevelOffset[colorBuffer->viewMip - 1];
      }
   }

   if (colorBuffer->surface.tileMode >= GX2TileMode::Tiled2DThin1 &&
       colorBuffer->surface.tileMode != GX2TileMode::LinearSpecial) {
      if (colorBuffer->viewMip < ((colorBuffer->surface.swizzle >> 16) & 0xFF)) {
         addr ^= colorBuffer->surface.swizzle & 0xFFFF;
      }
   }

   if (colorBuffer->surface.aa) {
      addrFrag = static_cast<uint32_t>(
         OSEffectiveToPhysical(virt_cast<virt_addr>(colorBuffer->aaBuffer)));
      addrTile = addrFrag + colorBuffer->regs.cmask_offset;
   }

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_BASE + target * 4),
      addr >> 8
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_SIZE + target * 4),
      cb_color_size.value
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_INFO + target * 4),
      cb_color_info.value
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_TILE + target * 4),
      addrTile >> 8
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_FRAG + target * 4),
      addrFrag >> 8
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_VIEW + target * 4),
      cb_color_view.value
   });

   internal::writePM4(latte::pm4::SetContextReg {
      reg(Register::CB_COLOR0_MASK + target * 4),
      cb_color_mask.value
   });
}

void
GX2SetDepthBuffer(virt_ptr<GX2DepthBuffer> depthBuffer)
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
   internal::writePM4(latte::pm4::SetContextRegs { latte::Register::DB_DEPTH_SIZE, gsl::make_span(values1) });

   auto addr = OSEffectiveToPhysical(virt_cast<virt_addr>(depthBuffer->surface.image));
   auto addrHiZ = OSEffectiveToPhysical(virt_cast<virt_addr>(depthBuffer->hiZPtr));

   if (depthBuffer->viewMip) {
      addr = OSEffectiveToPhysical(virt_cast<virt_addr>(depthBuffer->surface.mipmaps));

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
   internal::writePM4(latte::pm4::SetContextRegs { latte::Register::DB_DEPTH_BASE, gsl::make_span(values2) });

   internal::writePM4(latte::pm4::SetContextReg { latte::Register::DB_HTILE_SURFACE, db_htile_surface.value });
   internal::writePM4(latte::pm4::SetContextReg { latte::Register::DB_PREFETCH_LIMIT, db_prefetch_limit.value });
   internal::writePM4(latte::pm4::SetContextReg { latte::Register::DB_PRELOAD_CONTROL, db_preload_control.value });
   internal::writePM4(latte::pm4::SetContextReg { latte::Register::PA_SU_POLY_OFFSET_DB_FMT_CNTL, pa_poly_offset_cntl.value });

   uint32_t values3[] = {
      depthBuffer->stencilClear,
      bit_cast<uint32_t, float>(depthBuffer->depthClear),
   };
   internal::writePM4(latte::pm4::SetContextRegs { latte::Register::DB_STENCIL_CLEAR, gsl::make_span(values3) });
}

void
GX2InitColorBufferRegs(virt_ptr<GX2ColorBuffer> colorBuffer)
{
   auto surfaceFormat = colorBuffer->surface.format;
   auto surfaceFormatType = internal::getSurfaceFormatType(surfaceFormat);

   auto output = ADDR_COMPUTE_SURFACE_INFO_OUTPUT { };
   internal::getSurfaceInfo(virt_addrof(colorBuffer->surface).get(),
                            colorBuffer->viewMip,
                            &output);

   // cb_color_size
   auto pitchTileMax = (output.pitch / latte::MicroTileWidth) - 1;
   auto sliceTileMax = ((output.pitch * output.height) / (latte::MicroTileWidth * latte::MicroTileHeight)) - 1;

   colorBuffer->regs.cb_color_size = latte::CB_COLORN_SIZE::get(0)
      .PITCH_TILE_MAX(pitchTileMax)
      .SLICE_TILE_MAX(sliceTileMax);

   // cb_color_info
   auto cbFormat = internal::getSurfaceFormatColorFormat(colorBuffer->surface.format);
   auto cbCompSwap = latte::CB_COMP_SWAP::STD;
   if (cbFormat == latte::CB_FORMAT::COLOR_5_5_5_1 ||
       cbFormat == latte::CB_FORMAT::COLOR_10_10_10_2) {
      cbCompSwap = latte::CB_COMP_SWAP::STD_REV;
   }

   auto cbNumberType = internal::getSurfaceFormatColorNumberType(surfaceFormat);
   auto cbTileMode = latte::CB_TILE_MODE::DISABLE;
   if (colorBuffer->surface.aa) {
      cbTileMode = latte::CB_TILE_MODE::FRAG_ENABLE;
   }

   auto blendBypass = false;
   if (surfaceFormatType == GX2SurfaceFormatType::UINT ||
       surfaceFormat == GX2SurfaceFormat::UNORM_R24_X8 ||
       surfaceFormat == GX2SurfaceFormat::FLOAT_D24_S8 ||
       surfaceFormat == GX2SurfaceFormat::FLOAT_X8_X24) {
      blendBypass = true;
   }

   auto cbRoundMode = latte::CB_ROUND_MODE::BY_HALF;
   if (surfaceFormatType == GX2SurfaceFormatType::FLOAT) {
      cbRoundMode = latte::CB_ROUND_MODE::TRUNCATE;
   }

   auto cbSourceFormat = internal::getSurfaceFormatColorSourceFormat(surfaceFormat);
   if (surfaceFormatType == GX2SurfaceFormatType::UINT) {
      cbSourceFormat = latte::CB_SOURCE_FORMAT::EXPORT_NORM;
   }

   colorBuffer->regs.cb_color_info = latte::CB_COLORN_INFO::get(0)
      .ENDIAN(internal::getSurfaceFormatColorEndian(surfaceFormat))
      .FORMAT(cbFormat)
      .NUMBER_TYPE(cbNumberType)
      .ARRAY_MODE(static_cast<latte::BUFFER_ARRAY_MODE>(output.tileMode))
      .COMP_SWAP(cbCompSwap)
      .TILE_MODE(cbTileMode)
      .BLEND_BYPASS(blendBypass)
      .ROUND_MODE(cbRoundMode)
      .SOURCE_FORMAT(cbSourceFormat);

   colorBuffer->regs.cb_color_mask = latte::CB_COLORN_MASK::get(0);

   if (colorBuffer->surface.tileMode == GX2TileMode::LinearSpecial) {
      colorBuffer->regs.cb_color_view = latte::CB_COLORN_VIEW::get(0);
   } else {
      colorBuffer->regs.cb_color_view = latte::CB_COLORN_VIEW::get(0)
         .SLICE_START(colorBuffer->viewFirstSlice)
         .SLICE_MAX(colorBuffer->viewFirstSlice + colorBuffer->viewNumSlices - 1);
   }

   if (colorBuffer->surface.aa) {
      computeAuxInfo(colorBuffer, nullptr, nullptr, true);
   }
}

void
GX2InitDepthBufferRegs(virt_ptr<GX2DepthBuffer> depthBuffer)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT surfaceInfo;
   internal::getSurfaceInfo(virt_addrof(depthBuffer->surface).get(),
                            depthBuffer->viewMip,
                            &surfaceInfo);

   // db_depth_size
   auto pitchTileMax = (surfaceInfo.pitch / latte::MicroTileWidth) - 1;
   auto sliceTileMax = ((surfaceInfo.pitch * surfaceInfo.height) / (latte::MicroTileWidth * latte::MicroTileHeight)) - 1;

   depthBuffer->regs.db_depth_size = latte::DB_DEPTH_SIZE::get(0)
      .PITCH_TILE_MAX(pitchTileMax)
      .SLICE_TILE_MAX(sliceTileMax);

   // db_depth_view
   depthBuffer->regs.db_depth_view = latte::DB_DEPTH_VIEW::get(0)
      .SLICE_START(depthBuffer->viewFirstSlice)
      .SLICE_MAX(depthBuffer->viewFirstSlice + depthBuffer->viewNumSlices - 1);

   depthBuffer->regs.db_htile_surface = latte::DB_HTILE_SURFACE::get(0)
      .HTILE_WIDTH(true)
      .HTILE_HEIGHT(true)
      .FULL_CACHE(true);

   depthBuffer->regs.db_prefetch_limit = latte::DB_PREFETCH_LIMIT::get(0)
      .DEPTH_HEIGHT_TILE_MAX(((depthBuffer->surface.height / 8) - 1) & 0x3FF);

   depthBuffer->regs.db_preload_control = latte::DB_PRELOAD_CONTROL::get(0)
      .MAX_X(depthBuffer->surface.width / 32)
      .MAX_Y(depthBuffer->surface.height / 32);

   auto db_depth_info = latte::DB_DEPTH_INFO::get(0)
      .READ_SIZE(latte::BUFFER_READ_SIZE::READ_512_BITS)
      .ARRAY_MODE(static_cast<latte::BUFFER_ARRAY_MODE>(surfaceInfo.tileMode))
      .TILE_SURFACE_ENABLE(depthBuffer->hiZPtr != nullptr);

   auto pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0);

   switch (depthBuffer->surface.format) {
   case GX2SurfaceFormat::UNORM_R16:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_16);
      pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0)
         .POLY_OFFSET_NEG_NUM_DB_BITS(240);
      break;
   case GX2SurfaceFormat::UNORM_R24_X8:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_8_24);
      pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0)
         .POLY_OFFSET_NEG_NUM_DB_BITS(232);
      break;
   case GX2SurfaceFormat::FLOAT_R32:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_32_FLOAT);
      pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0)
         .POLY_OFFSET_NEG_NUM_DB_BITS(233)
         .POLY_OFFSET_DB_IS_FLOAT_FMT(true);
      break;
   case GX2SurfaceFormat::FLOAT_D24_S8:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_8_24_FLOAT);
      pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0)
         .POLY_OFFSET_NEG_NUM_DB_BITS(236)
         .POLY_OFFSET_DB_IS_FLOAT_FMT(true);
      break;
   case GX2SurfaceFormat::FLOAT_X8_X24:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_X24_8_32_FLOAT);
      pa_poly_offset_cntl = latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL::get(0)
         .POLY_OFFSET_NEG_NUM_DB_BITS(233)
         .POLY_OFFSET_DB_IS_FLOAT_FMT(true);
      break;
   default:
      db_depth_info = db_depth_info
         .FORMAT(latte::DB_FORMAT::DEPTH_INVALID);
   }

   depthBuffer->regs.db_depth_info = db_depth_info;
   depthBuffer->regs.pa_poly_offset_cntl = pa_poly_offset_cntl;
}

void
GX2InitDepthBufferHiZEnable(virt_ptr<GX2DepthBuffer> depthBuffer,
                            BOOL enable)
{
   auto db_depth_info = depthBuffer->regs.db_depth_info.value();

   db_depth_info = db_depth_info
      .TILE_SURFACE_ENABLE(!!enable);

   depthBuffer->regs.db_depth_info = db_depth_info;
}

uint32_t
GX2GetSurfaceSwizzle(virt_ptr<GX2Surface> surface)
{
   return (surface->swizzle >> 8) & 0xff;
}

uint32_t
GX2GetSurfaceSwizzleOffset(virt_ptr<GX2Surface> surface,
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
GX2SetSurfaceSwizzle(virt_ptr<GX2Surface> surface,
                     uint32_t swizzle)
{
   surface->swizzle &= 0xFFFF00FF;
   surface->swizzle |= swizzle << 8;
}

uint32_t
GX2GetSurfaceMipPitch(virt_ptr<GX2Surface> surface,
                      uint32_t level)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   internal::getSurfaceInfo(surface.get(), level, &info);
   return info.pitch;
}

uint32_t
GX2GetSurfaceMipSliceSize(virt_ptr<GX2Surface> surface,
                          uint32_t level)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   internal::getSurfaceInfo(surface.get(), level, &info);
   return internal::calcSliceSize(surface.get(), &info);
}

void
GX2CopySurface(virt_ptr<GX2Surface> src,
               uint32_t srcLevel,
               uint32_t srcSlice,
               virt_ptr<GX2Surface> dst,
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
      gx2::internal::copySurface(src.get(), srcLevel, srcSlice,
                                 dst.get(), dstLevel, dstSlice);
      return;
   }

   auto dstTileType = latte::SQ_TILE_TYPE::DEFAULT;
   if (dst->use & GX2SurfaceUse::DepthBuffer) {
      dstTileType = latte::SQ_TILE_TYPE::DEPTH;
   }

   auto dstDim = static_cast<latte::SQ_TEX_DIM>(dst->dim.value());
   auto dstFormat = static_cast<latte::SQ_DATA_FORMAT>(dst->format & 0x3f);
   auto dstTileMode = static_cast<latte::SQ_TILE_MODE>(dst->tileMode.value());
   auto dstFormatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto dstNumFormat = latte::SQ_NUM_FORMAT::NORM;
   auto dstForceDegamma = false;
   auto dstPitch = dst->pitch;
   auto dstDepth = dst->depth;
   auto dstSamples = 0u;

   if (dst->format & GX2AttribFormatFlags::SIGNED) {
      dstFormatComp = latte::SQ_FORMAT_COMP::SIGNED;
   }

   if (dst->format & GX2AttribFormatFlags::SCALED) {
      dstNumFormat = latte::SQ_NUM_FORMAT::SCALED;
   } else if (dst->format & GX2AttribFormatFlags::INTEGER) {
      dstNumFormat = latte::SQ_NUM_FORMAT::INT;
   }

   if (dst->format & GX2AttribFormatFlags::DEGAMMA) {
      dstForceDegamma = true;
   }

   if (dstFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dstFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      dstPitch *= 4;
   }

   if (dstDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      dstDepth /= 6;
   }

   if (dst->aa == GX2AAMode::Mode2X) {
      dstSamples = 2;
   } else if (dst->aa == GX2AAMode::Mode4X) {
      dstSamples = 4;
   } else if (dst->aa == GX2AAMode::Mode8X) {
      dstSamples = 8;
   }

   auto srcTileType = latte::SQ_TILE_TYPE::DEFAULT;
   if (src->use & GX2SurfaceUse::DepthBuffer) {
      srcTileType = latte::SQ_TILE_TYPE::DEPTH;
   }

   auto srcDim = static_cast<latte::SQ_TEX_DIM>(src->dim.value());
   auto srcFormat = static_cast<latte::SQ_DATA_FORMAT>(src->format & 0x3f);
   auto srcTileMode = static_cast<latte::SQ_TILE_MODE>(src->tileMode.value());
   auto srcFormatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto srcNumFormat = latte::SQ_NUM_FORMAT::NORM;
   auto srcForceDegamma = false;
   auto srcPitch = src->pitch;
   auto srcDepth = src->depth;
   auto srcSamples = 0u;

   if (src->format & GX2AttribFormatFlags::SIGNED) {
      srcFormatComp = latte::SQ_FORMAT_COMP::SIGNED;
   }

   if (src->format & GX2AttribFormatFlags::SCALED) {
      srcNumFormat = latte::SQ_NUM_FORMAT::SCALED;
   } else if (src->format & GX2AttribFormatFlags::INTEGER) {
      srcNumFormat = latte::SQ_NUM_FORMAT::INT;
   }

   if (src->format & GX2AttribFormatFlags::DEGAMMA) {
      srcForceDegamma = true;
   }

   if (srcFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && srcFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      srcPitch *= 4;
   }

   if (srcDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      srcDepth /= 6;
   }

   if (src->aa == GX2AAMode::Mode2X) {
      srcSamples = 2;
   } else if (src->aa == GX2AAMode::Mode4X) {
      srcSamples = 4;
   } else if (src->aa == GX2AAMode::Mode8X) {
      srcSamples = 8;
   }

   internal::writePM4(latte::pm4::DecafCopySurface {
      OSEffectiveToPhysical(virt_cast<virt_addr>(dst->image)),
      OSEffectiveToPhysical(virt_cast<virt_addr>(dst->mipmaps)),
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
      dstTileType,
      dstTileMode,
      OSEffectiveToPhysical(virt_cast<virt_addr>(src->image)),
      OSEffectiveToPhysical(virt_cast<virt_addr>(src->mipmaps)),
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
      srcTileType,
      srcTileMode
   });
}

void
GX2ExpandColorBuffer(virt_ptr<GX2ColorBuffer> buffer)
{
   // TODO: GX2ExpandColorBuffer
   decaf_warn_stub();
}

void
GX2ExpandDepthBuffer(virt_ptr<GX2DepthBuffer> buffer)
{
   // TODO: GX2ExpandDepthBuffer
   decaf_warn_stub();
}

void
GX2ResolveAAColorBuffer(virt_ptr<GX2ColorBuffer> src,
                        virt_ptr<GX2Surface> dst,
                        uint32_t dstLevel,
                        uint32_t dstSlice)
{
   if (src->surface.format == GX2SurfaceFormat::INVALID || src->surface.width == 0 || src->surface.height == 0) {
      return;
   }

   if (dst->format == GX2SurfaceFormat::INVALID) {
      return;
   }

   auto dstTileType = latte::SQ_TILE_TYPE::DEFAULT;
   auto dstDim = static_cast<latte::SQ_TEX_DIM>(dst->dim.value());
   auto dstFormat = static_cast<latte::SQ_DATA_FORMAT>(dst->format & 0x3f);
   auto dstTileMode = static_cast<latte::SQ_TILE_MODE>(dst->tileMode.value());
   auto dstFormatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto dstNumFormat = latte::SQ_NUM_FORMAT::NORM;
   auto dstForceDegamma = false;
   auto dstPitch = dst->pitch;
   auto dstDepth = dst->depth;
   auto dstSamples = 0u;

   if (dst->format & GX2AttribFormatFlags::SIGNED) {
      dstFormatComp = latte::SQ_FORMAT_COMP::SIGNED;
   }

   if (dst->format & GX2AttribFormatFlags::SCALED) {
      dstNumFormat = latte::SQ_NUM_FORMAT::SCALED;
   } else if (dst->format & GX2AttribFormatFlags::INTEGER) {
      dstNumFormat = latte::SQ_NUM_FORMAT::INT;
   }

   if (dst->format & GX2AttribFormatFlags::DEGAMMA) {
      dstForceDegamma = true;
   }

   if (dstFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dstFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      dstPitch *= 4;
   }

   if (dstDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      dstDepth /= 6;
   }

   if (dst->aa == GX2AAMode::Mode2X) {
      dstSamples = 2;
   } else if (dst->aa == GX2AAMode::Mode4X) {
      dstSamples = 4;
   } else if (dst->aa == GX2AAMode::Mode8X) {
      dstSamples = 8;
   }

   auto srcTileType = latte::SQ_TILE_TYPE::DEFAULT;
   auto srcDim = static_cast<latte::SQ_TEX_DIM>(src->surface.dim.value());
   auto srcFormat = static_cast<latte::SQ_DATA_FORMAT>(src->surface.format & 0x3f);
   auto srcTileMode = static_cast<latte::SQ_TILE_MODE>(src->surface.tileMode.value());
   auto srcFormatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto srcNumFormat = latte::SQ_NUM_FORMAT::NORM;
   auto srcForceDegamma = false;
   auto srcPitch = src->surface.pitch;
   auto srcDepth = src->surface.depth;
   auto srcSamples = 0u;

   if (src->surface.format & GX2AttribFormatFlags::SIGNED) {
      srcFormatComp = latte::SQ_FORMAT_COMP::SIGNED;
   }

   if (src->surface.format & GX2AttribFormatFlags::SCALED) {
      srcNumFormat = latte::SQ_NUM_FORMAT::SCALED;
   } else if (src->surface.format & GX2AttribFormatFlags::INTEGER) {
      srcNumFormat = latte::SQ_NUM_FORMAT::INT;
   }

   if (src->surface.format & GX2AttribFormatFlags::DEGAMMA) {
      srcForceDegamma = true;
   }

   if (srcFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && srcFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      srcPitch *= 4;
   }

   if (srcDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      srcDepth /= 6;
   }

   if (src->surface.aa == GX2AAMode::Mode2X) {
      srcSamples = 2;
   } else if (src->surface.aa == GX2AAMode::Mode4X) {
      srcSamples = 4;
   } else if (src->surface.aa == GX2AAMode::Mode8X) {
      srcSamples = 8;
   }

   internal::writePM4(latte::pm4::DecafExpandColorBuffer {
      OSEffectiveToPhysical(virt_cast<virt_addr>(dst->image)),
      OSEffectiveToPhysical(virt_cast<virt_addr>(dst->mipmaps)),
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
      dstTileType,
      dstTileMode,
      OSEffectiveToPhysical(virt_cast<virt_addr>(src->surface.image)),
      OSEffectiveToPhysical(virt_cast<virt_addr>(src->aaBuffer)),
      OSEffectiveToPhysical(virt_cast<virt_addr>(src->surface.mipmaps)),
      src->viewMip,
      src->viewFirstSlice,
      srcPitch,
      src->surface.width,
      src->surface.height,
      srcDepth,
      srcSamples,
      srcDim,
      srcFormat,
      srcNumFormat,
      srcFormatComp,
      srcForceDegamma ? 1u : 0u,
      srcTileType,
      srcTileMode,
      src->viewNumSlices
   });
}

void
Library::registerSurfaceSymbols()
{
   RegisterFunctionExport(GX2InitDepthBufferHiZEnable);
   RegisterFunctionExport(GX2InitDepthBufferRegs);
   RegisterFunctionExport(GX2InitColorBufferRegs);
   RegisterFunctionExport(GX2SetDepthBuffer);
   RegisterFunctionExport(GX2SetColorBuffer);
   RegisterFunctionExport(GX2CalcColorBufferAuxInfo);
   RegisterFunctionExport(GX2CalcDepthBufferHiZInfo);
   RegisterFunctionExport(GX2CalcSurfaceSizeAndAlignment);
   RegisterFunctionExport(GX2GetSurfaceSwizzle);
   RegisterFunctionExport(GX2GetSurfaceSwizzleOffset);
   RegisterFunctionExport(GX2SetSurfaceSwizzle);
   RegisterFunctionExport(GX2GetSurfaceMipPitch);
   RegisterFunctionExport(GX2GetSurfaceMipSliceSize);
   RegisterFunctionExport(GX2CopySurface);
   RegisterFunctionExport(GX2ExpandColorBuffer);
   RegisterFunctionExport(GX2ExpandDepthBuffer);
   RegisterFunctionExport(GX2ResolveAAColorBuffer);
}

} // namespace cafe::gx2
