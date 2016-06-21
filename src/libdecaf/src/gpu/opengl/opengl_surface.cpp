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
      // GX2_ENUM_VALUE(UNORM_R4_G4, 0x02)
      // GX2_ENUM_VALUE(UNORM_A1_B5_G5_R5, 0x0c)
      // GX2_ENUM_VALUE(UNORM_A2_B10_G10_R10, 0x01b)
      // GX2_ENUM_VALUE(UNORM_R10_G10_B10_A2, 0x019)
      // GX2_ENUM_VALUE(UNORM_NV12, 0x081)
   case GX2SurfaceFormat::UNORM_R24_X8:
      return gl::GL_DEPTH24_STENCIL8;
   case GX2SurfaceFormat::UNORM_R4_G4_B4_A4:
      return gl::GL_RGBA4;
   case GX2SurfaceFormat::UNORM_R8:
      return gl::GL_R8;
   case GX2SurfaceFormat::UNORM_R8_G8:
      return gl::GL_RG8;
   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
      return gl::GL_RGBA8;
   case GX2SurfaceFormat::UNORM_R16:
      return gl::GL_R16;
   case GX2SurfaceFormat::UNORM_R16_G16:
      return gl::GL_RG16;
   case GX2SurfaceFormat::UNORM_R16_G16_B16_A16:
      return gl::GL_RGBA16;
   case GX2SurfaceFormat::UNORM_R5_G6_B5:
      return gl::GL_RGB565;
   case GX2SurfaceFormat::UNORM_R5_G5_B5_A1:
      return gl::GL_RGB5_A1;
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
      return gl::GL_RGB10_A2;
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

      // GX2_ENUM_VALUE(UINT_A2_B10_G10_R10, 0x11b)
      // GX2_ENUM_VALUE(UINT_X24_G8, 0x111)
      // GX2_ENUM_VALUE(UINT_G8_X24, 0x11c)
   case GX2SurfaceFormat::UINT_R8:
      return gl::GL_R8UI;
   case GX2SurfaceFormat::UINT_R8_G8:
      return gl::GL_RG8UI;
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
      return gl::GL_RGBA8UI;
   case GX2SurfaceFormat::UINT_R16:
      return gl::GL_R16UI;
   case GX2SurfaceFormat::UINT_R16_G16:
      return gl::GL_RG16UI;
   case GX2SurfaceFormat::UINT_R16_G16_B16_A16:
      return gl::GL_RGBA16UI;
   case GX2SurfaceFormat::UINT_R32:
      return gl::GL_R32UI;
   case GX2SurfaceFormat::UINT_R32_G32:
      return gl::GL_RG32UI;
   case GX2SurfaceFormat::UINT_R32_G32_B32_A32:
      return gl::GL_RGBA32UI;
   case GX2SurfaceFormat::UINT_R10_G10_B10_A2:
      return gl::GL_RGB10_A2UI;

      // GX2_ENUM_VALUE(SNORM_R10_G10_B10_A2, 0x219)
   case GX2SurfaceFormat::SNORM_R8:
      return gl::GL_R8_SNORM;
   case GX2SurfaceFormat::SNORM_R8_G8:
      return gl::GL_RG8_SNORM;
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
      return gl::GL_RGBA8_SNORM;
   case GX2SurfaceFormat::SNORM_R16:
      return gl::GL_R16_SNORM;
   case GX2SurfaceFormat::SNORM_R16_G16:
      return gl::GL_RG16_SNORM;
   case GX2SurfaceFormat::SNORM_R16_G16_B16_A16:
      return gl::GL_RGBA16_SNORM;
   case GX2SurfaceFormat::SNORM_BC4:
      return gl::GL_COMPRESSED_SIGNED_RED_RGTC1;
   case GX2SurfaceFormat::SNORM_BC5:
      return gl::GL_COMPRESSED_SIGNED_RG_RGTC2;

      // GX2_ENUM_VALUE(SINT_R10_G10_B10_A2, 0x319)
   case GX2SurfaceFormat::SINT_R8:
      return gl::GL_R8I;
   case GX2SurfaceFormat::SINT_R8_G8:
      return gl::GL_RG8I;
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
      return gl::GL_RGBA8I;
   case GX2SurfaceFormat::SINT_R16:
      return gl::GL_R16I;
   case GX2SurfaceFormat::SINT_R16_G16:
      return gl::GL_RG16I;
   case GX2SurfaceFormat::SINT_R16_G16_B16_A16:
      return gl::GL_RGBA16I;
   case GX2SurfaceFormat::SINT_R32:
      return gl::GL_R32I;
   case GX2SurfaceFormat::SINT_R32_G32:
      return gl::GL_RG32I;
   case GX2SurfaceFormat::SINT_R32_G32_B32_A32:
      return gl::GL_RGBA32I;

   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
      return gl::GL_SRGB8_ALPHA8;
   case GX2SurfaceFormat::SRGB_BC1:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
   case GX2SurfaceFormat::SRGB_BC2:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
   case GX2SurfaceFormat::SRGB_BC3:
      return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;

      // GX2_ENUM_VALUE(FLOAT_D24_S8, 0x811)
      // GX2_ENUM_VALUE(FLOAT_X8_X24, 0x81c)
   case GX2SurfaceFormat::FLOAT_R32:
      return gl::GL_DEPTH_COMPONENT32F;
   case GX2SurfaceFormat::FLOAT_R32_G32:
      return gl::GL_RG32F;
   case GX2SurfaceFormat::FLOAT_R32_G32_B32_A32:
      return gl::GL_RGBA32F;
   case GX2SurfaceFormat::FLOAT_R16:
      return gl::GL_R16F;
   case GX2SurfaceFormat::FLOAT_R16_G16:
      return gl::GL_RG16F;
   case GX2SurfaceFormat::FLOAT_R16_G16_B16_A16:
      return gl::GL_RGBA16F;
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
      return gl::GL_R11F_G11F_B10F;
   default:
      gLog->debug("getStorageFormat: Unimplemented texture storage format 0x{:X}", value);
      return gl::GL_INVALID_ENUM;
   }
}

