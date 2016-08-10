#include "common/align.h"
#include "common/decaf_assert.h"
#include "gpu/latte_enum_sq.h"
#include "modules/gx2/gx2_addrlib.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Meta.h>

namespace gpu
{

namespace opengl
{

static gl::GLenum
getStorageFormat(latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma,
                 gl::GLenum unorm,
                 gl::GLenum snorm,
                 gl::GLenum uint,
                 gl::GLenum sint,
                 gl::GLenum srgb,
                 gl::GLenum scaled)
{
   if (!degamma) {
      if (numFormat == latte::SQ_NUM_FORMAT_NORM) {
         if (formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
            return snorm;
         } else if (formatComp == latte::SQ_FORMAT_COMP_UNSIGNED) {
            return unorm;
         } else {
            return gl::GL_INVALID_ENUM;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT_INT) {
         if (formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
            return sint;
         } else if (formatComp == latte::SQ_FORMAT_COMP_UNSIGNED) {
            return uint;
         } else {
            return gl::GL_INVALID_ENUM;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT_SCALED) {
         if (formatComp == latte::SQ_FORMAT_COMP_UNSIGNED) {
            return scaled;
         } else {
            return gl::GL_INVALID_ENUM;
         }
      } else {
         return gl::GL_INVALID_ENUM;
      }
   } else {
      if (numFormat == 0 && formatComp == 0) {
         return srgb;
      } else {
         return gl::GL_INVALID_ENUM;
      }
   }
}

static gl::GLenum
getStorageFormat(latte::SQ_DATA_FORMAT format,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma,
                 bool isDepthBuffer)
{
   static const auto BADFMT = gl::GL_INVALID_ENUM;
   auto getFormat =
      [=](gl::GLenum unorm, gl::GLenum snorm, gl::GLenum uint, gl::GLenum sint, gl::GLenum srgb) {
         return getStorageFormat(numFormat, formatComp, degamma, unorm, snorm, uint, sint, srgb, BADFMT);
      };
   auto getFormatF =
      [=](gl::GLenum scaled) {
         return getStorageFormat(numFormat, formatComp, degamma, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, scaled);
      };

   switch (format) {
   case latte::FMT_8:
      return getFormat(gl::GL_R8, gl::GL_R8_SNORM, gl::GL_R8UI, gl::GL_R8I, gl::GL_SRGB8);
   //case latte::FMT_4_4:
   //case latte::FMT_3_3_2:
   case latte::FMT_16:
      if (isDepthBuffer) {
         return gl::GL_DEPTH_COMPONENT16;
      } else {
         return getFormat(gl::GL_R16, gl::GL_R16_SNORM, gl::GL_R16UI, gl::GL_R16I, BADFMT);
      }
   case latte::FMT_16_FLOAT:
      return getFormatF(gl::GL_R16F);
   case latte::FMT_8_8:
      return getFormat(gl::GL_RG8, gl::GL_RG8_SNORM, gl::GL_RG8UI, gl::GL_RG8I, BADFMT);
   case latte::FMT_5_6_5:
      return getFormat(gl::GL_RGB565, BADFMT, BADFMT, BADFMT, BADFMT);
   //case latte::FMT_6_5_5:
   //case latte::FMT_1_5_5_5:
   case latte::FMT_4_4_4_4:
      return getFormat(gl::GL_RGBA4, BADFMT, BADFMT, BADFMT, BADFMT);
   //case latte::FMT_5_5_5_1:
   case latte::FMT_32:
      return getFormat(BADFMT, BADFMT, gl::GL_R32UI, gl::GL_R32I, BADFMT);
   case latte::FMT_32_FLOAT:
      if (isDepthBuffer) {
         return gl::GL_DEPTH_COMPONENT32F;
      }

      return getFormatF(gl::GL_R32F);
   case latte::FMT_16_16:
      return getFormat(gl::GL_RG16, gl::GL_RG16_SNORM, gl::GL_RG16UI, gl::GL_RG16I, BADFMT);
   case latte::FMT_16_16_FLOAT:
      return getFormatF(gl::GL_RG16F);
   //case latte::FMT_24_8:
   //case latte::FMT_24_8_FLOAT:
   //case latte::FMT_10_11_11:
   case latte::FMT_10_11_11_FLOAT:
      return getFormatF(gl::GL_R11F_G11F_B10F);
   //case latte::FMT_11_11_10:
   case latte::FMT_11_11_10_FLOAT:
      return getFormatF(gl::GL_R11F_G11F_B10F);
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
      return getFormat(gl::GL_RGB10_A2, BADFMT, gl::GL_RGB10_A2UI, BADFMT, BADFMT);
   case latte::FMT_8_8_8_8:
      return getFormat(gl::GL_RGBA8, gl::GL_RGBA8_SNORM, gl::GL_RGBA8UI, gl::GL_RGBA8I, gl::GL_SRGB8_ALPHA8);
   case latte::FMT_32_32:
      return getFormat(BADFMT, BADFMT, gl::GL_RG32UI, gl::GL_RG32I, BADFMT);
   case latte::FMT_32_32_FLOAT:
      return getFormatF(gl::GL_RG32F);
   case latte::FMT_16_16_16_16:
      return getFormat(gl::GL_RGBA16, gl::GL_RGBA16_SNORM, gl::GL_RGBA16UI, gl::GL_RGBA16I, BADFMT);
   case latte::FMT_16_16_16_16_FLOAT:
      return getFormatF(gl::GL_RGBA16F);
   case latte::FMT_32_32_32_32:
      return getFormat(BADFMT, BADFMT, gl::GL_RGBA32UI, gl::GL_RGBA32I, BADFMT);
   case latte::FMT_32_32_32_32_FLOAT:
      return getFormatF(gl::GL_RGBA32F);
   //case latte::FMT_1:
   //case latte::FMT_GB_GR:
   //case latte::FMT_BG_RG:
   //case latte::FMT_32_AS_8:
   //case latte::FMT_32_AS_8_8:
   //case latte::FMT_5_9_9_9_SHAREDEXP:
   case latte::FMT_8_8_8:
      return getFormat(gl::GL_RGB8, gl::GL_RGB8_SNORM, gl::GL_RGB8UI, gl::GL_RGB8I, gl::GL_SRGB8);
   case latte::FMT_16_16_16:
      return getFormat(gl::GL_RGB16, gl::GL_RGB16_SNORM, gl::GL_RGB16UI, gl::GL_RGB16I, BADFMT);
   case latte::FMT_16_16_16_FLOAT:
      return getFormatF(gl::GL_RGB16F);
   case latte::FMT_32_32_32:
      return getFormat(BADFMT, BADFMT, gl::GL_RGB32UI, gl::GL_RGB32I, BADFMT);
   case latte::FMT_32_32_32_FLOAT:
      return getFormatF(gl::GL_RGB32F);
   case latte::FMT_BC1:
      return getFormat(gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT);
   case latte::FMT_BC2:
      return getFormat(gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT);
   case latte::FMT_BC3:
      return getFormat(gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
   case latte::FMT_BC4:
      return getFormat(gl::GL_COMPRESSED_RED_RGTC1, gl::GL_COMPRESSED_SIGNED_RED_RGTC1, BADFMT, BADFMT, BADFMT);
   case latte::FMT_BC5:
      return getFormat(gl::GL_COMPRESSED_RG_RGTC2, gl::GL_COMPRESSED_SIGNED_RG_RGTC2, BADFMT, BADFMT, BADFMT);
   //case latte::FMT_APC0:
   //case latte::FMT_APC1:
   //case latte::FMT_APC2:
   //case latte::FMT_APC3:
   //case latte::FMT_APC4:
   //case latte::FMT_APC5:
   //case latte::FMT_APC6:
   //case latte::FMT_APC7:
   //case latte::FMT_CTX1:

   // Depth Types
   case latte::FMT_8_24:
      decaf_check(isDepthBuffer);
      return gl::GL_DEPTH24_STENCIL8;
   case latte::FMT_X24_8_32_FLOAT:
      decaf_check(isDepthBuffer);
      return gl::GL_DEPTH32F_STENCIL8;

   default:
      decaf_abort(fmt::format("Invalid surface format {}", format));
   }
}

static int getStorageFormatBits(gl::GLenum format)
{
   switch (format) {
   case gl::GL_COMPRESSED_RED_RGTC1:
   case gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case gl::GL_COMPRESSED_SIGNED_RED_RGTC1:
   case gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      return 4;

   case gl::GL_COMPRESSED_RG_RGTC2:
   case gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case gl::GL_COMPRESSED_SIGNED_RG_RGTC2:
   case gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
   case gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
   case gl::GL_R8:
   case gl::GL_R8_SNORM:
   case gl::GL_R8I:
   case gl::GL_R8UI:
      return 8;

   case gl::GL_DEPTH_COMPONENT16:
   case gl::GL_R16:
   case gl::GL_R16_SNORM:
   case gl::GL_R16F:
   case gl::GL_R16I:
   case gl::GL_R16UI:
   case gl::GL_RG8:
   case gl::GL_RG8_SNORM:
   case gl::GL_RG8I:
   case gl::GL_RG8UI:
   case gl::GL_RGB565:
   case gl::GL_RGBA4:
      return 16;

   case gl::GL_RGB8:
   case gl::GL_RGB8_SNORM:
   case gl::GL_RGB8I:
   case gl::GL_RGB8UI:
   case gl::GL_SRGB8:
      return 24;

   case gl::GL_DEPTH_COMPONENT32F:
   case gl::GL_DEPTH24_STENCIL8:
   case gl::GL_R11F_G11F_B10F:
   case gl::GL_R32F:
   case gl::GL_R32I:
   case gl::GL_R32UI:
   case gl::GL_RG16:
   case gl::GL_RG16_SNORM:
   case gl::GL_RG16F:
   case gl::GL_RG16I:
   case gl::GL_RG16UI:
   case gl::GL_RGB10_A2:
   case gl::GL_RGB10_A2UI:
   case gl::GL_RGBA8:
   case gl::GL_RGBA8_SNORM:
   case gl::GL_RGBA8I:
   case gl::GL_RGBA8UI:
   case gl::GL_SRGB8_ALPHA8:
      return 32;

   case gl::GL_RGB16:
   case gl::GL_RGB16_SNORM:
   case gl::GL_RGB16F:
   case gl::GL_RGB16I:
   case gl::GL_RGB16UI:
      return 48;

   case gl::GL_DEPTH32F_STENCIL8:
   case gl::GL_RG32F:
   case gl::GL_RG32I:
   case gl::GL_RG32UI:
   case gl::GL_RGBA16:
   case gl::GL_RGBA16_SNORM:
   case gl::GL_RGBA16F:
   case gl::GL_RGBA16I:
   case gl::GL_RGBA16UI:
      return 64;

   case gl::GL_RGB32F:
   case gl::GL_RGB32I:
   case gl::GL_RGB32UI:
      return 96;

   case gl::GL_RGBA32F:
   case gl::GL_RGBA32I:
   case gl::GL_RGBA32UI:
      return 128;

   default:
      decaf_abort(fmt::format("Invalid GL storage format {}", glbinding::Meta::getString(format)));
   }
}

SurfaceBuffer *
GLDriver::getSurfaceBuffer(ppcaddr_t baseAddress,
                           uint32_t width,
                           uint32_t height,
                           uint32_t depth,
                           latte::SQ_TEX_DIM dim,
                           latte::SQ_DATA_FORMAT format,
                           latte::SQ_NUM_FORMAT numFormat,
                           latte::SQ_FORMAT_COMP formatComp,
                           uint32_t degamma,
                           bool isDepthBuffer)
{
   decaf_check(baseAddress);
   decaf_check(width);
   decaf_check(height);
   decaf_check(depth);
   decaf_check(width <= 8192);
   decaf_check(height <= 8192);

   auto surfaceKey = static_cast<uint64_t>(baseAddress) << 32;
   // Note: align_up on width and height is needed to ensure that strangely
   //  sized framebuffers can be properly reused as textures (for example,
   //  a 384x320 framebuffer getting drawn as a 384x310 texture)
   surfaceKey ^= align_up(width, 16) ^ align_up(height, 16) ^ depth ^ dim;
   surfaceKey ^= format ^ numFormat ^ formatComp ^ degamma;

   auto bufferIter = mSurfaces.find(surfaceKey);

   if (bufferIter != mSurfaces.end()) {
      return &bufferIter->second;
   }

   auto insertRes = mSurfaces.emplace(surfaceKey, SurfaceBuffer{});
   auto buffer = &insertRes.first->second;

   auto storageFormat = getStorageFormat(format, numFormat, formatComp, degamma, isDepthBuffer);

   if (storageFormat == gl::GL_INVALID_ENUM) {
      decaf_abort(fmt::format("Surface with unsupported format {} {} {} {}", format, numFormat, formatComp, degamma));
   }

   // We need to keep track of the memory region every GPU resource uses
   //  so that we are able to invalidate them as needed.
   buffer->cpuMemStart = baseAddress;

   // Lets track some other useful information
   buffer->dbgInfo.width = width;
   buffer->dbgInfo.height = height;
   buffer->dbgInfo.depth = depth;
   buffer->dbgInfo.dim = dim;
   buffer->dbgInfo.format = format;
   buffer->dbgInfo.numFormat = numFormat;
   buffer->dbgInfo.formatComp = formatComp;
   buffer->dbgInfo.degamma = degamma;

   auto bitsPerPixel = getStorageFormatBits(storageFormat);
   uint32_t numPixels;

   if (!buffer->object) {
      switch (dim) {
      case latte::SQ_TEX_DIM_1D:
         gl::glCreateTextures(gl::GL_TEXTURE_1D, 1, &buffer->object);
         gl::glTextureStorage1D(buffer->object, 1, storageFormat, width);
         numPixels = width;
         break;
      case latte::SQ_TEX_DIM_2D:
         gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         numPixels = width * height;
         break;
      case latte::SQ_TEX_DIM_2D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_2D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         numPixels = width * height * depth;
         break;
      case latte::SQ_TEX_DIM_CUBEMAP:
         gl::glCreateTextures(gl::GL_TEXTURE_CUBE_MAP, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         numPixels = width * height * 6;
         break;
      case latte::SQ_TEX_DIM_3D:
         gl::glCreateTextures(gl::GL_TEXTURE_3D, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         numPixels = width * height * depth;
         break;
      case latte::SQ_TEX_DIM_1D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_1D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         numPixels = width * height;
         break;
      default:
         decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
      }
   }

   // numBits could theoretically be >= 2^32, so uint64_t to avoid overflow
   auto numBits = static_cast<uint64_t>(numPixels) * bitsPerPixel;
   // Some (buggy?) apps try to give us 1x1 compressed textures...
   if (format >= latte::FMT_BC1 && format <= latte::FMT_BC5 && (width == 1 || height == 1)) {
      gLog->debug("Got compressed texture with invalid size {}x{}", width, height);
   } else {
      decaf_check(numBits % 8 == 0);
   }
   decaf_check(numBits / 8 < UINT64_C(1) << 32);
   buffer->cpuMemEnd = baseAddress + static_cast<uint32_t>(numBits / 8);

   return buffer;
}

} // namespace opengl

} // namespace gpu
