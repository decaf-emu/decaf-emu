#ifdef DECAF_GL
#include "opengl_driver.h"
#include "latte/latte_enum_cb.h"
#include "latte/latte_enum_sq.h"
#include "latte/latte_formats.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>
#include <glbinding/gl/gl.h>

namespace opengl
{

bool GLDriver::checkActiveColorBuffer()
{
   auto cb_target_mask = getRegister<latte::CB_TARGET_MASK>(latte::Register::CB_TARGET_MASK);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto cb_color_control = getRegister<latte::CB_COLOR_CONTROL>(latte::Register::CB_COLOR_CONTROL);
   auto cb_shader_control = getRegister<latte::CB_SHADER_CONTROL>(latte::Register::CB_SHADER_CONTROL);

   auto mask = cb_target_mask.value & cb_shader_mask.value;

   auto special_op = cb_color_control.SPECIAL_OP();
   switch (special_op) {
   case latte::CB_SPECIAL_OP::NORMAL:
      break;
   case latte::CB_SPECIAL_OP::DISABLE:
      mask = 0;
      break;
   default:
      decaf_abort(fmt::format("Draw call with unhandled CB_SPECIAL_OP {}", special_op));
      break;
   }

   bool drawBuffersChanged = false;

   for (auto i = 0u; i < mColorBufferCache.size(); ++i, mask >>= 4) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
      auto thisMask = mask & 0xF;
      SurfaceBuffer *surface = nullptr;

      if (cb_shader_control.value & (1 << i)) {
         if (cb_color_base.BASE_256B()) {
            surface = getColorBuffer(cb_color_base, cb_color_size, cb_color_info, false);
         }
      }

      auto surfaceObject = surface ? surface->active->object : 0;

      if (surfaceObject != mColorBufferCache[i].object) {
         mColorBufferCache[i].object = surfaceObject;

         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, surfaceObject, 0);
         mFramebufferChanged = true;

         auto drawBuffer = surfaceObject ? gl::GL_COLOR_ATTACHMENT0 + i : gl::GL_NONE;
         if (mDrawBuffers[i] != drawBuffer) {
            mDrawBuffers[i] = drawBuffer;
            drawBuffersChanged = true;
         }
      }

      if (surfaceObject && thisMask != mColorBufferCache[i].mask) {
         mColorBufferCache[i].mask = thisMask;

         gl::glColorMaski(i,
                          static_cast<gl::GLboolean>(!!(thisMask & (1 << 0))),
                          static_cast<gl::GLboolean>(!!(thisMask & (1 << 1))),
                          static_cast<gl::GLboolean>(!!(thisMask & (1 << 2))),
                          static_cast<gl::GLboolean>(!!(thisMask & (1 << 3))));
      }
   }

   if (drawBuffersChanged) {
      gl::glDrawBuffers(static_cast<gl::GLsizei>(mDrawBuffers.size()), &mDrawBuffers[0]);
   }

   return true;
}

SurfaceBuffer *
GLDriver::getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                         latte::CB_COLORN_SIZE cb_color_size,
                         latte::CB_COLORN_INFO cb_color_info,
                         bool discardData)
{
   auto baseAddress = phys_addr { (cb_color_base.BASE_256B() << 8) & 0xFFFFF800 };
   auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX();
   auto slice_tile_max = cb_color_size.SLICE_TILE_MAX();

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto cbNumberType = cb_color_info.NUMBER_TYPE();
   auto cbFormat = cb_color_info.FORMAT();
   auto format = static_cast<latte::SQ_DATA_FORMAT>(cbFormat);

   auto numFormat = latte::SQ_NUM_FORMAT::NORM;
   auto formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   auto degamma = 0u;

   switch (cbNumberType) {
   case latte::CB_NUMBER_TYPE::UNORM:
      numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SNORM:
      numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatComp = latte::SQ_FORMAT_COMP::SIGNED;
      degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::UINT:
      numFormat = latte::SQ_NUM_FORMAT::INT;
      formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SINT:
      numFormat = latte::SQ_NUM_FORMAT::INT;
      formatComp = latte::SQ_FORMAT_COMP::SIGNED;
      degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::FLOAT:
      numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SRGB:
      numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      degamma = 1;
      break;
   default:
      decaf_abort(fmt::format("Color buffer with unsupported number type {}", cbNumberType));
   }

   auto tileMode = latte::getArrayModeTileMode(cb_color_info.ARRAY_MODE());
   auto buffer = getSurfaceBuffer(baseAddress, pitch, pitch, height, 1, 0,
                                  latte::SQ_TEX_DIM::DIM_2D, format, numFormat,
                                  formatComp, degamma, false, tileMode, true,
                                  discardData);
   buffer->dirtyMemory = false;
   buffer->needUpload = false;
   buffer->state = SurfaceUseState::GpuWritten;
   return buffer;
}

} // namespace opengl

#endif // ifdef DECAF_GL
