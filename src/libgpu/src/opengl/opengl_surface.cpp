#ifdef DECAF_GL
#include "gpu_config.h"
#include "gpu_memory.h"
#include "gpu_tiling.h"
#include "latte/latte_formats.h"
#include "opengl_driver.h"

#include <common/decaf_assert.h>
#include <common/murmur3.h>
#include <fmt/format.h>
#include <libcpu/mem.h>

namespace opengl
{

static GLenum
getGlFormat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_1:
   case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
   case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
   case latte::SQ_DATA_FORMAT::FMT_BC4:
      return GL_RED;

   case latte::SQ_DATA_FORMAT::FMT_4_4:
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_8_24:
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_24_8:
   case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      return GL_RG;

   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
   case latte::SQ_DATA_FORMAT::FMT_6_5_5:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return GL_RGB;

   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
   case latte::SQ_DATA_FORMAT::FMT_BC1:
   case latte::SQ_DATA_FORMAT::FMT_BC2:
   case latte::SQ_DATA_FORMAT::FMT_BC3:
      return GL_RGBA;

      // case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      // case latte::SQ_DATA_FORMAT::FMT_GB_GR:
      // case latte::SQ_DATA_FORMAT::FMT_BG_RG:
   default:
      decaf_abort(fmt::format("Unimplemented texture format {}", format));
   }
}

static GLenum
getGlDataType(latte::SQ_DATA_FORMAT format,
              latte::SQ_FORMAT_COMP formatComp,
              uint32_t degamma)
{
   auto isSigned = (formatComp == latte::SQ_FORMAT_COMP::SIGNED);

   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return isSigned ? GL_BYTE : GL_UNSIGNED_BYTE;

   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
      return isSigned ? GL_SHORT : GL_UNSIGNED_SHORT;

   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return GL_HALF_FLOAT;

   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return isSigned ? GL_INT : GL_UNSIGNED_INT;

   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return GL_FLOAT;

   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      return GL_UNSIGNED_BYTE_3_3_2;

   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
      return GL_UNSIGNED_SHORT_5_6_5;

   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
      return GL_UNSIGNED_SHORT_5_5_5_1;
   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
      return GL_UNSIGNED_SHORT_1_5_5_5_REV;

   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      return GL_UNSIGNED_SHORT_4_4_4_4;

   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return GL_UNSIGNED_INT_10_10_10_2;
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return GL_UNSIGNED_INT_2_10_10_10_REV;

   case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
      return GL_UNSIGNED_INT_10F_11F_11F_REV;

   case latte::SQ_DATA_FORMAT::FMT_24_8:
   case latte::SQ_DATA_FORMAT::FMT_8_24:
      return GL_UNSIGNED_INT_24_8;

   default:
      decaf_abort(fmt::format("Unimplemented texture format {}", format));
   }
}

static GLenum
getGlCompressedDataType(latte::SQ_DATA_FORMAT format,
                        latte::SQ_FORMAT_COMP formatComp,
                        uint32_t degamma)
{
   auto isSigned = formatComp == latte::SQ_FORMAT_COMP::SIGNED;

   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_BC1:
      decaf_check(!isSigned);
      return degamma ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   case latte::SQ_DATA_FORMAT::FMT_BC2:
      decaf_check(!isSigned);
      return degamma ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   case latte::SQ_DATA_FORMAT::FMT_BC3:
      decaf_check(!isSigned);
      return degamma ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
   case latte::SQ_DATA_FORMAT::FMT_BC4:
      decaf_check(!degamma);
      return isSigned ? GL_COMPRESSED_SIGNED_RED_RGTC1 : GL_COMPRESSED_RED_RGTC1;
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      decaf_check(!degamma);
      return isSigned ? GL_COMPRESSED_SIGNED_RG_RGTC2 : GL_COMPRESSED_RG_RGTC2;
   default:
      decaf_abort(fmt::format("Unimplemented compressed texture format {}", format));
   }
}

