#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkActiveDepthBuffer()
{
   auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
   auto &active = mActiveDepthBuffer;

   if (!db_depth_base.value) {
      if (active) {
         // Unbind depth buffer
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, 0, 0);
         active = nullptr;
      }

      return true;
   }

   // Bind depth buffer
   auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
   auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);
   active = getDepthBuffer(db_depth_base, db_depth_size, db_depth_info);
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, active->object, 0);
   return true;
}

DepthBuffer *
GLDriver::getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                         latte::DB_DEPTH_SIZE db_depth_size,
                         latte::DB_DEPTH_INFO db_depth_info)
{
   auto buffer = &mDepthBuffers[db_depth_base.value];
   buffer->db_depth_base = db_depth_base;

   if (buffer->object) {
      if (buffer->db_depth_info.value != db_depth_info.value ||
          buffer->db_depth_size.value != db_depth_size.value) {
         // We must recreate the depth buffer if it has changed
         gl::glDeleteTextures(1, &buffer->object);
         buffer->object = 0;
      }
   }

   if (!buffer->object) {
      auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX();
      auto slice_tile_max = db_depth_size.SLICE_TILE_MAX();

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::MicroTileWidth);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

      // Create depth buffer
      gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureStorage2D(buffer->object, 1, gl::GL_DEPTH_COMPONENT32F, pitch, height);
   }

   return buffer;
}

} // namespace opengl

} // namespace gpu
