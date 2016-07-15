#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/murmur3.h"
#include "gpu/latte_enum_sq.h"
#include "modules/gx2/gx2_addrlib.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

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
      decaf_abort(fmt::format("Unimplemented SQ_TEX_DIM {}", dim));
   }
}

static gx2::GX2SurfaceFormat
getSurfaceFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   uint32_t value = format;

   if (numFormat == latte::SQ_NUM_FORMAT_SCALED) {
      value |= gx2::GX2AttribFormatFlags::SCALED;
   } else if (numFormat == latte::SQ_NUM_FORMAT_INT) {
      value |= gx2::GX2AttribFormatFlags::INTEGER;
   }

   if (formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
      value |= gx2::GX2AttribFormatFlags::SIGNED;
   }

   if (degamma) {
      value |= gx2::GX2AttribFormatFlags::DEGAMMA;
   }

   return static_cast<gx2::GX2SurfaceFormat>(value);
}

static gl::GLenum
getTextureFormat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
   case latte::FMT_1:
   case latte::FMT_32_AS_8:
   case latte::FMT_32_AS_8_8:
   case latte::FMT_BC4:
      return gl::GL_RED;

   case latte::FMT_4_4:
   case latte::FMT_8_8:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_8_24:
   case latte::FMT_8_24_FLOAT:
   case latte::FMT_24_8:
   case latte::FMT_24_8_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_BC5:
      return gl::GL_RG;

   case latte::FMT_3_3_2:
   case latte::FMT_5_6_5:
   case latte::FMT_6_5_5:
   case latte::FMT_10_11_11:
   case latte::FMT_10_11_11_FLOAT:
   case latte::FMT_11_11_10:
   case latte::FMT_11_11_10_FLOAT:
   case latte::FMT_8_8_8:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      return gl::GL_RGB;

   case latte::FMT_1_5_5_5:
   case latte::FMT_4_4_4_4:
   case latte::FMT_5_5_5_1:
   case latte::FMT_2_10_10_10:
   case latte::FMT_8_8_8_8:
   case latte::FMT_10_10_10_2:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
   case latte::FMT_5_9_9_9_SHAREDEXP:
   case latte::FMT_BC1:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
      return gl::GL_RGBA;

   // case latte::FMT_X24_8_32_FLOAT:
   // case latte::FMT_GB_GR:
   // case latte::FMT_BG_RG:
   default:
      decaf_abort(fmt::format("Unimplemented texture format {}", format));
   }
}

static gl::GLenum
getTextureDataType(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp)
{
   auto isSigned = (formatComp == latte::SQ_FORMAT_COMP_SIGNED);

   switch (format) {
   case latte::FMT_8:
   case latte::FMT_8_8:
   case latte::FMT_8_8_8:
   case latte::FMT_8_8_8_8:
      return isSigned ? gl::GL_BYTE : gl::GL_UNSIGNED_BYTE;

   case latte::FMT_16:
   case latte::FMT_16_16:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_16:
      return isSigned ? gl::GL_SHORT : gl::GL_UNSIGNED_SHORT;

   case latte::FMT_16_FLOAT:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_16_16_16_16_FLOAT:
      return gl::GL_HALF_FLOAT;

   case latte::FMT_32:
   case latte::FMT_32_32:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_32:
      return isSigned ? gl::GL_INT : gl::GL_UNSIGNED_INT;

   case latte::FMT_32_FLOAT:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_32_32_32_32_FLOAT:
      return gl::GL_FLOAT;

   case latte::FMT_3_3_2:
      return gl::GL_UNSIGNED_BYTE_3_3_2;

   case latte::FMT_5_6_5:
      return gl::GL_UNSIGNED_SHORT_5_6_5;

   case latte::FMT_5_5_5_1:
      return gl::GL_UNSIGNED_SHORT_5_5_5_1;
   case latte::FMT_1_5_5_5:
      return gl::GL_UNSIGNED_SHORT_1_5_5_5_REV;

   case latte::FMT_4_4_4_4:
      return gl::GL_UNSIGNED_SHORT_4_4_4_4;

   case latte::FMT_10_10_10_2:
      return gl::GL_UNSIGNED_INT_10_10_10_2;
   case latte::FMT_2_10_10_10:
      return gl::GL_UNSIGNED_INT_2_10_10_10_REV;

   case latte::FMT_10_11_11:
   case latte::FMT_10_11_11_FLOAT:
   case latte::FMT_11_11_10:
   case latte::FMT_11_11_10_FLOAT:
      return gl::GL_UNSIGNED_INT_10F_11F_11F_REV;

   default:
      decaf_abort(fmt::format("Unimplemented texture format {}", format));
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
      decaf_abort(fmt::format("Unimplemented compressed texture format {}", format));
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
      decaf_abort(fmt::format("Unimplemented compressed texture swizzle {}", sel));
   }
}