static GLenum
getGlStorageFormat(latte::SQ_NUM_FORMAT numFormat,
                   latte::SQ_FORMAT_COMP formatComp,
                   uint32_t degamma,
                   GLenum unorm,
                   GLenum snorm,
                   GLenum uint,
                   GLenum sint,
                   GLenum srgb,
                   GLenum scaled)
{
   if (!degamma) {
      if (numFormat == latte::SQ_NUM_FORMAT::NORM) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return snorm;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return unorm;
         } else {
            return GL_INVALID_ENUM;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::INT) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return sint;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return uint;
         } else {
            return GL_INVALID_ENUM;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return scaled;
         } else {
            return GL_INVALID_ENUM;
         }
      } else {
         return GL_INVALID_ENUM;
      }
   } else {
      if (numFormat == 0 && formatComp == 0) {
         return srgb;
      } else {
         return GL_INVALID_ENUM;
      }
   }
}

static GLenum
getGlStorageFormat(latte::SQ_DATA_FORMAT format,
                   latte::SQ_NUM_FORMAT numFormat,
                   latte::SQ_FORMAT_COMP formatComp,
                   uint32_t degamma,
                   bool isDepthBuffer)
{
   static const auto BADFMT = GL_INVALID_ENUM;
   auto getFormat =
      [=](GLenum unorm, GLenum snorm, GLenum uint, GLenum sint, GLenum srgb) {
         return getGlStorageFormat(numFormat, formatComp, degamma, unorm, snorm, uint, sint, srgb, BADFMT);
      };
   auto getFormatF =
      [=](GLenum scaled) {
         return getGlStorageFormat(numFormat, formatComp, degamma, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, scaled);
      };

   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
      return getFormat(GL_R8, GL_R8_SNORM, GL_R8UI, GL_R8I, GL_SRGB8);
   //case latte::SQ_DATA_FORMAT::FMT_4_4:
   //case latte::SQ_DATA_FORMAT::FMT_3_3_2:
   case latte::SQ_DATA_FORMAT::FMT_16:
      if (isDepthBuffer) {
         return GL_DEPTH_COMPONENT16;
      } else {
         return getFormat(GL_R16, GL_R16_SNORM, GL_R16UI, GL_R16I, BADFMT);
      }
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
      return getFormatF(GL_R16F);
   case latte::SQ_DATA_FORMAT::FMT_8_8:
      return getFormat(GL_RG8, GL_RG8_SNORM, GL_RG8UI, GL_RG8I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
      return getFormat(GL_RGB565, BADFMT, BADFMT, BADFMT, BADFMT);
   //case latte::SQ_DATA_FORMAT::FMT_6_5_5:
   //case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      return getFormat(GL_RGBA4, BADFMT, BADFMT, BADFMT, BADFMT);
   //case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
   case latte::SQ_DATA_FORMAT::FMT_32:
      return getFormat(BADFMT, BADFMT, GL_R32UI, GL_R32I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      if (isDepthBuffer) {
         return GL_DEPTH_COMPONENT32F;
      }

      return getFormatF(GL_R32F);
   case latte::SQ_DATA_FORMAT::FMT_16_16:
      return getFormat(GL_RG16, GL_RG16_SNORM, GL_RG16UI, GL_RG16I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
      return getFormatF(GL_RG16F);
   //case latte::SQ_DATA_FORMAT::FMT_24_8:
   //case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
   //case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
      return getFormatF(GL_R11F_G11F_B10F);
   //case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
      return getFormatF(GL_R11F_G11F_B10F);
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return getFormat(GL_RGB10_A2, BADFMT, GL_RGB10_A2UI, BADFMT, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return getFormat(GL_RGBA8, GL_RGBA8_SNORM, GL_RGBA8UI, GL_RGBA8I, GL_SRGB8_ALPHA8);
   case latte::SQ_DATA_FORMAT::FMT_32_32:
      return getFormat(BADFMT, BADFMT, GL_RG32UI, GL_RG32I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
      return getFormatF(GL_RG32F);
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
      return getFormat(GL_RGBA16, GL_RGBA16_SNORM, GL_RGBA16UI, GL_RGBA16I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return getFormatF(GL_RGBA16F);
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return getFormat(BADFMT, BADFMT, GL_RGBA32UI, GL_RGBA32I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return getFormatF(GL_RGBA32F);
   //case latte::SQ_DATA_FORMAT::FMT_1:
   //case latte::SQ_DATA_FORMAT::FMT_GB_GR:
   //case latte::SQ_DATA_FORMAT::FMT_BG_RG:
   //case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
   //case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
   //case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
      return getFormat(GL_RGB8, GL_RGB8_SNORM, GL_RGB8UI, GL_RGB8I, GL_SRGB8);
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
      return getFormat(GL_RGB16, GL_RGB16_SNORM, GL_RGB16UI, GL_RGB16I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
      return getFormatF(GL_RGB16F);
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
      return getFormat(BADFMT, BADFMT, GL_RGB32UI, GL_RGB32I, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return getFormatF(GL_RGB32F);
   case latte::SQ_DATA_FORMAT::FMT_BC1:
      return getFormat(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BADFMT, BADFMT, BADFMT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT);
   case latte::SQ_DATA_FORMAT::FMT_BC2:
      return getFormat(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, BADFMT, BADFMT, BADFMT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT);
   case latte::SQ_DATA_FORMAT::FMT_BC3:
      return getFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BADFMT, BADFMT, BADFMT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
   case latte::SQ_DATA_FORMAT::FMT_BC4:
      return getFormat(GL_COMPRESSED_RED_RGTC1, GL_COMPRESSED_SIGNED_RED_RGTC1, BADFMT, BADFMT, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      return getFormat(GL_COMPRESSED_RG_RGTC2, GL_COMPRESSED_SIGNED_RG_RGTC2, BADFMT, BADFMT, BADFMT);
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
   case latte::SQ_DATA_FORMAT::FMT_8_24:
   case latte::SQ_DATA_FORMAT::FMT_24_8:
      if (isDepthBuffer) {
         return GL_DEPTH24_STENCIL8;
      }

      return getFormat(GL_DEPTH24_STENCIL8, BADFMT, GL_UNSIGNED_INT_24_8, BADFMT, BADFMT);
   case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      decaf_check(isDepthBuffer);
      return GL_DEPTH32F_STENCIL8;

   default:
      decaf_abort(fmt::format("Invalid surface format {}", format));
   }
}

static GLenum
getGlTarget(latte::SQ_TEX_DIM dim)
{
   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      return GL_TEXTURE_1D;
   case latte::SQ_TEX_DIM::DIM_2D:
      return GL_TEXTURE_2D;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      return GL_TEXTURE_2D_MULTISAMPLE;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      return GL_TEXTURE_2D_ARRAY;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      return GL_TEXTURE_CUBE_MAP;
   case latte::SQ_TEX_DIM::DIM_3D:
      return GL_TEXTURE_3D;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      return GL_TEXTURE_1D_ARRAY;
   default:
      decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
   }
}

static HostSurface *
createHostSurface(phys_addr baseAddress,
                  uint32_t pitch,
                  uint32_t width,
                  uint32_t height,
                  uint32_t depth,
                  uint32_t samples,
                  latte::SQ_TEX_DIM dim,
                  latte::SQ_DATA_FORMAT format,
                  latte::SQ_NUM_FORMAT numFormat,
                  latte::SQ_FORMAT_COMP formatComp,
                  uint32_t degamma,
                  bool isDepthBuffer,
                  bool isDebug)
{
   auto newSurface = new HostSurface();

   auto storageFormat = getGlStorageFormat(format, numFormat, formatComp, degamma, isDepthBuffer);

   if (storageFormat == GL_INVALID_ENUM) {
      decaf_abort(fmt::format("Surface with unsupported format {} {} {} {}", format, numFormat, formatComp, degamma));
   }

   auto target = getGlTarget(dim);
   glCreateTextures(target, 1, &newSurface->object);

   if (isDebug) {
      auto label = fmt::format("surface @ {}", baseAddress);
      glObjectLabel(GL_TEXTURE, newSurface->object, -1, label.c_str());
   }

   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      glTextureStorage1D(newSurface->object, 1, storageFormat, width);
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
      glTextureStorage2D(newSurface->object, 1, storageFormat, width, height);
      break;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      // TODO: Figure out if last parameter should be GL_TRUE or GL_FALSE
      glTextureStorage2DMultisample(newSurface->object, samples, storageFormat, width, height, GL_FALSE);
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      glTextureStorage3D(newSurface->object, 1, storageFormat, width, height, depth);
      break;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      glTextureStorage2D(newSurface->object, 1, storageFormat, width, height);
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
      glTextureStorage3D(newSurface->object, 1, storageFormat, width, height, depth);
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      glTextureStorage2D(newSurface->object, 1, storageFormat, width, height);
      break;
   default:
      decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
   }

   newSurface->width = width;
   newSurface->height = height;
   newSurface->depth = depth;
   newSurface->degamma = degamma;
   newSurface->isDepthBuffer = isDepthBuffer;
   newSurface->swizzleR = GL_RED;
   newSurface->swizzleG = GL_GREEN;
   newSurface->swizzleB = GL_BLUE;
   newSurface->swizzleA = GL_ALPHA;
   newSurface->next = nullptr;
   return newSurface;
}

static void
copyHostSurface(HostSurface* dest,
                HostSurface *source,
                latte::SQ_TEX_DIM dim)
{
   auto copyWidth = std::min(dest->width, source->width);
   auto copyHeight = std::min(dest->height, source->height);
   auto copyDepth = std::min(dest->depth, source->depth);
   auto target = getGlTarget(dim);

   glCopyImageSubData(
      source->object, target, 0, 0, 0, 0,
      dest->object, target, 0, 0, 0, 0,
      copyWidth, copyHeight, copyDepth);
}

static uint32_t
getSurfaceBytes(uint32_t pitch,
                uint32_t height,
                uint32_t depth,
                uint32_t samples,
                latte::SQ_TEX_DIM dim,
                latte::SQ_DATA_FORMAT format)
{
   uint32_t numPixels = 0;

   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      numPixels = pitch;
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
      numPixels = pitch * height;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      numPixels = pitch * height * samples;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      numPixels = pitch * height * depth;
      break;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      numPixels = pitch * height * 6;
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
      numPixels = pitch * height * depth;
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      numPixels = pitch * height;
      break;
   default:
      decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
   }

   if (format >= latte::SQ_DATA_FORMAT::FMT_BC1 && format <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      numPixels /= 4 * 4;
   }

   auto bitsPerPixel = latte::getDataFormatBitsPerElement(format);
   return numPixels * bitsPerPixel / 8;
}

void
GLDriver::uploadSurface(SurfaceBuffer *buffer,
                        phys_addr baseAddress,
                        uint32_t swizzle,
                        uint32_t pitch,
                        uint32_t width,
                        uint32_t height,
                        uint32_t depth,
                        uint32_t samples,
                        latte::SQ_TEX_DIM dim,
                        latte::SQ_DATA_FORMAT format,
                        latte::SQ_NUM_FORMAT numFormat,
                        latte::SQ_FORMAT_COMP formatComp,
                        uint32_t degamma,
                        bool isDepthBuffer,
                        latte::SQ_TILE_MODE tileMode)
{
   auto imagePtr = gpu::internal::translateAddress(baseAddress);
   auto bpp = latte::getDataFormatBitsPerElement(format);
   auto srcWidth = width;
   auto srcHeight = height;
   auto srcPitch = pitch;
   auto uploadWidth = width;
   auto uploadHeight = height;
   auto uploadPitch = width;
   auto uploadDepth = depth;

   if (format >= latte::SQ_DATA_FORMAT::FMT_BC1 && format <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      srcWidth = (srcWidth + 3) / 4;
      srcHeight = (srcHeight + 3) / 4;
      srcPitch = srcPitch / 4;
      uploadWidth = srcWidth * 4;
      uploadHeight = srcHeight * 4;
      uploadPitch = uploadWidth / 4;
   }

   if (dim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
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

      std::vector<uint8_t> untiledImage, untiledMipmap;
      untiledImage.resize(dstImageSize);

      // Untile
      gpu::convertFromTiled(
         untiledImage.data(),
         uploadPitch,
         reinterpret_cast<uint8_t *>(imagePtr),
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
      auto compressed = latte::getDataFormatIsCompressed(format);
      auto target = getGlTarget(dim);
      auto textureDataType = GL_INVALID_ENUM;
      auto textureFormat = getGlFormat(format);
      auto size = untiledImage.size();

      if (compressed) {
         textureDataType = getGlCompressedDataType(format, formatComp, degamma);
      } else {
         textureDataType = getGlDataType(format, formatComp, degamma);
      }

      if (textureDataType == GL_INVALID_ENUM || textureFormat == GL_INVALID_ENUM) {
         decaf_abort(fmt::format("Texture with unsupported format {}", format));
      }

      switch (dim) {
      case latte::SQ_TEX_DIM::DIM_1D:
         if (compressed) {
            glCompressedTextureSubImage1D(buffer->active->object,
               0, /* level */
               0, /* xoffset */
               width,
               textureDataType,
               gsl::narrow_cast<GLsizei>(size),
               untiledImage.data());
         } else {
            glTextureSubImage1D(buffer->active->object,
               0, /* level */
               0, /* xoffset */
               width,
               textureFormat,
               textureDataType,
               untiledImage.data());
         }
         break;
      case latte::SQ_TEX_DIM::DIM_2D:
         if (compressed) {
            glCompressedTextureSubImage2D(buffer->active->object,
               0, /* level */
               0, 0, /* xoffset, yoffset */
               width,
               height,
               textureDataType,
               gsl::narrow_cast<GLsizei>(size),
               untiledImage.data());
         } else {
            glTextureSubImage2D(buffer->active->object,
               0, /* level */
               0, 0, /* xoffset, yoffset */
               width, height,
               textureFormat,
               textureDataType,
               untiledImage.data());
         }
         break;
      case latte::SQ_TEX_DIM::DIM_3D:
         if (compressed) {
            glCompressedTextureSubImage3D(buffer->active->object,
               0, /* level */
               0, 0, 0, /* xoffset, yoffset, zoffset */
               width, height, depth,
               textureDataType,
               gsl::narrow_cast<GLsizei>(size),
               untiledImage.data());
         } else {
            glTextureSubImage3D(buffer->active->object,
               0, /* level */
               0, 0, 0, /* xoffset, yoffset, zoffset */
               width, height, depth,
               textureFormat,
               textureDataType,
               untiledImage.data());
         }
         break;
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         //decaf_check(uploadDepth == 6);
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         if (compressed) {
            glCompressedTextureSubImage3D(buffer->active->object,
               0, /* level */
               0, 0, 0, /* xoffset, yoffset, zoffset */
               width, height, uploadDepth,
               textureDataType,
               gsl::narrow_cast<GLsizei>(size),
               untiledImage.data());
         } else {
            glTextureSubImage3D(buffer->active->object,
               0, /* level */
               0, 0, 0, /* xoffset, yoffset, zoffset */
               width, height, uploadDepth,
               textureFormat,
               textureDataType,
               untiledImage.data());
         }
         break;
      default:
         decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
      }
   }
}

SurfaceBuffer *
GLDriver::getSurfaceBuffer(phys_addr baseAddress,
                           uint32_t pitch,
                           uint32_t width,
                           uint32_t height,
                           uint32_t depth,
                           uint32_t samples,
                           latte::SQ_TEX_DIM dim,
                           latte::SQ_DATA_FORMAT format,
                           latte::SQ_NUM_FORMAT numFormat,
                           latte::SQ_FORMAT_COMP formatComp,
                           uint32_t degamma,
                           bool isDepthBuffer,
                           latte::SQ_TILE_MODE tileMode,
                           bool forWrite,
                           bool discardData)
{
   decaf_check(baseAddress);
   decaf_check(width);
   decaf_check(height);
   decaf_check(depth);
   decaf_check(width <= 8192);
   decaf_check(height <= 8192);
   decaf_check(!(!forWrite && discardData));  // Nonsensical combination

   // Grab the swizzle from this...
   auto swizzle = static_cast<uint32_t>(baseAddress) & 0xFFF;

   // Align the base address according to the GPU logic
   if (tileMode >= latte::SQ_TILE_MODE::TILED_2D_THIN1) {
      baseAddress &= ~(0x800 - 1);
   } else {
      baseAddress &= ~(0x100 - 1);
   }

   // The size key is selected based on which level the dims
   //  are compatible across.  Note that at some point, we may
   //  need to make format not be part of the key as well...
   auto surfaceKey = static_cast<uint64_t>(baseAddress) << 32;
   surfaceKey ^= static_cast<uint64_t>(format) << 22;
   surfaceKey ^= static_cast<uint64_t>(numFormat) << 28;
   surfaceKey ^= static_cast<uint64_t>(formatComp) << 30;

   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      surfaceKey ^= pitch;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
   case latte::SQ_TEX_DIM::DIM_3D:
      surfaceKey ^= pitch ^ height<<16;
      break;
   default:
      decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
   }

   auto &buffer = mSurfaces[surfaceKey];

   if (buffer.active &&
      buffer.active->width == width &&
      buffer.active->height == height &&
      buffer.active->depth == depth &&
      buffer.active->degamma == degamma &&
      buffer.active->isDepthBuffer == isDepthBuffer)
   {
      if (!forWrite && buffer.needUpload) {
         uploadSurface(&buffer, baseAddress, swizzle, pitch, width, height,
                       depth, samples, dim, format, numFormat, formatComp,
                       degamma, isDepthBuffer, tileMode);
         buffer.needUpload = false;
      }

      return &buffer;
   }

   if (!buffer.master) {
      // We are the first user of this surface, lets quickly set it up and
      //  allocate a host surface to use

      // Let's track some other useful information
      buffer.dbgInfo.dim = dim;
      buffer.dbgInfo.format = format;
      buffer.dbgInfo.numFormat = numFormat;
      buffer.dbgInfo.formatComp = formatComp;
      buffer.dbgInfo.degamma = degamma;

      // The memory bounds
      buffer.cpuMemStart = baseAddress;
      buffer.cpuMemEnd = baseAddress + getSurfaceBytes(pitch, height, depth,
                                                       samples, dim, format);

      mResourceMap.addResource(&buffer);

      auto newSurf = createHostSurface(baseAddress, pitch, width, height,
                                       depth, samples, dim, format, numFormat,
                                       formatComp, degamma, isDepthBuffer, mDebug);
      buffer.active = newSurf;
      buffer.master = newSurf;

      if (!forWrite) {
         uploadSurface(&buffer, baseAddress, swizzle, pitch, width, height,
                       depth, samples, dim, format, numFormat, formatComp,
                       degamma, isDepthBuffer, tileMode);
         buffer.needUpload = false;
      }

      return &buffer;
   }

   HostSurface *foundSurface = nullptr;
   HostSurface *newMaster = nullptr;
   HostSurface *newSurface = nullptr;

   if (!foundSurface) {
      // First lets check to see if we already have a surface created
      for (auto surf = buffer.master; surf != nullptr; surf = surf->next) {
         if (surf->width == width &&
            surf->height == height &&
            surf->depth == depth &&
            surf->degamma == degamma &&
            surf->isDepthBuffer == isDepthBuffer) {
            foundSurface = surf;
            break;
         }
      }
   }

   if (!foundSurface) {
      // Lets check if we need to build a new master surface
      auto masterWidth = width;
      auto masterHeight = height;
      auto masterDepth = depth;

      if (buffer.master) {
         masterWidth = std::max(masterWidth, buffer.master->width);
         masterHeight = std::max(masterHeight, buffer.master->height);
         masterDepth = std::max(masterDepth, buffer.master->depth);
      }

      if (!buffer.master ||
          buffer.master->width < masterWidth ||
          buffer.master->height < masterHeight ||
          buffer.master->depth < masterDepth) {
         newMaster = createHostSurface(baseAddress, pitch, masterWidth,
                                       masterHeight, masterDepth, samples, dim,
                                       format, numFormat, formatComp, degamma,
                                       isDepthBuffer, mDebug);

         // Check if the new master we just made matches our size perfectly.
         if (width == masterWidth && height == masterHeight && depth == masterDepth) {
            foundSurface = newMaster;
         }
      }
   }

   if (!foundSurface) {
      // Lets finally just build our perfect surface...
      foundSurface = createHostSurface(baseAddress, pitch, width, height,
                                       depth, samples, dim, format, numFormat,
                                       formatComp, degamma, isDepthBuffer, mDebug);
      newSurface = foundSurface;
   }

   // If the active surface is not the master surface, we first need
   //  to copy that surface up to the master
   if (buffer.active != buffer.master) {
      if (!discardData) {
         copyHostSurface(buffer.master, buffer.active, dim);
      }

      buffer.active = buffer.master;
   }

   // If we allocated a new master, we need to copy the old master to
   //  the new one.  Note that this can cause a HostSurface which only
   //  was acting as a surface to be orphaned for later GC.
   if (newMaster) {
      if (!discardData) {
         copyHostSurface(newMaster, buffer.active, dim);
      }

      buffer.active = newMaster;
   }

   // Check to see if we have finally became the active surface, if we
   //   have not, we need to copy one last time... Lolcopies
   if (buffer.active != foundSurface) {
      if (!discardData) {
         copyHostSurface(foundSurface, buffer.active, dim);
      }

      buffer.active = foundSurface;
   }

   // If we have a new master surface, we need to put it at the top
   //  of the list (so its actually in the .master slot...)
   if (newMaster) {
      newMaster->next = buffer.master;
      buffer.master = newMaster;
   }

   // Other surfaces are just inserted after the master surface.
   if (newSurface) {
      newSurface->next = buffer.master->next;
      buffer.master->next = newSurface;
   }

   // Update the memory bounds to reflect this usage of the texture data
   auto newMemEnd = buffer.cpuMemStart + getSurfaceBytes(pitch, height, depth,
                                                         samples, dim, format);

   if (newMemEnd > buffer.cpuMemEnd) {
      mResourceMap.removeResource(&buffer);
      buffer.cpuMemEnd = newMemEnd;
      mResourceMap.addResource(&buffer);
   }

   if (!forWrite && buffer.needUpload) {
      uploadSurface(&buffer, baseAddress, swizzle, pitch, width, height, depth,
                    samples, dim, format, numFormat, formatComp, degamma,
                    isDepthBuffer, tileMode);
      buffer.needUpload = false;
   }

   return &buffer;
}

void
GLDriver::setSurfaceSwizzle(SurfaceBuffer *surface,
                            GLenum swizzleR,
                            GLenum swizzleG,
                            GLenum swizzleB,
                            GLenum swizzleA)
{
   HostSurface *host = surface->active;
   decaf_check(host);

   if (swizzleR != host->swizzleR
    || swizzleG != host->swizzleG
    || swizzleB != host->swizzleB
    || swizzleA != host->swizzleA) {
      host->swizzleR = swizzleR;
      host->swizzleG = swizzleG;
      host->swizzleB = swizzleB;
      host->swizzleA = swizzleA;

      GLint textureSwizzle[] = {
         static_cast<GLint>(swizzleR),
         static_cast<GLint>(swizzleG),
         static_cast<GLint>(swizzleB),
         static_cast<GLint>(swizzleA),
      };

      glTextureParameteriv(host->object, GL_TEXTURE_SWIZZLE_RGBA, textureSwizzle);
   }
}

} // namespace opengl

#endif // ifdef DECAF_GL
