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
   auto &active = mActiveDepthBuffer;

   if (db_depth_base.value == mDepthBufferCache.base
    && db_depth_size.value == mDepthBufferCache.size
    && db_depth_info.value == mDepthBufferCache.info) {
      return true;
   }

   mDepthBufferCache.base = db_depth_base.value;
   mDepthBufferCache.size = db_depth_size.value;
   mDepthBufferCache.info = db_depth_info.value;

   if (!db_depth_base.BASE_256B) {
      if (active) {
         // Unbind depth buffer
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, 0, 0);
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_STENCIL_ATTACHMENT, 0, 0);
         active = nullptr;
      }

      return true;
   }

   // Bind depth buffer
   active = getDepthBuffer(db_depth_base, db_depth_size, db_depth_info);
   auto dbFormat = db_depth_info.FORMAT();
   if (dbFormat == latte::DEPTH_8_24
    || dbFormat == latte::DEPTH_8_24_FLOAT
    || dbFormat == latte::DEPTH_X24_8_32_FLOAT) {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_STENCIL_ATTACHMENT, active->active->object, 0);
   } else {
      // Unbind the stencil attachment first so the framebuffer doesn't get
      //  into an inconsistent state, even temporarily (to avoid unnecessary
      //  warnings from apitrace).
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_STENCIL_ATTACHMENT, 0, 0);
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, active->active->object, 0);
   }

   return true;
}

SurfaceBuffer *
GLDriver::getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                         latte::DB_DEPTH_SIZE db_depth_size,
                         latte::DB_DEPTH_INFO db_depth_info)
{
   auto baseAddress = (db_depth_base.BASE_256B << 8) & 0xFFFFF800;
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

   auto buffer = getSurfaceBuffer(baseAddress, pitch, pitch, height, 1, latte::SQ_TEX_DIM_2D, format, numFormat, formatComp, degamma, true, tileMode, true);
   gl::glTextureParameteri(buffer->active->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(buffer->active->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(buffer->active->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glTextureParameteri(buffer->active->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));

   buffer->dirtyAsTexture = false;
   buffer->state = SurfaceUseState::GpuWritten;
   return buffer;
}

} // namespace opengl

} // namespace gpu
