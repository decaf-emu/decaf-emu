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
      throw unimplemented_error(fmt::format("Unimplemented SQ_TEX_DIM {}", dim));
   }
}

static GX2SurfaceFormat
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

   return static_cast<GX2SurfaceFormat>(value);
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
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
      return gl::GL_RGB10_A2;
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
      return gl::GL_RGBA8_SNORM;
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
      return gl::GL_RGBA8UI;
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
      return gl::GL_RGBA8I;
   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
      return gl::GL_SRGB8_ALPHA8;
   case GX2SurfaceFormat::SRGB_BC1:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
   case GX2SurfaceFormat::SRGB_BC2:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
   case GX2SurfaceFormat::SRGB_BC3:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
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
      throw unimplemented_error(fmt::format("Unimplemented texture storage format {}", value));
   }
}

static gl::GLenum
getTextureFormat(latte::SQ_DATA_FORMAT format)
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
   case latte::FMT_2_10_10_10:
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
      throw unimplemented_error(fmt::format("Unimplemented texture type format {}", format));
   }
}

static gl::GLenum
getTextureDataType(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp)
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
   case latte::FMT_2_10_10_10:
      return isSigned ? gl::GL_INT : gl::GL_UNSIGNED_INT;
   default:
      throw unimplemented_error(fmt::format("Unimplemented texture format {}", format));
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
getCompressedTextureDataType(latte::SQ_DATA_FORMAT format, uint32_t degamma)
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
      throw unimplemented_error(fmt::format("Unimplemented compressed texture format {}", format));
   }
}

static gl::GLenum
getTextureSwizzle(latte::SQ_SEL sel)
{
   switch (sel) {
   case latte::SQ_SEL_X:
      return gl::GL_RED;
   case latte::SQ_SEL_Y:
      return gl::GL_GREEN;
   case latte::SQ_SEL_Z:
      return gl::GL_BLUE;
   case latte::SQ_SEL_W:
      return gl::GL_ALPHA;
   case latte::SQ_SEL_0:
      return gl::GL_ZERO;
   case latte::SQ_SEL_1:
      return gl::GL_ONE;
   default:
      throw unimplemented_error(fmt::format("Unimplemented compressed texture swizzle {}", sel));
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

      // Untile texture
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

      if (!latte::untile(tiled, width, height, depth, pitch, format, tileMode, swizzle, untiled)) {
         gLog->error("Failed to untile texture.");
         return false;
      }

      // Create texture
      bool compressed = isCompressedFormat(format);
      auto target = getTextureTarget(dim);
      auto storageFormat = getStorageFormat(format, numFormat, formatComp, degamma);
      auto textureDataType = gl::GL_INVALID_ENUM;
      auto textureFormat = getTextureFormat(format);
      auto size = untiled.size();

      if (compressed) {
         textureDataType = getCompressedTextureDataType(format, degamma);
      } else {
         textureDataType = getTextureDataType(format, formatComp);
      }

      switch (dim) {
      case latte::SQ_TEX_DIM_2D:
         gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &texture.object);
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureStorage2D(texture.object, 1, storageFormat, width, height);

         if (compressed) {
            gl::glCompressedTextureSubImage2D(texture.object, 0,
                                              0, 0,
                                              width, height,
                                              textureDataType,
                                              gsl::narrow_cast<gl::GLsizei>(size), untiled.data());
         } else {
            gl::glTextureSubImage2D(texture.object, 0,
                                    0, 0,
                                    width, height,
                                    textureFormat, textureDataType,
                                    untiled.data());
         }
         break;
      case latte::SQ_TEX_DIM_2D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_2D_ARRAY, 1, &texture.object);
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureParameteri(texture.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
         gl::glTextureStorage3D(texture.object, 1, storageFormat, width, height, depth);

         if (compressed) {
            gl::glCompressedTextureSubImage3D(texture.object, 0,
                                              0, 0, 0,
                                              width, height, depth,
                                              textureDataType,
                                              gsl::narrow_cast<gl::GLsizei>(size), untiled.data());
         } else {
            gl::glTextureSubImage3D(texture.object, 0,
                                    0, 0, 0,
                                    width, height, depth,
                                    textureFormat, textureDataType,
                                    untiled.data());
         }
         break;
      case latte::SQ_TEX_DIM_1D:
      case latte::SQ_TEX_DIM_3D:
      case latte::SQ_TEX_DIM_CUBEMAP:
      case latte::SQ_TEX_DIM_1D_ARRAY:
      case latte::SQ_TEX_DIM_2D_MSAA:
      case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
         gLog->error("Unsupported texture dim: {}", sq_tex_resource_word0.DIM);
         return false;
      }

      // Setup texture swizzle
      auto dst_sel_x = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_X);
      auto dst_sel_y = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Y);
      auto dst_sel_z = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Z);
      auto dst_sel_w = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_W);

      gl::GLint textureSwizzle[] = {
         static_cast<gl::GLint>(dst_sel_x),
         static_cast<gl::GLint>(dst_sel_y),
         static_cast<gl::GLint>(dst_sel_z),
         static_cast<gl::GLint>(dst_sel_w),
      };

      gl::glTextureParameteriv(texture.object, gl::GL_TEXTURE_SWIZZLE_RGBA, textureSwizzle);
      gl::glBindTextureUnit(i, texture.object);
   }

   return true;
}

