#include "common/emuassert.h"
#include "gpu/latte_enum_sq.h"
#include "modules/gx2/gx2_addrlib.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

static gl::GLenum
getStorageFormat(latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma,
   gl::GLenum unorm, gl::GLenum snorm, gl::GLenum uint, gl::GLenum sint, gl::GLenum srgb)
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
getStorageFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   static const auto BADFMT = gl::GL_INVALID_ENUM;

   // No format means that the GPU picks?  Maybe...
   if (format == 0) {
      return gl::GL_RGBA8;
   }

   // Pick the format
   switch (format) {
      // Normal Types
      case latte::SQ_DATA_FORMAT::FMT_8:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_R8, gl::GL_R8_SNORM, gl::GL_R8UI, gl::GL_R8I, gl::GL_SRGB8);
      //case latte::SQ_DATA_FORMAT::FMT_4_4:
      //case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_R16, gl::GL_R16_SNORM, gl::GL_R16UI, gl::GL_R16I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_8_8:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RG8, gl::GL_RG8_SNORM, gl::GL_RG8UI, gl::GL_RG8I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_5_6_5:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGB565, BADFMT, BADFMT, BADFMT, BADFMT);
      //case latte::SQ_DATA_FORMAT::FMT_6_5_5:
      //case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
      case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGBA4, BADFMT, BADFMT, BADFMT, BADFMT);
      //case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
      case latte::SQ_DATA_FORMAT::FMT_32:
         return getStorageFormat(numFormat, formatComp, degamma,
            BADFMT, BADFMT, gl::GL_R32UI, gl::GL_R32I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RG16, gl::GL_RG16_SNORM, gl::GL_RG16UI, gl::GL_RG16I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
         return gl::GL_RG16F;
      //case latte::SQ_DATA_FORMAT::FMT_24_8:
      //case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
      //case latte::SQ_DATA_FORMAT::FMT_10_11_11:
      //case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
      //case latte::SQ_DATA_FORMAT::FMT_11_11_10:
      case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
         return gl::GL_R11F_G11F_B10F;
      case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
         // TODO: Swap the swizzles! (2,10,10,10 vs 10,10,10,2)
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGB10_A2, BADFMT, gl::GL_RGB10_A2UI, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGBA8, gl::GL_RGBA8_SNORM, gl::GL_RGBA8UI, gl::GL_RGBA8I, gl::GL_SRGB8);
      case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGB10_A2, BADFMT, gl::GL_RGB10_A2UI, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32:
         return getStorageFormat(numFormat, formatComp, degamma,
            BADFMT, BADFMT, gl::GL_RG32UI, gl::GL_RG32I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
         return gl::GL_RG32F;
      case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGBA16, gl::GL_RGBA16_SNORM, gl::GL_RGBA16UI, gl::GL_RGBA16I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
         return gl::GL_RGBA16F;
      case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
         return getStorageFormat(numFormat, formatComp, degamma,
            BADFMT, BADFMT, gl::GL_RGBA32UI, gl::GL_RGBA32I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
         return gl::GL_RGBA32F;
      //case latte::SQ_DATA_FORMAT::FMT_1:
      //case latte::SQ_DATA_FORMAT::FMT_GB_GR:
      //case latte::SQ_DATA_FORMAT::FMT_BG_RG:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
      //case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
      case latte::SQ_DATA_FORMAT::FMT_8_8_8:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGB8, gl::GL_RGB8_SNORM, gl::GL_RGB8UI, gl::GL_RGB8I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_16:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_RGB16, gl::GL_RGB16_SNORM, gl::GL_RGB16UI, gl::GL_RGB16I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
         return gl::GL_RGB16F;
      case latte::SQ_DATA_FORMAT::FMT_32_32_32:
         return getStorageFormat(numFormat, formatComp, degamma,
            BADFMT, BADFMT, gl::GL_RGB32UI, gl::GL_RGB32I, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
         return gl::GL_RGB32F;
      case latte::SQ_DATA_FORMAT::FMT_BC1:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT);
      case latte::SQ_DATA_FORMAT::FMT_BC2:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT);
      case latte::SQ_DATA_FORMAT::FMT_BC3:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BADFMT, BADFMT, BADFMT, gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
      case latte::SQ_DATA_FORMAT::FMT_BC4:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_COMPRESSED_RED_RGTC1, BADFMT, BADFMT, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_BC5:
         return getStorageFormat(numFormat, formatComp, degamma,
            gl::GL_COMPRESSED_RG_RGTC2, BADFMT, BADFMT, BADFMT, BADFMT);
      //case latte::SQ_DATA_FORMAT::FMT_APC0:
      //case latte::SQ_DATA_FORMAT::FMT_APC1:
      //case latte::SQ_DATA_FORMAT::FMT_APC2:
      //case latte::SQ_DATA_FORMAT::FMT_APC3:
      //case latte::SQ_DATA_FORMAT::FMT_APC4:
      //case latte::SQ_DATA_FORMAT::FMT_APC5:
      //case latte::SQ_DATA_FORMAT::FMT_APC6:
      //case latte::SQ_DATA_FORMAT::FMT_APC7:
      //case latte::SQ_DATA_FORMAT::FMT_CTX1:

      // Depth Types
      case latte::SQ_DATA_FORMAT::FMT_16:
         return gl::GL_DEPTH_COMPONENT16;
      case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
         return gl::GL_DEPTH_COMPONENT32F;
      case latte::SQ_DATA_FORMAT::FMT_8_24:
         return gl::GL_DEPTH24_STENCIL8;
      case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
         // TODO: Maybe find a better solution here?
         return gl::GL_DEPTH24_STENCIL8;
      case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
         return gl::GL_DEPTH32F_STENCIL8;

      default:
         return gl::GL_INVALID_ENUM;
   }
}

