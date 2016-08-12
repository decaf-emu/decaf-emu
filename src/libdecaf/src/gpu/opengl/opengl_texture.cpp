#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/murmur3.h"
#include "gpu/gpu_tiling.h"
#include "gpu/latte_enum_sq.h"
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
getTextureDataType(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
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

static uint32_t
getDataFormatBitsPerElement(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_3_3_2:
      return 8;

   case latte::FMT_8_8:
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_5_6_5:
   case latte::FMT_5_5_5_1:
   case latte::FMT_1_5_5_5:
   case latte::FMT_4_4_4_4:
      return 16;

   case latte::FMT_8_8_8:
      return 24;

   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_11_11:
   case latte::FMT_10_11_11_FLOAT:
   case latte::FMT_11_11_10:
   case latte::FMT_11_11_10_FLOAT:
      return 32;

   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
      return 48;

   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_BC1:
   case latte::FMT_BC4:
      return 64;

   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      return 96;

   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
   case latte::FMT_BC5:
      return 128;

   default:
      decaf_abort(fmt::format("Unimplemented data format {}", format));
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
getCompressedTextureDataType(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   auto isSigned = formatComp == latte::SQ_FORMAT_COMP_SIGNED;

   switch (format) {
   case latte::FMT_BC1:
      decaf_check(!isSigned);
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   case latte::FMT_BC2:
      decaf_check(!isSigned);
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   case latte::FMT_BC3:
      decaf_check(!isSigned);
      return degamma ? gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
   case latte::FMT_BC4:
      decaf_check(!degamma);
      return isSigned ? gl::GL_COMPRESSED_SIGNED_RED_RGTC1 : gl::GL_COMPRESSED_RED_RGTC1;
   case latte::FMT_BC5:
      decaf_check(!degamma);
      return isSigned ? gl::GL_COMPRESSED_SIGNED_RG_RGTC2 : gl::GL_COMPRESSED_RG_RGTC2;
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
   std::vector<uint8_t> untiledImage, untiledMipmap;

   for (auto i = 0; i < latte::MaxTextures; ++i) {
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

      if (baseAddress == mPixelTextureCache[i].baseAddress
       && sq_tex_resource_word0.value == mPixelTextureCache[i].word0
       && sq_tex_resource_word1.value == mPixelTextureCache[i].word1
       && sq_tex_resource_word2.value == mPixelTextureCache[i].word2
       && sq_tex_resource_word3.value == mPixelTextureCache[i].word3
       && sq_tex_resource_word4.value == mPixelTextureCache[i].word4
       && sq_tex_resource_word5.value == mPixelTextureCache[i].word5
       && sq_tex_resource_word6.value == mPixelTextureCache[i].word6) {
         continue;  // No change in sampler state
      }

      mPixelTextureCache[i].baseAddress = baseAddress;
      mPixelTextureCache[i].word0 = sq_tex_resource_word0.value;
      mPixelTextureCache[i].word1 = sq_tex_resource_word1.value;
      mPixelTextureCache[i].word2 = sq_tex_resource_word2.value;
      mPixelTextureCache[i].word3 = sq_tex_resource_word3.value;
      mPixelTextureCache[i].word4 = sq_tex_resource_word4.value;
      mPixelTextureCache[i].word5 = sq_tex_resource_word5.value;
      mPixelTextureCache[i].word6 = sq_tex_resource_word6.value;

      // Decode resource registers
      auto pitch = (sq_tex_resource_word0.PITCH() + 1) * 8;
      auto width = sq_tex_resource_word0.TEX_WIDTH() + 1;
      auto height = sq_tex_resource_word1.TEX_HEIGHT() + 1;
      auto depth = sq_tex_resource_word1.TEX_DEPTH() + 1;

      auto format = sq_tex_resource_word1.DATA_FORMAT();
      auto tileMode = sq_tex_resource_word0.TILE_MODE();
      auto numFormat = sq_tex_resource_word4.NUM_FORMAT_ALL();
      auto formatComp = sq_tex_resource_word4.FORMAT_COMP_X();
      auto degamma = sq_tex_resource_word4.FORCE_DEGAMMA();
      auto dim = sq_tex_resource_word0.DIM();
      auto swizzle = sq_tex_resource_word2.SWIZZLE() << 8;
      auto isDepthBuffer = !!sq_tex_resource_word0.TILE_TYPE();

      if (tileMode >= latte::SQ_TILE_MODE_TILED_2D_THIN1) {
         baseAddress &= ~(0x800 - 1);
      } else {
         baseAddress &= ~(0x100 - 1);
      }

      auto buffer = getSurfaceBuffer(baseAddress, pitch, width, height, depth, dim, format, numFormat, formatComp, degamma, isDepthBuffer);

      if (buffer->dirtyAsTexture) {
         auto imagePtr = make_virtual_ptr<uint8_t>(baseAddress);

         auto bpp = getDataFormatBitsPerElement(format);

         auto srcWidth = width;
         auto srcHeight = height;
         auto srcPitch = pitch;
         auto uploadWidth = width;
         auto uploadHeight = height;
         auto uploadPitch = width;
         if (format >= latte::FMT_BC1 && format <= latte::FMT_BC5) {
            srcWidth = (srcWidth + 3) / 4;
            srcHeight = (srcHeight + 3) / 4;
            srcPitch = srcPitch / 4;
            uploadWidth = srcWidth * 4;
            uploadHeight = srcHeight * 4;
            uploadPitch = uploadWidth / 4;
         }

         auto uploadDepth = depth;
         if (dim == latte::SQ_TEX_DIM_CUBEMAP) {
            uploadDepth *= 6;
         }

         auto srcImageSize = srcPitch * srcHeight * uploadDepth * bpp / 8;
         auto dstImageSize = srcWidth * srcHeight * uploadDepth * bpp / 8;

         // Calculate a new memory CRC
         uint64_t newHash[2] = { 0 };
         MurmurHash3_x64_128(imagePtr, srcImageSize, 0, newHash);

         // If the CPU memory has changed, we should re-upload this.  This hashing is
         //  also means that if the application temporarily uses one of its buffers as
         //  a color buffer, we are able to accurately handle this.  Providing they are
         //  not updating the memory at the same time.
         if (newHash[0] != buffer->cpuMemHash[0] || newHash[1] != buffer->cpuMemHash[1]) {
            buffer->cpuMemHash[0] = newHash[0];
            buffer->cpuMemHash[1] = newHash[1];

            untiledImage.resize(dstImageSize);

            // Untile
            gpu::convertFromTiled(
               untiledImage.data(),
               uploadPitch,
               imagePtr,
               tileMode,
               swizzle,
               srcPitch,
               srcWidth,
               srcHeight,
               uploadDepth,
               0,
               isDepthBuffer,
               bpp
            );

            // Create texture
            auto compressed = isCompressedFormat(format);
            auto target = getTextureTarget(dim);
            auto textureDataType = gl::GL_INVALID_ENUM;
            auto textureFormat = getTextureFormat(format);
            auto size = untiledImage.size();

            if (compressed) {
               textureDataType = getCompressedTextureDataType(format, formatComp, degamma);
            } else {
               textureDataType = getTextureDataType(format, formatComp, degamma);
            }

            if (textureDataType == gl::GL_INVALID_ENUM || textureFormat == gl::GL_INVALID_ENUM) {
               decaf_abort(fmt::format("Texture with unsupported format {}", format));
            }

            switch (dim) {
            case latte::SQ_TEX_DIM_1D:
               if (compressed) {
                  gl::glCompressedTextureSubImage1D(buffer->object,
                                                    0, /* level */
                                                    0, /* xoffset */
                                                    width,
                                                    textureDataType,
                                                    gsl::narrow_cast<gl::GLsizei>(size),
                                                    untiledImage.data());
               } else {
                  gl::glTextureSubImage1D(buffer->object,
                                          0, /* level */
                                          0, /* xoffset */
                                          width,
                                          textureFormat,
                                          textureDataType,
                                          untiledImage.data());
               }
               break;
            case latte::SQ_TEX_DIM_2D:
               if (compressed) {
                  gl::glCompressedTextureSubImage2D(buffer->object,
                                                    0, /* level */
                                                    0, 0, /* xoffset, yoffset */
                                                    width,
                                                    height,
                                                    textureDataType,
                                                    gsl::narrow_cast<gl::GLsizei>(size),
                                                    untiledImage.data());
               } else {
                  gl::glTextureSubImage2D(buffer->object,
                                          0, /* level */
                                          0, 0, /* xoffset, yoffset */
                                          width, height,
                                          textureFormat,
                                          textureDataType,
                                          untiledImage.data());
               }
               break;
            case latte::SQ_TEX_DIM_3D:
               if (compressed) {
                  gl::glCompressedTextureSubImage3D(buffer->object,
                                                    0, /* level */
                                                    0, 0, 0, /* xoffset, yoffset, zoffset */
                                                    width, height, depth,
                                                    textureDataType,
                                                    gsl::narrow_cast<gl::GLsizei>(size),
                                                    untiledImage.data());
               } else {
                  gl::glTextureSubImage3D(buffer->object,
                                          0, /* level */
                                          0, 0, 0, /* xoffset, yoffset, zoffset */
                                          width, height, depth,
                                          textureFormat,
                                          textureDataType,
                                          untiledImage.data());
               }
               break;
            case latte::SQ_TEX_DIM_CUBEMAP:
               decaf_check(uploadDepth == 6);
            case latte::SQ_TEX_DIM_2D_ARRAY:
               if (compressed) {
                  gl::glCompressedTextureSubImage3D(buffer->object,
                                                    0, /* level */
                                                    0, 0, 0, /* xoffset, yoffset, zoffset */
                                                    width, height, uploadDepth,
                                                    textureDataType,
                                                    gsl::narrow_cast<gl::GLsizei>(size),
                                                    untiledImage.data());
               } else {
                  gl::glTextureSubImage3D(buffer->object,
                                          0, /* level */
                                          0, 0, 0, /* xoffset, yoffset, zoffset */
                                          width, height, uploadDepth,
                                          textureFormat,
                                          textureDataType,
                                          untiledImage.data());
               }
               break;
            default:
               decaf_abort(fmt::format("Unsupported texture dim: {}", sq_tex_resource_word0.DIM()));
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

      // Store texture coordinate scale factors for shaders
      mTexCoordScale[i * 4 + 0] = static_cast<float>(width) / static_cast<float>(buffer->width);
      mTexCoordScale[i * 4 + 1] = static_cast<float>(height) / static_cast<float>(buffer->height);
      mTexCoordScale[i * 4 + 2] = static_cast<float>(depth) / static_cast<float>(buffer->depth);
   }

   // Send texture scale array to shaders
   if (mActiveShader->vertex->uniformTexScale != -1) {
      gl::glProgramUniform4fv(mActiveShader->vertex->object,
                              mActiveShader->vertex->uniformTexScale,
                              latte::MaxTextures,
                              &mTexCoordScale[0]);
   }

   if (mActiveShader->pixel && mActiveShader->pixel->uniformTexScale != -1) {
      gl::glProgramUniform4fv(mActiveShader->pixel->object,
                              mActiveShader->pixel->uniformTexScale,
                              latte::MaxTextures,
                              &mTexCoordScale[0]);
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
   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto sq_tex_sampler_word0 = getRegister<latte::SQ_TEX_SAMPLER_WORD0_N>(latte::Register::SQ_TEX_SAMPLER_WORD0_0 + 4 * (i * 3));
      auto sq_tex_sampler_word1 = getRegister<latte::SQ_TEX_SAMPLER_WORD1_N>(latte::Register::SQ_TEX_SAMPLER_WORD1_0 + 4 * (i * 3));
      auto sq_tex_sampler_word2 = getRegister<latte::SQ_TEX_SAMPLER_WORD2_N>(latte::Register::SQ_TEX_SAMPLER_WORD2_0 + 4 * (i * 3));

      // TODO: is there a sampler bit that indicates this, maybe word2.TYPE?
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + latte::SQ_PS_TEX_RESOURCE_0 + 4 * (i * 7));
      auto depthCompare = !!sq_tex_resource_word0.TILE_TYPE();

      if (sq_tex_sampler_word0.value == mPixelSamplerCache[i].word0
       && sq_tex_sampler_word1.value == mPixelSamplerCache[i].word1
       && sq_tex_sampler_word2.value == mPixelSamplerCache[i].word2
       && depthCompare == mPixelSamplerCache[i].depthCompare) {
         continue;  // No change in sampler state
      }

      mPixelSamplerCache[i].word0 = sq_tex_sampler_word0.value;
      mPixelSamplerCache[i].word1 = sq_tex_sampler_word1.value;
      mPixelSamplerCache[i].word2 = sq_tex_sampler_word2.value;
      mPixelSamplerCache[i].depthCompare = depthCompare;

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
      {
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
      default:
         decaf_abort(fmt::format("Unsupported border_color_type = {}", border_color_type));
      }

      gl::glSamplerParameterfv(sampler.object, gl::GL_TEXTURE_BORDER_COLOR, &colors[0]);

      // Depth compare
      auto mode = depthCompare ? gl::GL_COMPARE_REF_TO_TEXTURE : gl::GL_NONE;
      auto depth_compare_function = getTextureCompareFunction(sq_tex_sampler_word0.DEPTH_COMPARE_FUNCTION());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_MODE, static_cast<gl::GLint>(mode));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_FUNC, static_cast<gl::GLint>(depth_compare_function));

      // Setup texture LOD
      auto min_lod = sq_tex_sampler_word1.MIN_LOD();
      auto max_lod = sq_tex_sampler_word1.MAX_LOD();
      auto lod_bias = sq_tex_sampler_word1.LOD_BIAS();

      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_MIN_LOD, static_cast<float>(min_lod));
      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_MAX_LOD, static_cast<float>(max_lod));
      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_LOD_BIAS, static_cast<float>(lod_bias));

      // Bind sampler
      gl::glBindSampler(i, sampler.object);
   }

   return true;
}

} // namespace opengl

} // namespace gpu
