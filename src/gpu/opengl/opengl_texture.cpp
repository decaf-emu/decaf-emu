#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>
#include "opengl_driver.h"
#include "gpu/latte_enum_sq.h"
#include "gpu/latte_format.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/log.h"

namespace gpu
{

namespace opengl
{

static gl::GLenum
getTextureTarget(latte::SQ_TEX_DIM dim)
{
   switch (dim)
   {
   case latte::SQ_TEX_DIM_1D:
      return gl::GL_TEXTURE_1D;
   case latte::SQ_TEX_DIM_2D:
      return gl::GL_TEXTURE_2D;
   case latte::SQ_TEX_DIM_3D:
      return gl::GL_TEXTURE_3D;
   case latte::SQ_TEX_DIM_CUBEMAP:
      return gl::GL_TEXTURE_CUBE_MAP;
   case latte::SQ_TEX_DIM_1D_ARRAY:
      return gl::GL_TEXTURE_1D_ARRAY;
   case latte::SQ_TEX_DIM_2D_ARRAY:
      return gl::GL_TEXTURE_2D_ARRAY;
   case latte::SQ_TEX_DIM_2D_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE;
   case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
   default:
      throw std::logic_error("Unexpected SQ_TEX_DIM");
      return gl::GL_TEXTURE_2D;
   }
}

static GX2SurfaceFormat::Value
getSurfaceFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   uint32_t value = format;

   if (numFormat == latte::SQ_NUM_FORMAT_SCALED) {
      value |= GX2AttribFormatFlags::SCALED;
   } else if (numFormat == latte::SQ_NUM_FORMAT_INT) {
      value |= GX2AttribFormatFlags::INTEGER;
   }

   if (formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
      value |= GX2AttribFormatFlags::SIGNED;
   }

   if (degamma) {
      value |= GX2AttribFormatFlags::DEGAMMA;
   }

   return static_cast<GX2SurfaceFormat::Value>(value);
}

static gl::GLenum
getStorageFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   // TODO: Not use gx2, but its fucking effort
   auto value = getSurfaceFormat(format, numFormat, formatComp, degamma);

   switch (value) {
   case GX2SurfaceFormat::UNORM_R8:
      return gl::GL_R8;
   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
      return gl::GL_RGBA8;
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
      return gl::GL_RGBA8_SNORM;
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
      return gl::GL_RGBA8UI;
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
      return gl::GL_RGBA8I;
   case GX2SurfaceFormat::UNORM_BC1:
      return gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   case GX2SurfaceFormat::UNORM_BC2:
      return gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   case GX2SurfaceFormat::UNORM_BC3:
      return gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
   case GX2SurfaceFormat::UNORM_BC4:
      return gl::GL_COMPRESSED_RED_RGTC1;
   case GX2SurfaceFormat::UNORM_BC5:
      return gl::GL_COMPRESSED_RG_RGTC2;
   default:
      throw std::logic_error("Invalid texture format");
      return gl::GL_INVALID_ENUM;
   }
}

static gl::GLenum
getTextureType(latte::SQ_DATA_FORMAT format)
{
   /*
   GL_RED, GL_RG, GL_RGB, GL_RGBA
   GL_BGR, GL_BGRA
   GL_DEPTH_COMPONENT, GL_STENCIL_INDEX
   */

   switch (format) {
   case latte::FMT_8:
      return gl::GL_RED;
   case latte::FMT_8_8_8_8:
      return gl::GL_RGBA;
   case latte::FMT_BC1:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
      return gl::GL_RGBA;
   case latte::FMT_BC4:
      return gl::GL_RED;
   case latte::FMT_BC5:
      return gl::GL_RG;
   default:
      throw std::logic_error("Invalid texture format");
      return gl::GL_INVALID_ENUM;
   }
}

static gl::GLenum
getTextureFormat(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp)
{
   /*
   GL_FLOAT,
   GL_BYTE, GL_UNSIGNED_BYTE,
   GL_SHORT, GL_UNSIGNED_SHORT,
   GL_INT, GL_UNSIGNED_INT,

   GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV,

   GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1,
   GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_SHORT_1_5_5_5_REV,

   GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_10_10_10_2,
   GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_2_10_10_10_REV
   */

   bool isSigned = (formatComp == latte::SQ_FORMAT_COMP_SIGNED);

   switch (format) {
   case latte::FMT_8:
   case latte::FMT_8_8_8_8:
      return isSigned ? gl::GL_BYTE : gl::GL_UNSIGNED_BYTE;
   default:
      throw std::logic_error("Invalid texture format");
      return gl::GL_INVALID_ENUM;
   }
}

