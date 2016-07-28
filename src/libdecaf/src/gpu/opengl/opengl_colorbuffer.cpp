#include "common/decaf_assert.h"
#include "opengl_driver.h"
#include "gpu/latte_enum_cb.h"
#include "gpu/latte_enum_sq.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkActiveColorBuffer()
{
   auto cb_target_mask = getRegister<latte::CB_TARGET_MASK>(latte::Register::CB_TARGET_MASK);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto mask = cb_target_mask.value & cb_shader_mask.value;

   for (auto i = 0u; i < mActiveColorBuffers.size(); ++i, mask >>= 4) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto &active = mActiveColorBuffers[i];

      if (!cb_color_base.BASE_256B || !(mask & 0xF)) {
         if (active) {
            // Unbind color buffer i
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, 0, 0);
            active = nullptr;
         }

         continue;
      } else {
         // Bind color buffer i
         auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
         auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
         active = getColorBuffer(cb_color_base, cb_color_size, cb_color_info);
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, active->object, 0);
      }
   }

   return true;
}

SurfaceBuffer *
GLDriver::getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                         latte::CB_COLORN_SIZE cb_color_size,
                         latte::CB_COLORN_INFO cb_color_info)
{
   auto baseAddress = (cb_color_base.BASE_256B << 8) & 0xFFFFF800;
   auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX();
   auto slice_tile_max = cb_color_size.SLICE_TILE_MAX();

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto cbNumberType = cb_color_info.NUMBER_TYPE();
   auto cbFormat = cb_color_info.FORMAT().get();
   auto format = static_cast<latte::SQ_DATA_FORMAT>(cbFormat);

   auto numFormat = latte::SQ_NUM_FORMAT_NORM;
   auto formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
   auto degamma = 0u;

   switch (cbNumberType) {
   case latte::NUMBER_UNORM:
      numFormat = latte::SQ_NUM_FORMAT_NORM;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::NUMBER_SNORM:
      numFormat = latte::SQ_NUM_FORMAT_NORM;
      formatComp = latte::SQ_FORMAT_COMP_SIGNED;
      degamma = 0;
      break;
   case latte::NUMBER_UINT:
      numFormat = latte::SQ_NUM_FORMAT_INT;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::NUMBER_SINT:
      numFormat = latte::SQ_NUM_FORMAT_INT;
      formatComp = latte::SQ_FORMAT_COMP_SIGNED;
      degamma = 0;
      break;
   case latte::NUMBER_FLOAT:
      numFormat = latte::SQ_NUM_FORMAT_SCALED;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 0;
      break;
   case latte::NUMBER_SRGB:
      numFormat = latte::SQ_NUM_FORMAT_NORM;
      formatComp = latte::SQ_FORMAT_COMP_UNSIGNED;
      degamma = 1;
      break;
   default:
      decaf_abort(fmt::format("Color buffer with unsupported number type {}", cbNumberType));
   }

   auto buffer = getSurfaceBuffer(baseAddress, pitch, height, 1, latte::SQ_TEX_DIM_2D, format, numFormat, formatComp, degamma, false);
   gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));

   buffer->dirtyAsTexture = false;
   buffer->state = SurfaceUseState::GpuWritten;
   return buffer;
}

} // namespace opengl

} // namespace gpu