bool GLDriver::checkActiveTextures()
{
   static const auto MAX_PS_TEXTURES = 16;
   std::vector<uint8_t> untiledImage, untiledMipmap;
   gx2::GX2Surface surface;

   for (auto i = 0; i < MAX_PS_TEXTURES; ++i) {
      auto resourceOffset = (latte::SQ_PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_TEX_RESOURCE_WORD1_0 + 4 * resourceOffset);
      auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_TEX_RESOURCE_WORD2_0 + 4 * resourceOffset);
      auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_TEX_RESOURCE_WORD3_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * resourceOffset);
      auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_TEX_RESOURCE_WORD5_0 + 4 * resourceOffset);
      auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_TEX_RESOURCE_WORD6_0 + 4 * resourceOffset);
      auto baseAddress = sq_tex_resource_word2.BASE_ADDRESS() << 8;

      if (!baseAddress) {
         continue;
      }

      // Decode resource registers
      auto pitch = (sq_tex_resource_word0.PITCH() + 1) * 8;
      auto width = sq_tex_resource_word0.TEX_WIDTH() + 1;
      auto height = sq_tex_resource_word1.TEX_HEIGHT() + 1;
      auto depth = sq_tex_resource_word1.TEX_DEPTH() + 1;

      auto format = sq_tex_resource_word1.DATA_FORMAT().get();
      auto tileMode = sq_tex_resource_word0.TILE_MODE().get();
      auto numFormat = sq_tex_resource_word4.NUM_FORMAT_ALL();
      auto formatComp = sq_tex_resource_word4.FORMAT_COMP_X();
      auto degamma = sq_tex_resource_word4.FORCE_DEGAMMA();
      auto dim = sq_tex_resource_word0.DIM().get();

      auto buffer = getSurfaceBuffer(baseAddress, width, height, depth, dim, format, numFormat, formatComp, degamma);

      if (buffer->dirtyAsTexture) {
         auto swizzle = sq_tex_resource_word2.SWIZZLE() << 8;

         // Rebuild a GX2Surface
         std::memset(&surface, 0, sizeof(gx2::GX2Surface));

         surface.dim = static_cast<gx2::GX2SurfaceDim>(dim);
         surface.width = width;
         surface.height = height;

         if (surface.dim == gx2::GX2SurfaceDim::TextureCube) {
            surface.depth = depth * 6;
         } else if (surface.dim == gx2::GX2SurfaceDim::Texture3D ||
            surface.dim == gx2::GX2SurfaceDim::Texture2DMSAAArray ||
            surface.dim == gx2::GX2SurfaceDim::Texture2DArray ||
            surface.dim == gx2::GX2SurfaceDim::Texture1DArray) {
            surface.depth = depth;
         } else {
            surface.depth = 1;
         }

         surface.mipLevels = 1;
         surface.format = getSurfaceFormat(format, numFormat, formatComp, degamma);

         surface.aa = gx2::GX2AAMode::Mode1X;
         surface.use = gx2::GX2SurfaceUse::Texture;

         if (sq_tex_resource_word0.TILE_TYPE()) {
            surface.use |= gx2::GX2SurfaceUse::DepthBuffer;
         }

         surface.tileMode = static_cast<gx2::GX2TileMode>(tileMode);
         surface.swizzle = swizzle;

         // Update the sizing information for the surface
         GX2CalcSurfaceSizeAndAlignment(&surface);

         // Align address
         baseAddress &= ~(surface.alignment - 1);

         surface.image = make_virtual_ptr<uint8_t>(baseAddress);
         surface.mipmaps = nullptr;

         // Calculate a new memory CRC
         uint64_t newHash[2] = { 0 };
         MurmurHash3_x64_128(surface.image, surface.imageSize, 0, newHash);

         // If the CPU memory has changed, we should re-upload this.  This hashing is
         //  also means that if the application temporarily uses one of its buffers as
         //  a color buffer, we are able to accurately handle this.  Providing they are
         //  not updating the memory at the same time.
         if (newHash[0] != buffer->cpuMemHash[0] || newHash[1] != buffer->cpuMemHash[1]) {
            buffer->cpuMemHash[0] = newHash[0];
            buffer->cpuMemHash[1] = newHash[1];

            // Untile
            gx2::internal::convertTiling(&surface, untiledImage, untiledMipmap);

            // Create texture
            auto compressed = isCompressedFormat(format);
            auto target = getTextureTarget(dim);
            auto textureDataType = gl::GL_INVALID_ENUM;
            auto textureFormat = getTextureFormat(format);
            auto size = untiledImage.size();

            if (compressed) {
               textureDataType = getCompressedTextureDataType(format, degamma);
            } else {
               textureDataType = getTextureDataType(format, formatComp);
            }

            if (textureDataType == gl::GL_INVALID_ENUM || textureFormat == gl::GL_INVALID_ENUM) {
               decaf_abort(fmt::format("Texture with unsupported format {}", surface.format.value()));
            }

            switch (dim) {
            case latte::SQ_TEX_DIM_2D:
               if (compressed) {
                  gl::glCompressedTextureSubImage2D(buffer->object, 0,
                     0, 0,
                     width, height,
                     textureDataType,
                     gsl::narrow_cast<gl::GLsizei>(size), untiledImage.data());
               } else {
                  gl::glTextureSubImage2D(buffer->object, 0,
                     0, 0,
                     width, height,
                     textureFormat, textureDataType,
                     untiledImage.data());
               }
               break;
            case latte::SQ_TEX_DIM_2D_ARRAY:
               if (compressed) {
                  gl::glCompressedTextureSubImage3D(buffer->object, 0,
                     0, 0, 0,
                     width, height, depth,
                     textureDataType,
                     gsl::narrow_cast<gl::GLsizei>(size), untiledImage.data());
               } else {
                  gl::glTextureSubImage3D(buffer->object, 0,
                     0, 0, 0,
                     width, height, depth,
                     textureFormat, textureDataType,
                     untiledImage.data());
               }
               break;
            case latte::SQ_TEX_DIM_1D:
            case latte::SQ_TEX_DIM_3D:
            case latte::SQ_TEX_DIM_CUBEMAP:
            case latte::SQ_TEX_DIM_1D_ARRAY:
            case latte::SQ_TEX_DIM_2D_MSAA:
            case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
               gLog->error("Unsupported texture dim: {}", sq_tex_resource_word0.DIM().get());
               continue;
            }
         }

         buffer->dirtyAsTexture = false;
         buffer->state = SurfaceUseState::CpuWritten;
      }

      // Setup texture swizzle
      auto dst_sel_x = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_X());
      auto dst_sel_y = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Y());
      auto dst_sel_z = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Z());
      auto dst_sel_w = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_W());

      gl::GLint textureSwizzle[] = {
         static_cast<gl::GLint>(dst_sel_x),
         static_cast<gl::GLint>(dst_sel_y),
         static_cast<gl::GLint>(dst_sel_z),
         static_cast<gl::GLint>(dst_sel_w),
      };

      gl::glTextureParameteriv(buffer->object, gl::GL_TEXTURE_SWIZZLE_RGBA, textureSwizzle);
      gl::glBindTextureUnit(i, buffer->object);
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
      decaf_abort(fmt::format("Unimplemented texture wrap {}", clamp));
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
      decaf_abort(fmt::format("Unimplemented texture xy filter {}", filter));
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
      decaf_abort(fmt::format("Unimplemented texture compare function {}", func));
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

      if (sq_tex_sampler_word0.value == 0
       && sq_tex_sampler_word1.value == 0
       && sq_tex_sampler_word2.value == 0) {
         gl::glBindSampler(i, 0);
         continue;
      }

      auto &sampler = mPixelSamplers[i];

      if (!sampler.object) {
         gl::glCreateSamplers(1, &sampler.object);
      }

      // Texture clamp
      auto clamp_x = getTextureWrap(sq_tex_sampler_word0.CLAMP_X());
      auto clamp_y = getTextureWrap(sq_tex_sampler_word0.CLAMP_Y());
      auto clamp_z = getTextureWrap(sq_tex_sampler_word0.CLAMP_Z());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_S, static_cast<gl::GLint>(clamp_x));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_T, static_cast<gl::GLint>(clamp_y));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_R, static_cast<gl::GLint>(clamp_z));

      // Texture filter
      auto xy_min_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MIN_FILTER());
      auto xy_mag_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MAG_FILTER());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<gl::GLint>(xy_min_filter));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<gl::GLint>(xy_mag_filter));

      // Setup border color
      auto border_color_type = sq_tex_sampler_word0.BORDER_COLOR_TYPE();
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
      auto depth_compare_function = getTextureCompareFunction(sq_tex_sampler_word0.DEPTH_COMPARE_FUNCTION());
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_FUNC, static_cast<gl::GLint>(depth_compare_function));

      // Setup texture LOD
      auto min_lod = sq_tex_sampler_word1.MIN_LOD();
      auto max_lod = sq_tex_sampler_word1.MAX_LOD();
      auto lod_bias = sq_tex_sampler_word1.LOD_BIAS();

      // TODO: GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD, GL_TEXTURE_LOD_BIAS

      // Bind sampler
      gl::glBindSampler(i, sampler.object);
   }

   return true;
}

} // namespace opengl

} // namespace gpu
