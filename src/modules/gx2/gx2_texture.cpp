#include "gx2_format.h"
#include "gx2_texture.h"
#include "gpu/latte_registers.h"
#include "gpu/pm4_writer.h"

void
GX2InitTextureRegs(GX2Texture *texture)
{
   // Minimum values
   if (!texture->viewNumMips) {
      texture->viewNumMips = 1;
   }

   if (!texture->viewNumSlices) {
      texture->viewNumSlices;
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
   texture->regWord0.value = 0;
   texture->regWord0.DIM = static_cast<latte::SQ_TEX_DIM>(texture->surface.dim & 0x7);
   texture->regWord0.TILE_MODE = texture->surface.tileMode;

   if (texture->surface.use & GX2SurfaceUse::DepthBuffer) {
      texture->regWord0.TILE_TYPE = 1;
   } else {
      texture->regWord0.TILE_TYPE = 0;
   }

   auto formatType = static_cast<latte::SQ_DATA_FORMAT>(texture->surface.format & latte::FMT_MASK);
   auto elemSize = 4u;

   if (formatType < latte::FMT_BC1 || formatType > latte::FMT_BC5) {
      elemSize = 1;
   }

   texture->regWord0.PITCH = ((elemSize * texture->surface.pitch) / 8) - 1;
   texture->regWord0.TEX_WIDTH = texture->surface.width - 1;

   // Word 1
   texture->regWord1.value = 0;
   texture->regWord1.TEX_HEIGHT = texture->surface.height - 1;

   if (texture->surface.dim == GX2SurfaceDim::TextureCube) {
      texture->regWord1.TEX_DEPTH = (texture->surface.depth / 6) - 1;
   } else if (texture->surface.dim == GX2SurfaceDim::Texture3D ||
              texture->surface.dim == GX2SurfaceDim::Texture2DMSAAArray ||
              texture->surface.dim == GX2SurfaceDim::Texture2DArray ||
              texture->surface.dim == GX2SurfaceDim::Texture1DArray) {
      texture->regWord1.TEX_DEPTH = texture->surface.depth - 1;
   } else {
      texture->regWord1.TEX_DEPTH = 0;
   }

   texture->regWord1.DATA_FORMAT = formatType;

   // Word 4
   auto formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;

   if (texture->surface.format & GX2AttribFormatFlags::SIGNED) {
      formatComp = latte::SQ_FORMAT_COMP_SIGNED;
   }

   texture->regWord4.value = 0;
   texture->regWord4.FORMAT_COMP_X = formatComp;
   texture->regWord4.FORMAT_COMP_Y = formatComp;
   texture->regWord4.FORMAT_COMP_Z = formatComp;
   texture->regWord4.FORMAT_COMP_W = formatComp;

   if (texture->surface.format & GX2AttribFormatFlags::SCALED) {
      texture->regWord4.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_SCALED;
   } else if (texture->surface.format & GX2AttribFormatFlags::SIGNED) {
      texture->regWord4.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_INT;
   }

   if (texture->surface.format & GX2AttribFormatFlags::DEGAMMA) {
      texture->regWord4.FORCE_DEGAMMA = 1;
   }

   // TODO: Setup endian swap based off texture->surface.format
   texture->regWord4.ENDIAN_SWAP = latte::SQ_ENDIAN_AUTO;

   texture->regWord4.REQUEST_SIZE = 2;

   texture->regWord4.DST_SEL_X = static_cast<latte::SQ_SEL>((texture->compMap >> 24) & 0x7);
   texture->regWord4.DST_SEL_Y = static_cast<latte::SQ_SEL>((texture->compMap >> 16) & 0x7);
   texture->regWord4.DST_SEL_Z = static_cast<latte::SQ_SEL>((texture->compMap >> 8) & 0x7);
   texture->regWord4.DST_SEL_W = static_cast<latte::SQ_SEL>(texture->compMap & 0x7);
   texture->regWord4.BASE_LEVEL = texture->viewFirstMip;

   // Word 5
   texture->regWord5.LAST_LEVEL = texture->viewFirstMip + texture->viewNumMips - 1;
   texture->regWord5.BASE_ARRAY = texture->viewFirstSlice;
   texture->regWord5.LAST_ARRAY = texture->viewFirstSlice + texture->viewNumSlices - 1;

   if (texture->surface.dim == GX2SurfaceDim::TextureCube && texture->regWord1.TEX_DEPTH) {
      texture->regWord5.YUV_CONV = 1;
   }

   // Word 6
   texture->regWord6.MAX_ANISO_RATIO = 4;
   texture->regWord6.PERF_MODULATION = 7;
   texture->regWord6.TYPE = latte::SQ_TEX_VTX_VALID_TEXTURE;
}

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit)
{
   pm4::write(pm4::SetResourceTexture {
      (unit * 7) + latte::Register::SQ_TEX_RESOURCE_WORD0_0,
      texture->regWord0,
      texture->regWord1,
      texture->surface.image,
      texture->surface.mipmaps,
      texture->regWord4,
      texture->regWord5,
      texture->regWord6,
   });
}
