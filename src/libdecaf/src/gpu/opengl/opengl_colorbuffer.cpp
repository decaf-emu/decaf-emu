#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkActiveColorBuffer()
{
   for (auto i = 0u; i < mActiveColorBuffers.size(); ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto &active = mActiveColorBuffers[i];

      if (!cb_color_base.BASE_256B) {
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

ColorBuffer *
GLDriver::getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                         latte::CB_COLORN_SIZE cb_color_size,
                         latte::CB_COLORN_INFO cb_color_info)
{
   auto buffer = &mColorBuffers[cb_color_base.BASE_256B ^ cb_color_size.value ^ cb_color_info.value];

   /* TODO: Games can reuse the same GPU memory for multiple color buffers,
            for example, Amiibo Settings uses same BASE_256B for the TV
            and DRC screens which have different cb_color_size.

   if (buffer->object) {
      if (buffer->cb_color_info.value != cb_color_info.value ||
          buffer->cb_color_size.value != cb_color_size.value) {
         // We must recreate color buffer if it has changed
         gl::glDeleteTextures(1, &buffer->object);
         buffer->object = 0;
      }
   }*/

   buffer->cb_color_base = cb_color_base;
   buffer->cb_color_info = cb_color_info;
   buffer->cb_color_size = cb_color_size;

   if (!buffer->object) {
      auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX();
      auto slice_tile_max = cb_color_size.SLICE_TILE_MAX();

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::MicroTileWidth);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

      // Create color buffer
      gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureStorage2D(buffer->object, 1, gl::GL_RGBA8, pitch, height);
   }

   return buffer;
}

} // namespace opengl

} // namespace gpu
