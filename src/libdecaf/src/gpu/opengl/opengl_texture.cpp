#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/murmur3.h"
#include "decaf_config.h"
#include "gpu/latte_enum_sq.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

namespace gpu
{

namespace opengl
{

static gl::GLenum
getTextureSwizzle(latte::SQ_SEL sel)
{
   switch (sel) {
   case latte::SQ_SEL_X:
      return gl::GL_RED;
   case latte::SQ_SEL_Y:
      return gl::GL_GREEN;
   case latte::SQ_SEL_Z:
      return gl::GL_BLUE;
   case latte::SQ_SEL_W:
      return gl::GL_ALPHA;
   case latte::SQ_SEL_0:
      return gl::GL_ZERO;
   case latte::SQ_SEL_1:
      return gl::GL_ONE;
   default:
      decaf_abort(fmt::format("Unimplemented compressed texture swizzle {}", sel));
   }
}

bool GLDriver::checkActiveTextures()
{
   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_TEX_RESOURCE_WORD1_0 + 4 * resourceOffset);
      auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_TEX_RESOURCE_WORD2_0 + 4 * resourceOffset);
      auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_TEX_RESOURCE_WORD3_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * resourceOffset);
      auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_TEX_RESOURCE_WORD5_0 + 4 * resourceOffset);
      auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_TEX_RESOURCE_WORD6_0 + 4 * resourceOffset);
      auto baseAddress = sq_tex_resource_word2.BASE_ADDRESS() << 8;

      if (!baseAddress) {
         // In debug mode, explicitly unbind the unit so tools like apitrace
         //  don't show lots of unused textures
         if (decaf::config::gpu::debug && mPixelTextureCache[i].surfaceObject != 0) {
            gl::glBindTextureUnit(i, 0);
            mPixelTextureCache[i].surfaceObject = 0;
         }

         continue;
      }

      // Decode resource registers
      auto pitch = (sq_tex_resource_word0.PITCH() + 1) * 8;
      auto width = sq_tex_resource_word0.TEX_WIDTH() + 1;
      auto height = sq_tex_resource_word1.TEX_HEIGHT() + 1;
      auto depth = sq_tex_resource_word1.TEX_DEPTH() + 1;

      auto format = sq_tex_resource_word1.DATA_FORMAT();
      auto tileMode = sq_tex_resource_word0.TILE_MODE();
      auto numFormat = sq_tex_resource_word4.NUM_FORMAT_ALL();
      auto formatComp = sq_tex_resource_word4.FORMAT_COMP_X();
      auto degamma = sq_tex_resource_word4.FORCE_DEGAMMA();
      auto dim = sq_tex_resource_word0.DIM();
      auto swizzle = sq_tex_resource_word2.SWIZZLE() << 8;
      auto isDepthBuffer = !!sq_tex_resource_word0.TILE_TYPE();

      // Check to make sure the incoming swizzle makes sense...  If this assertion ever
      //  fails to hold, it indicates that the pipe bank swizzle bit might be being used
      //  and sending it through swizzle register is how they do that.  I can't find any
      //  case where the swizzle in the registers doesn't match the swizzle in the
      //  baseAddress, but it's confusing why the GPU needs the same information twice.
      decaf_check((baseAddress & 0x7FF) == swizzle);

      // Get the surface
      auto buffer = getSurfaceBuffer(baseAddress, pitch, width, height, depth, dim, format, numFormat, formatComp, degamma, isDepthBuffer, tileMode, false);

      if (buffer->active->object != mPixelTextureCache[i].surfaceObject
       || sq_tex_resource_word4.value != mPixelTextureCache[i].word4) {
         mPixelTextureCache[i].surfaceObject = buffer->active->object;
         mPixelTextureCache[i].word4 = sq_tex_resource_word4.value;

         // Setup texture swizzle
         auto dst_sel_x = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_X());
         auto dst_sel_y = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Y());
         auto dst_sel_z = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Z());
         auto dst_sel_w = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_W());

         gl::GLint textureSwizzle[] = {
            static_cast<gl::GLint>(dst_sel_x),
            static_cast<gl::GLint>(dst_sel_y),
            static_cast<gl::GLint>(dst_sel_z),
            static_cast<gl::GLint>(dst_sel_w),
         };

         gl::glTextureParameteriv(buffer->active->object, gl::GL_TEXTURE_SWIZZLE_RGBA, textureSwizzle);

         // In debug mode, first unbind the unit to remove any textures of
         //  different types (again, to reduce clutter in apitrace etc.)
         if (decaf::config::gpu::debug) {
            gl::glBindTextureUnit(i, 0);
         }

         gl::glBindTextureUnit(i, buffer->active->object);
      }
   }

   return true;
}

