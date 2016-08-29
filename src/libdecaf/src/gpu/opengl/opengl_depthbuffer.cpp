#include "common/decaf_assert.h"
#include "opengl_driver.h"
#include "gpu/gpu_utilities.h"
#include "gpu/latte_enum_db.h"
#include "gpu/latte_enum_sq.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkActiveDepthBuffer()
{
   auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
   auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
   auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);
   auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
   auto format = db_depth_info.FORMAT();
   auto z_enable = db_depth_control.Z_ENABLE();
   auto stencil_enable = db_depth_control.STENCIL_ENABLE();

   // We unbind the depth buffer whenever depth testing is disabled so a
   //  size mismatch with color buffers doesn't clip render operations, but
   //  we have to look up the surface regardless of whether depth testing
   //  is enabled; if it's a combined depth/stencil format and stencil
   //  testing is enabled, we still have to bind the buffer to the stencil
   //  attachment.
   SurfaceBuffer *surface;

   if (db_depth_base.BASE_256B()) {
      surface = getDepthBuffer(db_depth_base, db_depth_size, db_depth_info, false);
   } else {
      surface = nullptr;
   }

   auto surfaceObject = surface ? surface->active->object : 0;

   if (format != latte::DEPTH_8_24
    && format != latte::DEPTH_8_24_FLOAT
    && format != latte::DEPTH_X8_24
    && format != latte::DEPTH_X8_24_FLOAT
    && format != latte::DEPTH_X24_8_32_FLOAT) {
      // You cannot bind a stencil buffer if the framebuffer does not support it.
      stencil_enable = false;
   }

   if (surfaceObject != mDepthBufferCache.object
    || z_enable != mDepthBufferCache.depthBound
    || stencil_enable != mDepthBufferCache.stencilBound) {
      if (z_enable && stencil_enable) {
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_STENCIL_ATTACHMENT, surfaceObject, 0);
      } else if (!z_enable && !stencil_enable) {
         if (mDepthBufferCache.depthBound || mDepthBufferCache.stencilBound) {
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
         }
      } else if (z_enable) {
         decaf_check(!stencil_enable);

         if (mDepthBufferCache.stencilBound) {
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_STENCIL_ATTACHMENT, 0, 0);
         }

         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, surfaceObject, 0);
      } else {
         decaf_check(!z_enable && stencil_enable);

         if (mDepthBufferCache.depthBound) {
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, 0, 0);
         }

         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_STENCIL_ATTACHMENT, surfaceObject, 0);
      }

      mFramebufferChanged = true;

      mDepthBufferCache.object = surfaceObject;
      mDepthBufferCache.depthBound = z_enable;
      mDepthBufferCache.stencilBound = stencil_enable;
   }

   return true;
}

SurfaceBuffer *
GLDriver::getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                         latte::DB_DEPTH_SIZE db_depth_size,
                         latte::DB_DEPTH_INFO db_depth_info,
                         bool discardData)
{
   auto baseAddress = (db_depth_base.BASE_256B() << 8) & 0xFFFFF800;
   auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX();
   auto slice_tile_max = db_depth_size.SLICE_TILE_MAX();
   auto dbFormat = db_depth_info.FORMAT();

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto format = latte::FMT_INVALID;
   auto numFormat = latte::SQ_NUM_FORMAT_NORM;
   auto formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
   auto degamma = 0u;

   switch (dbFormat) {
   case latte::DEPTH_16:
      format = latte::SQ_DATA_FORMAT::FMT_16;
      numFormat = latte::SQ_NUM_FORMAT_NORM;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::DEPTH_8_24:
      format = latte::SQ_DATA_FORMAT::FMT_8_24;
      numFormat = latte::SQ_NUM_FORMAT_NORM;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::DEPTH_8_24_FLOAT:
      format = latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT;
      numFormat = latte::SQ_NUM_FORMAT_SCALED;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::DEPTH_32_FLOAT:
      format = latte::SQ_DATA_FORMAT::FMT_32_FLOAT;
      numFormat = latte::SQ_NUM_FORMAT_SCALED;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::DEPTH_X24_8_32_FLOAT:
      format = latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT;
      numFormat = latte::SQ_NUM_FORMAT_SCALED;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   // case latte::DEPTH_X8_24:
   // case latte::DEPTH_X8_24_FLOAT:
   // case latte::DEPTH_INVALID:
   default:
      decaf_abort(fmt::format("Depth buffer with unsupported format {}", dbFormat));
   }

   auto tileMode = getArrayModeTileMode(db_depth_info.ARRAY_MODE());

   auto buffer = getSurfaceBuffer(baseAddress, pitch, pitch, height, 1, 0, latte::SQ_TEX_DIM_2D, format, numFormat, formatComp, degamma, true, tileMode, true, discardData);

   buffer->dirtyMemory = false;
   buffer->needUpload = false;
   buffer->state = SurfaceUseState::GpuWritten;
   return buffer;
}

} // namespace opengl

} // namespace gpu
