#include "gx2.h"
#include "gx2_debug.h"
#include "gx2_format.h"
#include "gx2_internal_cbpool.h"
#include "gx2_texture.h"

#include <common/decaf_assert.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

void
GX2InitTextureRegs(virt_ptr<GX2Texture> texture)
{
   auto word0 = latte::SQ_TEX_RESOURCE_WORD0_N::get(0);
   auto word1 = latte::SQ_TEX_RESOURCE_WORD1_N::get(0);
   auto word4 = latte::SQ_TEX_RESOURCE_WORD4_N::get(0);
   auto word5 = texture->regs.word5.value();
   auto word6 = texture->regs.word6.value();

   // Minimum values
   if (!texture->viewNumMips) {
      texture->viewNumMips = 1u;
   }

   if (!texture->viewNumSlices) {
      texture->viewNumSlices = 1u;
   }

   if (!texture->surface.width) {
      texture->surface.width = 1u;
   }

   if (!texture->surface.height) {
      texture->surface.height = 1u;
   }

   if (!texture->surface.depth) {
      texture->surface.depth = 1u;
   }

   if (!texture->surface.mipLevels) {
      texture->surface.mipLevels = 1u;
   }

   // Word 0
   auto tileType = 0;
   auto format = static_cast<latte::SQ_DATA_FORMAT>(texture->surface.format & 0x3F);
   auto pitch = texture->surface.pitch;

   if (texture->surface.use & GX2SurfaceUse::DepthBuffer) {
      tileType = 1;
   }

   if (format >= latte::SQ_DATA_FORMAT::FMT_BC1 && format <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      pitch *= 4;
   }

   pitch = std::max<uint32_t>(pitch, 8u);

   word0 = word0
      .DIM(static_cast<latte::SQ_TEX_DIM>(texture->surface.dim & 0x7))
      .TILE_MODE(static_cast<latte::SQ_TILE_MODE>(texture->surface.tileMode.value()))
      .TILE_TYPE(tileType)
      .PITCH((pitch / 8) - 1)
      .TEX_WIDTH(texture->surface.width - 1);

   // Word 1
   auto depth = 0u;

   if (texture->surface.dim == GX2SurfaceDim::TextureCube) {
      depth = (texture->surface.depth / 6) - 1;
   } else if (texture->surface.dim == GX2SurfaceDim::Texture3D ||
              texture->surface.dim == GX2SurfaceDim::Texture2DMSAAArray ||
              texture->surface.dim == GX2SurfaceDim::Texture2DArray ||
              texture->surface.dim == GX2SurfaceDim::Texture1DArray) {
      depth = texture->surface.depth - 1;
   }

   word1 = word1
      .TEX_HEIGHT(texture->surface.height - 1)
      .TEX_DEPTH(depth)
      .DATA_FORMAT(format);

   // Word 4
   auto formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto numFormat = latte::SQ_NUM_FORMAT::NORM;
   auto forceDegamma = false;

   if (texture->surface.format & GX2AttribFormatFlags::SIGNED) {
      formatComp = latte::SQ_FORMAT_COMP::SIGNED;
   }

   if (texture->surface.format & GX2AttribFormatFlags::SCALED) {
      numFormat = latte::SQ_NUM_FORMAT::SCALED;
   } else if (texture->surface.format & GX2AttribFormatFlags::INTEGER) {
      numFormat = latte::SQ_NUM_FORMAT::INT;
   }

   if (texture->surface.format & GX2AttribFormatFlags::DEGAMMA) {
      forceDegamma = true;
   }

   auto endian = internal::getSurfaceFormatEndian(texture->surface.format);
   auto dstSelX = static_cast<latte::SQ_SEL>((texture->compMap >> 24) & 0x7);
   auto dstSelY = static_cast<latte::SQ_SEL>((texture->compMap >> 16) & 0x7);
   auto dstSelZ = static_cast<latte::SQ_SEL>((texture->compMap >> 8) & 0x7);
   auto dstSelW = static_cast<latte::SQ_SEL>(texture->compMap & 0x7);

   word4 = word4
      .FORMAT_COMP_X(formatComp)
      .FORMAT_COMP_Y(formatComp)
      .FORMAT_COMP_Z(formatComp)
      .FORMAT_COMP_W(formatComp)
      .NUM_FORMAT_ALL(numFormat)
      .FORCE_DEGAMMA(forceDegamma)
      .ENDIAN_SWAP(endian)
      .REQUEST_SIZE(2)
      .DST_SEL_X(dstSelX)
      .DST_SEL_Y(dstSelY)
      .DST_SEL_Z(dstSelZ)
      .DST_SEL_W(dstSelW)
      .BASE_LEVEL(texture->viewFirstMip);

   // Word 5
   auto yuvConv = 0u;

   if (texture->surface.dim == GX2SurfaceDim::TextureCube && word1.TEX_DEPTH()) {
      yuvConv = 1;
   }

   word5 = word5
      .LAST_LEVEL(texture->viewFirstMip + texture->viewNumMips - 1)
      .BASE_ARRAY(texture->viewFirstSlice)
      .LAST_ARRAY(texture->viewFirstSlice + texture->viewNumSlices - 1)
      .YUV_CONV(yuvConv);

   // For MSAA textures, we overwrite the LAST_LEVEL field
   if (texture->surface.aa) {
      decaf_check(texture->surface.dim == GX2SurfaceDim::Texture2DMSAA || texture->surface.dim == GX2SurfaceDim::Texture2DMSAAArray);

      if (texture->surface.aa == GX2AAMode::Mode2X) {
         word5 = word5
            .LAST_LEVEL(1);
      } else if (texture->surface.aa == GX2AAMode::Mode4X) {
         word5 = word5
            .LAST_LEVEL(2);
      } else if (texture->surface.aa == GX2AAMode::Mode8X) {
         word5 = word5
            .LAST_LEVEL(3);
      }
   }

   // Word 6
   word6 = word6
      .MAX_ANISO_RATIO(4)
      .PERF_MODULATION(7)
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_TEXTURE);

   // Update big endian register in texture
   texture->regs.word0 = word0;
   texture->regs.word1 = word1;
   texture->regs.word4 = word4;
   texture->regs.word5 = word5;
   texture->regs.word6 = word6;
}