static gl::GLenum
getTextureWrap(latte::SQ_TEX_CLAMP clamp)
{
   switch (clamp) {
   case latte::SQ_TEX_WRAP:
      return gl::GL_REPEAT;
   case latte::SQ_TEX_MIRROR:
      return gl::GL_MIRRORED_REPEAT;
   case latte::SQ_TEX_CLAMP_LAST_TEXEL:
      return gl::GL_CLAMP_TO_EDGE;
   case latte::SQ_TEX_MIRROR_ONCE_LAST_TEXEL:
      return gl::GL_MIRROR_CLAMP_TO_EDGE;
   case latte::SQ_TEX_CLAMP_BORDER:
      return gl::GL_CLAMP_TO_BORDER;
   case latte::SQ_TEX_MIRROR_ONCE_BORDER:
      return gl::GL_MIRROR_CLAMP_TO_BORDER_EXT;
   case latte::SQ_TEX_CLAMP_HALF_BORDER:
   case latte::SQ_TEX_MIRROR_ONCE_HALF_BORDER:
   default:
      decaf_abort(fmt::format("Unimplemented texture wrap {}", clamp));
   }
}

static gl::GLenum
getTextureXYFilter(latte::SQ_TEX_XY_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_XY_FILTER_POINT:
      return gl::GL_NEAREST;
   case latte::SQ_TEX_XY_FILTER_BILINEAR:
      return gl::GL_LINEAR;
   default:
      decaf_abort(fmt::format("Unimplemented texture xy filter {}", filter));
   }
}

static gl::GLenum
getTextureCompareFunction(latte::SQ_TEX_DEPTH_COMPARE func)
{
   switch (func) {
   case latte::SQ_TEX_DEPTH_COMPARE_NEVER:
      return gl::GL_NEVER;
   case latte::SQ_TEX_DEPTH_COMPARE_LESS:
      return gl::GL_LESS;
   case latte::SQ_TEX_DEPTH_COMPARE_EQUAL:
      return gl::GL_EQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_LESSEQUAL:
      return gl::GL_LEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_GREATER:
      return gl::GL_GREATER;
   case latte::SQ_TEX_DEPTH_COMPARE_NOTEQUAL:
      return gl::GL_NOTEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_GREATEREQUAL:
      return gl::GL_GEQUAL;
   case latte::SQ_TEX_DEPTH_COMPARE_ALWAYS:
      return gl::GL_ALWAYS;
   default:
      decaf_abort(fmt::format("Unimplemented texture compare function {}", func));
   }
}