SurfaceBuffer *
GLDriver::getSurfaceBuffer(ppcaddr_t baseAddress, uint32_t width, uint32_t height, uint32_t depth,
   latte::SQ_TEX_DIM dim, latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat,
   latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   emuassert(baseAddress);
   emuassert(width);
   emuassert(height);
   emuassert(depth);
   emuassert(width <= 8192);
   emuassert(height <= 8192);

   uint64_t surfaceKey = static_cast<uint64_t>(baseAddress) << 32;
   surfaceKey ^= width ^ height ^ depth ^ dim;
   surfaceKey ^= format ^ numFormat ^ formatComp ^ degamma;

   auto bufferIter = mSurfaces.find(surfaceKey);

   if (bufferIter != mSurfaces.end()) {
      return &bufferIter->second;
   }

   auto insertRes = mSurfaces.emplace(surfaceKey, SurfaceBuffer{});
   auto buffer = &insertRes.first->second;

   auto storageFormat = getStorageFormat(format, numFormat, formatComp, degamma);

   if (storageFormat == gl::GL_INVALID_ENUM) {
      gLog->debug("Skipping texture with unsupported format {} {} {} {}", format, numFormat, formatComp, degamma);
      return nullptr;
   }

   // We need to keep track of the memory region every GPU resource uses
   //  so that we are able to invalidate them as needed.
   buffer->cpuMemStart = baseAddress;
   buffer->cpuMemEnd = baseAddress;

   // Lets track some other useful information
   buffer->dbgInfo.width = width;
   buffer->dbgInfo.height = height;
   buffer->dbgInfo.depth = depth;
   buffer->dbgInfo.dim = dim;
   buffer->dbgInfo.format = format;
   buffer->dbgInfo.numFormat = numFormat;
   buffer->dbgInfo.formatComp = formatComp;
   buffer->dbgInfo.degamma = degamma;

   // TODO: Calculate the true size of the texture instead of cheating
   //  and assuming the texture is 32 bits per pixel.
   auto bytesPerPixel = 4;

   if (!buffer->object) {
      switch (dim) {
      case latte::SQ_TEX_DIM_2D:
         gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         buffer->cpuMemEnd = baseAddress + (width * height * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_2D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_2D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         buffer->cpuMemEnd = baseAddress + (width * height * depth * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_CUBEMAP:
         gl::glCreateTextures(gl::GL_TEXTURE_CUBE_MAP, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         buffer->cpuMemEnd = baseAddress + (width * height * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_1D:
         gl::glCreateTextures(gl::GL_TEXTURE_1D, 1, &buffer->object);
         gl::glTextureStorage1D(buffer->object, 1, storageFormat, width);
         buffer->cpuMemEnd = baseAddress + (width * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_3D:
         gl::glCreateTextures(gl::GL_TEXTURE_3D, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         buffer->cpuMemEnd = baseAddress + (width * height * depth * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_1D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_1D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         buffer->cpuMemEnd = baseAddress + (width * height * bytesPerPixel);
         break;
      case latte::SQ_TEX_DIM_2D_MSAA:
      case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
      default:
         gLog->error("Unsupported texture dim: {}", dim);
         return nullptr;
      }
   }

   return buffer;
}

} // namespace opengl

} // namespace gpu