SurfaceBuffer *
GLDriver::getSurfaceBuffer(ppcaddr_t baseAddress, uint32_t width, uint32_t height, uint32_t depth,
   latte::SQ_TEX_DIM dim, latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat,
   latte::SQ_FORMAT_COMP formatComp, uint32_t degamma)
{
   uint64_t surfaceKey = static_cast<uint64_t>(baseAddress) << 32;
   surfaceKey ^= width ^ height ^ depth ^ dim;
   surfaceKey ^= format ^ numFormat ^ formatComp ^ degamma;
   auto buffer = &mSurfaces[surfaceKey];

   auto storageFormat = getStorageFormat(format, numFormat, formatComp, degamma);
   if (storageFormat == gl::GL_INVALID_ENUM) {
      gLog->debug("Skipping texture with unsupported format {} {} {} {}", format, numFormat, formatComp, degamma);
      return nullptr;
   }

   if (!buffer->object) {
      switch (dim) {
      case latte::SQ_TEX_DIM_2D:
         gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         break;
      case latte::SQ_TEX_DIM_2D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_2D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         break;
      case latte::SQ_TEX_DIM_CUBEMAP:
         gl::glCreateTextures(gl::GL_TEXTURE_CUBE_MAP, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         break;
      case latte::SQ_TEX_DIM_1D:
         gl::glCreateTextures(gl::GL_TEXTURE_1D, 1, &buffer->object);
         gl::glTextureStorage1D(buffer->object, 1, storageFormat, width);
         break;
      case latte::SQ_TEX_DIM_3D:
         gl::glCreateTextures(gl::GL_TEXTURE_3D, 1, &buffer->object);
         gl::glTextureStorage3D(buffer->object, 1, storageFormat, width, height, depth);
         break;
      case latte::SQ_TEX_DIM_1D_ARRAY:
         gl::glCreateTextures(gl::GL_TEXTURE_1D_ARRAY, 1, &buffer->object);
         gl::glTextureStorage2D(buffer->object, 1, storageFormat, width, height);
         break;
      case latte::SQ_TEX_DIM_2D_MSAA:
      case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
         gLog->error("Unsupported texture dim: {}", dim);
         return nullptr;
      }
   }

   return buffer;
}

} // namespace opengl

} // namespace gpu