static void
setTexture(virt_ptr<GX2Texture> texture,
           latte::SQ_RES_OFFSET offset,
           uint32_t unit)
{
   auto imageAddress = static_cast<uint32_t>(virt_cast<virt_addr>(texture->surface.image));
   auto mipAddress = static_cast<uint32_t>(virt_cast<virt_addr>(texture->surface.mipmaps));

   decaf_check(!(mipAddress & 0xff));
   decaf_check(!(imageAddress & 0xff));

   if (texture->surface.tileMode >= GX2TileMode::Tiled2DThin1 &&
       texture->surface.tileMode != GX2TileMode::LinearSpecial) {
      if ((texture->surface.swizzle >> 16) & 0xFF) {
         imageAddress ^= (texture->surface.swizzle & 0xFFFF);
         mipAddress ^= (texture->surface.swizzle & 0xFFFF);
      }
   }

   auto word2 = latte::SQ_TEX_RESOURCE_WORD2_N::get(imageAddress >> 8);
   auto word3 = latte::SQ_TEX_RESOURCE_WORD3_N::get(mipAddress >> 8);

   internal::writePM4(latte::pm4::SetTexResource {
      (offset + unit) * 7,
      texture->regs.word0,
      texture->regs.word1,
      word2,
      word3,
      texture->regs.word4,
      texture->regs.word5,
      texture->regs.word6,
   });

   internal::debugDumpTexture(texture);
}

void
GX2SetPixelTexture(virt_ptr<GX2Texture> texture,
                   uint32_t unit)
{
   setTexture(texture, latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0, unit);
}

void
GX2SetVertexTexture(virt_ptr<GX2Texture> texture,
                    uint32_t unit)
{
   setTexture(texture, latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0, unit);
}

void
GX2SetGeometryTexture(virt_ptr<GX2Texture> texture,
                      uint32_t unit)
{
   setTexture(texture, latte::SQ_RES_OFFSET::GS_TEX_RESOURCE_0, unit);
}

void
Library::registerTextureSymbols()
{
   RegisterFunctionExport(GX2InitTextureRegs);
   RegisterFunctionExport(GX2SetPixelTexture);
   RegisterFunctionExport(GX2SetVertexTexture);
   RegisterFunctionExport(GX2SetGeometryTexture);
}

} // namespace cafe::gx2