static bool
isCompressedFormat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_BC1:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
   case latte::FMT_BC4:
   case latte::FMT_BC5:
      return true;
   default:
      return false;
   }
}

static gl::GLenum
getCompressedTextureFormat(latte::SQ_DATA_FORMAT format, uint32_t degamma)
{
   switch (format) {
   case latte::FMT_BC1:
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   case latte::FMT_BC2:
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   case latte::FMT_BC3:
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
   case latte::FMT_BC4:
      return degamma ? gl::GL_COMPRESSED_SIGNED_RED_RGTC1 : gl::GL_COMPRESSED_RED_RGTC1;
   case latte::FMT_BC5:
      return degamma ? gl::GL_COMPRESSED_SIGNED_RG_RGTC2 : gl::GL_COMPRESSED_RG_RGTC2;
   default:
      throw std::logic_error("Invalid compressed texture format");
      return gl::GL_INVALID_ENUM;
   }
}

bool GLDriver::checkActiveTextures()
{
   static const auto MAX_PS_TEXTURES = 16;
   std::vector<uint8_t> untiled;

   for (auto i = 0; i < MAX_PS_TEXTURES; ++i) {
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_TEX_RESOURCE_WORD1_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_TEX_RESOURCE_WORD2_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_TEX_RESOURCE_WORD3_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_TEX_RESOURCE_WORD5_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_TEX_RESOURCE_WORD6_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));

      if (!sq_tex_resource_word2.BASE_ADDRESS) {
         continue;
      }

      auto &texture = mTextures[sq_tex_resource_word2.BASE_ADDRESS];

      if (texture.object) {
         gl::glBindTextureUnit(i, texture.object);
         continue;
      }

      auto pitch = (sq_tex_resource_word0.PITCH + 1) * gsl::narrow_cast<uint32_t>(latte::tile_width);
      auto width = sq_tex_resource_word0.TEX_WIDTH + 1;
      auto height = sq_tex_resource_word1.TEX_HEIGHT + 1;
      auto depth = sq_tex_resource_word1.TEX_DEPTH + 1;

      auto format = sq_tex_resource_word1.DATA_FORMAT;
      auto tileMode = sq_tex_resource_word0.TILE_MODE;
      auto numFormat = sq_tex_resource_word4.NUM_FORMAT_ALL;
      auto formatComp = sq_tex_resource_word4.FORMAT_COMP_X;
      auto degamma = sq_tex_resource_word4.FORCE_DEGAMMA;
      auto dim = sq_tex_resource_word0.DIM;

      auto addr = (sq_tex_resource_word2.BASE_ADDRESS & (~7)) << 8;
      auto swizzle = sq_tex_resource_word2.SWIZZLE << 8;

      auto tiled = make_virtual_ptr<uint8_t>(addr);

      if (!latte::untile(tiled, width, height, pitch, format, tileMode, swizzle, untiled)) {
         gLog->error("Failed to untile texture.");
         return false;
      }

      bool compressed = isCompressedFormat(format);
      auto target = getTextureTarget(dim);
      auto storageFormat = getStorageFormat(format, numFormat, formatComp, degamma);
      auto textureType = getTextureType(format);
      auto textureFormat = gl::GL_INVALID_ENUM;
      auto size = untiled.size();

      if (compressed) {
         textureFormat = getCompressedTextureFormat(format, degamma);
      } else {
         textureFormat = getTextureFormat(format, formatComp);
      }

      switch (dim) {
      case latte::SQ_TEX_DIM_2D:
         gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &texture.object);
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureStorage2D(texture.object, 1, storageFormat, width, height);

         if (compressed) {
            gl::glCompressedTextureSubImage2D(texture.object, 0, 0, 0, width, height, textureFormat, gsl::narrow_cast<gl::GLsizei>(size), untiled.data());
         } else {
            gl::glTextureSubImage2D(texture.object, 0, 0, 0, width, height, textureFormat, textureType, untiled.data());
         }
         break;
      case latte::SQ_TEX_DIM_1D:
      case latte::SQ_TEX_DIM_3D:
      case latte::SQ_TEX_DIM_CUBEMAP:
      case latte::SQ_TEX_DIM_1D_ARRAY:
      case latte::SQ_TEX_DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM_2D_MSAA:
      case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
         gLog->error("Unsupported texture dim: {}", sq_tex_resource_word0.DIM);
         return false;
      }

      gl::glBindTextureUnit(i, texture.object);
   }

   return true;
}

} // namespace opengl

} // namespace gpu
