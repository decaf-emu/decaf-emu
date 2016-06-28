#include "common/emuassert.h"
#include "gx2_debug.h"
#include "gx2_format.h"
#include "gx2_texture.h"
#include "gpu/latte_registers.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

void
GX2InitTextureRegs(GX2Texture *texture)
{
   auto word0 = latte::SQ_TEX_RESOURCE_WORD0_N::get(0);
   auto word1 = latte::SQ_TEX_RESOURCE_WORD1_N::get(0);
   auto word4 = latte::SQ_TEX_RESOURCE_WORD4_N::get(0);
   auto word5 = texture->regs.word5.value();
   auto word6 = texture->regs.word6.value();

   // Minimum values
   if (!texture->viewNumMips) {
      texture->viewNumMips = 1;
   }

   if (!texture->viewNumSlices) {
      texture->viewNumSlices = 1;
   }

   if (!texture->surface.width) {
      texture->surface.width = 1;
   }

   if (!texture->surface.height) {
      texture->surface.height = 1;
   }

   if (!texture->surface.depth) {
      texture->surface.depth = 1;
   }

   if (!texture->surface.mipLevels) {
      texture->surface.mipLevels = 1;
   }

   // Word 0
   auto tileType = 0;
   auto format = static_cast<latte::SQ_DATA_FORMAT>(texture->surface.format & 0x3F);
   auto pitch = texture->surface.pitch;

   if (texture->surface.use & GX2SurfaceUse::DepthBuffer) {
      tileType = 1;
   }

   if (format >= latte::FMT_BC1 && format <= latte::FMT_BC5) {
      pitch *= 4;
   }

   word0 = word0
      .DIM().set(static_cast<latte::SQ_TEX_DIM>(texture->surface.dim & 0x7))
      .TILE_MODE().set(static_cast<latte::SQ_TILE_MODE>(texture->surface.tileMode.value()))
      .TILE_TYPE().set(tileType)
      .PITCH().set((pitch / 8) - 1)
      .TEX_WIDTH().set(texture->surface.width - 1);

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
      .TEX_HEIGHT().set(texture->surface.height - 1)
      .TEX_DEPTH().set(depth)
      .DATA_FORMAT().set(format);

   // Word 4
   auto formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
   auto numFormat = latte::SQ_NUM_FORMAT_NORM;
   auto forceDegamma = false;

   if (texture->surface.format & GX2AttribFormatFlags::SIGNED) {
      formatComp = latte::SQ_FORMAT_COMP_SIGNED;
   }

   if (texture->surface.format & GX2AttribFormatFlags::SCALED) {
      numFormat = latte::SQ_NUM_FORMAT_SCALED;
   } else if (texture->surface.format & GX2AttribFormatFlags::INTEGER) {
      numFormat = latte::SQ_NUM_FORMAT_INT;
   }

   if (texture->surface.format & GX2AttribFormatFlags::DEGAMMA) {
      forceDegamma = true;
   }

   auto endian = static_cast<latte::SQ_ENDIAN>(GX2GetSurfaceSwap(texture->surface.format));
   auto dstSelX = static_cast<latte::SQ_SEL>((texture->compMap >> 24) & 0x7);
   auto dstSelY = static_cast<latte::SQ_SEL>((texture->compMap >> 16) & 0x7);
   auto dstSelZ = static_cast<latte::SQ_SEL>((texture->compMap >> 8) & 0x7);
   auto dstSelW = static_cast<latte::SQ_SEL>(texture->compMap & 0x7);

   word4 = word4
      .FORMAT_COMP_X().set(formatComp)
      .FORMAT_COMP_Y().set(formatComp)
      .FORMAT_COMP_Z().set(formatComp)
      .FORMAT_COMP_W().set(formatComp)
      .NUM_FORMAT_ALL().set(numFormat)
      .FORCE_DEGAMMA().set(forceDegamma)
      .ENDIAN_SWAP().set(endian)
      .REQUEST_SIZE().set(2)
      .DST_SEL_X().set(dstSelX)
      .DST_SEL_Y().set(dstSelY)
      .DST_SEL_Z().set(dstSelZ)
      .DST_SEL_W().set(dstSelW)
      .BASE_LEVEL().set(texture->viewFirstMip);

   // Word 5
   auto yuvConv = 0u;

   if (texture->surface.dim == GX2SurfaceDim::TextureCube && word1.TEX_DEPTH()) {
      yuvConv = 1;
   }

   word5 = word5
      .LAST_LEVEL().set(texture->viewFirstMip + texture->viewNumMips - 1)
      .BASE_ARRAY().set(texture->viewFirstSlice)
      .LAST_ARRAY().set(texture->viewFirstSlice + texture->viewNumSlices - 1)
      .YUV_CONV().set(yuvConv);

   // Word 6
   word6 = word6
      .MAX_ANISO_RATIO().set(4)
      .PERF_MODULATION().set(7)
      .TYPE().set(latte::SQ_TEX_VTX_VALID_TEXTURE);

   // Update big endian register in texture
   texture->regs.word0 = word0;
   texture->regs.word1 = word1;
   texture->regs.word4 = word4;
   texture->regs.word5 = word5;
   texture->regs.word6 = word6;
}

static void
setTexture(GX2Texture *texture, latte::SQ_RES_OFFSET offset, uint32_t unit)
{
   auto imageAddress = texture->surface.image.getAddress();
   auto mipAddress = texture->surface.mipmaps.getAddress();

   emuassert(!(mipAddress & 0xff));
   emuassert(!(imageAddress & 0xff));

   if (texture->surface.tileMode >= GX2TileMode::Tiled2DThin1 && texture->surface.tileMode != GX2TileMode::LinearSpecial) {
      if ((texture->surface.swizzle >> 16) & 0xFF) {
         imageAddress ^= (texture->surface.swizzle & 0xFFFF);
         mipAddress ^= (texture->surface.swizzle & 0xFFFF);
      }
   }

   auto word2 = latte::SQ_TEX_RESOURCE_WORD2_N::get(imageAddress >> 8);
   auto word3 = latte::SQ_TEX_RESOURCE_WORD3_N::get(mipAddress >> 8);

   pm4::write(pm4::SetTexResource {
      (unit * 7) + offset,
      texture->regs.word0,
      texture->regs.word1,
      word2,
      word3,
      texture->regs.word4,
      texture->regs.word5,
      texture->regs.word6,
   });

   GX2DebugDumpTexture(texture);
}

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit)
{
   setTexture(texture, latte::SQ_PS_TEX_RESOURCE_0, unit);
}

void
GX2SetVertexTexture(GX2Texture *texture, uint32_t unit)
{
   setTexture(texture, latte::SQ_VS_TEX_RESOURCE_0, unit);
}

void
GX2SetGeometryTexture(GX2Texture *texture, uint32_t unit)
{
   setTexture(texture, latte::SQ_GS_TEX_RESOURCE_0, unit);
}

} // namespace gx2