static gl::GLenum
getTextureWrap(latte::SQ_TEX_CLAMP clamp)
{
   switch (clamp) {
   case latte::SQ_TEX_WRAP:
      return gl::GL_REPEAT;
   case latte::SQ_TEX_MIRROR:
      return gl::GL_MIRRORED_REPEAT;
   case latte::SQ_TEX_CLAMP_LAST_TEXEL:
      return gl::GL_CLAMP_TO_EDGE;
   case latte::SQ_TEX_MIRROR_ONCE_LAST_TEXEL:
      return gl::GL_MIRROR_CLAMP_TO_EDGE;
   case latte::SQ_TEX_CLAMP_BORDER:
      return gl::GL_CLAMP_TO_BORDER;
   case latte::SQ_TEX_MIRROR_ONCE_BORDER:
      return gl::GL_MIRROR_CLAMP_TO_BORDER_EXT;
   case latte::SQ_TEX_CLAMP_HALF_BORDER:
   case latte::SQ_TEX_MIRROR_ONCE_HALF_BORDER:
   default:
      throw unimplemented_error(fmt::format("Unimplemented texture wrap {}", clamp));
   }
}

static gl::GLenum
getTextureXYFilter(latte::SQ_TEX_XY_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_XY_FILTER_POINT:
      return gl::GL_NEAREST;
   case latte::SQ_TEX_XY_FILTER_BILINEAR:
      return gl::GL_LINEAR;
   default:
      throw unimplemented_error(fmt::format("Unimplemented texture xy filter {}", filter));
   }
}

static gl::GLenum
getTextureCompareFunction(latte::SQ_TEX_DEPTH_COMPARE func)
{
   switch (func) {
   case latte::SQ_TEX_DEPTH_COMPARE_NEVER:
      return gl::GL_NEVER;
   case latte::SQ_TEX_DEPTH_COMPARE_LESS:
      return gl::GL_LESS;
   case latte::SQ_TEX_DEPTH_COMPARE_EQUAL:
      return gl::GL_EQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_LESSEQUAL:
      return gl::GL_LEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_GREATER:
      return gl::GL_GREATER;
   case latte::SQ_TEX_DEPTH_COMPARE_NOTEQUAL:
      return gl::GL_NOTEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_GREATEREQUAL:
      return gl::GL_GEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_ALWAYS:
      return gl::GL_ALWAYS;
   default:
      throw unimplemented_error(fmt::format("Unimplemented texture compare function {}", func));
   }
}