bool GLDriver::checkActiveSamplers()
{
   // TODO: Vertex Samplers, Geometry Samplers
   // Pixel samplers id 0...16
   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto sq_tex_sampler_word0 = getRegister<latte::SQ_TEX_SAMPLER_WORD0_N>(latte::Register::SQ_TEX_SAMPLER_WORD0_0 + 4 * (i * 3));
      auto sq_tex_sampler_word1 = getRegister<latte::SQ_TEX_SAMPLER_WORD1_N>(latte::Register::SQ_TEX_SAMPLER_WORD1_0 + 4 * (i * 3));
      auto sq_tex_sampler_word2 = getRegister<latte::SQ_TEX_SAMPLER_WORD2_N>(latte::Register::SQ_TEX_SAMPLER_WORD2_0 + 4 * (i * 3));

      // TODO: is there a sampler bit that indicates this, maybe word2.TYPE?
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + latte::SQ_PS_TEX_RESOURCE_0 + 4 * (i * 7));
      auto depthCompare = !!sq_tex_resource_word0.TILE_TYPE();

      if (sq_tex_sampler_word0.value == mPixelSamplerCache[i].word0
       && sq_tex_sampler_word1.value == mPixelSamplerCache[i].word1
       && sq_tex_sampler_word2.value == mPixelSamplerCache[i].word2
       && depthCompare == mPixelSamplerCache[i].depthCompare) {
         continue;  // No change in sampler state
      }

      mPixelSamplerCache[i].word0 = sq_tex_sampler_word0.value;
      mPixelSamplerCache[i].word1 = sq_tex_sampler_word1.value;
      mPixelSamplerCache[i].word2 = sq_tex_sampler_word2.value;
      mPixelSamplerCache[i].depthCompare = depthCompare;

      if (sq_tex_sampler_word0.value == 0
       && sq_tex_sampler_word1.value == 0
       && sq_tex_sampler_word2.value == 0) {
         gl::glBindSampler(i, 0);
         continue;
      }

      auto &sampler = mPixelSamplers[i];

      if (!sampler.object) {
         gl::glCreateSamplers(1, &sampler.object);
         if (decaf::config::gpu::debug) {
            std::string label = fmt::format("pixel sampler {}", i);
            gl::glObjectLabel(gl::GL_SAMPLER, sampler.object, -1, label.c_str());
         }
      }

      // Texture clamp
      auto clamp_x = getTextureWrap(sq_tex_sampler_word0.CLAMP_X());
      auto clamp_y = getTextureWrap(sq_tex_sampler_word0.CLAMP_Y());
      auto clamp_z = getTextureWrap(sq_tex_sampler_word0.CLAMP_Z());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_S, static_cast<gl::GLint>(clamp_x));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_T, static_cast<gl::GLint>(clamp_y));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_WRAP_R, static_cast<gl::GLint>(clamp_z));

      // Texture filter
      auto xy_min_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MIN_FILTER());
      auto xy_mag_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MAG_FILTER());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MIN_FILTER, static_cast<gl::GLint>(xy_min_filter));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_MAG_FILTER, static_cast<gl::GLint>(xy_mag_filter));

      // Setup border color
      auto border_color_type = sq_tex_sampler_word0.BORDER_COLOR_TYPE();
      std::array<float, 4> colors;

      switch (border_color_type) {
      case latte::SQ_TEX_BORDER_COLOR_TRANS_BLACK:
         colors = { 0.0f, 0.0f, 0.0f, 0.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_OPAQUE_BLACK:
         colors = { 0.0f, 0.0f, 0.0f, 1.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_OPAQUE_WHITE:
         colors = { 1.0f, 1.0f, 1.0f, 0.0f };
         break;
      case latte::SQ_TEX_BORDER_COLOR_REGISTER:
      {
         auto td_ps_sampler_border_red = getRegister<latte::TD_PS_SAMPLER_BORDERN_RED>(latte::Register::TD_PS_SAMPLER_BORDER0_RED + 4 * (i * 4));
         auto td_ps_sampler_border_green = getRegister<latte::TD_PS_SAMPLER_BORDERN_GREEN>(latte::Register::TD_PS_SAMPLER_BORDER0_GREEN + 4 * (i * 4));
         auto td_ps_sampler_border_blue = getRegister<latte::TD_PS_SAMPLER_BORDERN_BLUE>(latte::Register::TD_PS_SAMPLER_BORDER0_BLUE + 4 * (i * 4));
         auto td_ps_sampler_border_alpha = getRegister<latte::TD_PS_SAMPLER_BORDERN_ALPHA>(latte::Register::TD_PS_SAMPLER_BORDER0_ALPHA + 4 * (i * 4));

         colors = {
            td_ps_sampler_border_red.BORDER_RED,
            td_ps_sampler_border_green.BORDER_GREEN,
            td_ps_sampler_border_blue.BORDER_BLUE,
            td_ps_sampler_border_alpha.BORDER_ALPHA,
         };

         break;
      }
      default:
         decaf_abort(fmt::format("Unsupported border_color_type = {}", border_color_type));
      }

      gl::glSamplerParameterfv(sampler.object, gl::GL_TEXTURE_BORDER_COLOR, &colors[0]);

      // Depth compare
      auto mode = depthCompare ? gl::GL_COMPARE_REF_TO_TEXTURE : gl::GL_NONE;
      auto depth_compare_function = getTextureCompareFunction(sq_tex_sampler_word0.DEPTH_COMPARE_FUNCTION());

      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_MODE, static_cast<gl::GLint>(mode));
      gl::glSamplerParameteri(sampler.object, gl::GL_TEXTURE_COMPARE_FUNC, static_cast<gl::GLint>(depth_compare_function));

      // Setup texture LOD
      auto min_lod = sq_tex_sampler_word1.MIN_LOD();
      auto max_lod = sq_tex_sampler_word1.MAX_LOD();
      auto lod_bias = sq_tex_sampler_word1.LOD_BIAS();

      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_MIN_LOD, static_cast<float>(min_lod));
      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_MAX_LOD, static_cast<float>(max_lod));
      gl::glSamplerParameterf(sampler.object, gl::GL_TEXTURE_LOD_BIAS, static_cast<float>(lod_bias));

      // Bind sampler
      gl::glBindSampler(i, sampler.object);
   }

   return true;
}

} // namespace opengl

} // namespace gpu