bool GLDriver::checkActiveSamplers()
{
   // TODO: Vertex Samplers, Geometry Samplers
   // Pixel samplers id 0...16
   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
      auto sq_tex_sampler_word0 = getRegister<latte::SQ_TEX_SAMPLER_WORD0_N>(latte::Register::SQ_TEX_SAMPLER_WORD0_0 + 4 * (i * 3));
      auto sq_tex_sampler_word1 = getRegister<latte::SQ_TEX_SAMPLER_WORD1_N>(latte::Register::SQ_TEX_SAMPLER_WORD1_0 + 4 * (i * 3));
      auto sq_tex_sampler_word2 = getRegister<latte::SQ_TEX_SAMPLER_WORD2_N>(latte::Register::SQ_TEX_SAMPLER_WORD2_0 + 4 * (i * 3));

      if (sq_tex_sampler_word0.value == 0 && sq_tex_sampler_word1.value == 0 && sq_tex_sampler_word2.value == 0) {
         gl::glBindSampler(i, 0);
         continue;
      }

      auto &sampler = mPixelSamplers[i];

      if (!sampler.object) {
         gl::glCreateSamplers(1, &sampler.object);
      }

      // Texture clamp
      auto clamp_x = getTextureWrap(sq_tex_sampler_word0.CLAMP_X);
      auto clamp_y = getTextureWrap(sq_tex_sampler_word0.CLAMP_Y);
      auto clamp_z = getTextureWrap(sq_tex_sampler_word0.CLAMP_Z);

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_S, static_cast<gl::GLint>(clamp_x));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_T, static_cast<gl::GLint>(clamp_y));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_R, static_cast<gl::GLint>(clamp_z));

      // Texture filter
      auto xy_min_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MIN_FILTER);
      auto xy_mag_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MAG_FILTER);

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<gl::GLint>(xy_min_filter));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<gl::GLint>(xy_mag_filter));

      // Setup border color
      auto border_color_type = sq_tex_sampler_word0.BORDER_COLOR_TYPE;
      std::array<float, 4> colors;

      switch (border_color_type) {
      case latte::SQ_TEX_BORDER_COLOR_TRANS_BLACK:
         colors = { 0.0f, 0.0f, 0.0f, 0.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_OPAQUE_BLACK:
         colors = { 0.0f, 0.0f, 0.0f, 1.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_OPAQUE_WHITE:
         colors = { 1.0f, 1.0f, 1.0f, 0.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_REGISTER:
         auto td_ps_sampler_border_red = getRegister<latte::TD_PS_SAMPLER_BORDERN_RED>(latte::Register::TD_PS_SAMPLER_BORDER0_RED + 4 * (i * 4));
         auto td_ps_sampler_border_green = getRegister<latte::TD_PS_SAMPLER_BORDERN_GREEN>(latte::Register::TD_PS_SAMPLER_BORDER0_GREEN + 4 * (i * 4));
         auto td_ps_sampler_border_blue = getRegister<latte::TD_PS_SAMPLER_BORDERN_BLUE>(latte::Register::TD_PS_SAMPLER_BORDER0_BLUE + 4 * (i * 4));
         auto td_ps_sampler_border_alpha = getRegister<latte::TD_PS_SAMPLER_BORDERN_ALPHA>(latte::Register::TD_PS_SAMPLER_BORDER0_ALPHA + 4 * (i * 4));

         colors = {
            td_ps_sampler_border_red.BORDER_RED,
            td_ps_sampler_border_green.BORDER_GREEN,
            td_ps_sampler_border_blue.BORDER_BLUE,
            td_ps_sampler_border_alpha.BORDER_ALPHA,
         };
         break;
      }

      gl::glSamplerParameterfv(sampler.object, gl::GL_TEXTURE_BORDER_COLOR, &colors[0]);

      // Depth compare
      auto depth_compare_function = getTextureCompareFunction(sq_tex_sampler_word0.DEPTH_COMPARE_FUNCTION);
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_FUNC, static_cast<gl::GLint>(depth_compare_function));

      // Setup texture LOD
      auto min_lod = sq_tex_sampler_word1.MIN_LOD;
      auto max_lod = sq_tex_sampler_word1.MAX_LOD;
      auto lod_bias = sq_tex_sampler_word1.LOD_BIAS;

      // TODO: GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD, GL_TEXTURE_LOD_BIAS

      // Bind sampler
      gl::glBindSampler(i, sampler.object);
   }

   return true;
}

} // namespace opengl

} // namespace gpu
