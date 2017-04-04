#ifndef DECAF_NOGL

#include <common/decaf_assert.h>
#include <common/gl.h>
#include <common/log.h>
#include <common/murmur3.h>
#include "decaf_config.h"
#include "gpu/latte_enum_sq.h"
#include "opengl_driver.h"
#include <gsl.h>

namespace gpu
{

namespace opengl
{

static GLenum
getTextureSwizzle(latte::SQ_SEL sel)
{
   switch (sel) {
   case latte::SQ_SEL::SEL_X:
      return GL_RED;
   case latte::SQ_SEL::SEL_Y:
      return GL_GREEN;
   case latte::SQ_SEL::SEL_Z:
      return GL_BLUE;
   case latte::SQ_SEL::SEL_W:
      return GL_ALPHA;
   case latte::SQ_SEL::SEL_0:
      return GL_ZERO;
   case latte::SQ_SEL::SEL_1:
      return GL_ONE;
   default:
      decaf_abort(fmt::format("Unimplemented compressed texture swizzle {}", sel));
   }
}

bool GLDriver::checkActiveTextures()
{
   for (auto i = 0; i < latte::MaxTextures; ++i) {
      if (!mActiveShader
       || !mActiveShader->pixel
       || mActiveShader->pixel->samplerUsage[i] == glsl2::SamplerUsage::Invalid) {
         // In debug mode, explicitly unbind the unit so tools like apitrace
         //  don't show lots of unused textures
         if (decaf::config::gpu::debug && mPixelTextureCache[i].surfaceObject != 0) {
            glBindTextureUnit(i, 0);
            mPixelTextureCache[i].surfaceObject = 0;
         }

         continue;
      }

      auto resourceOffset = (latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_TEX_RESOURCE_WORD1_0 + 4 * resourceOffset);
      auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_TEX_RESOURCE_WORD2_0 + 4 * resourceOffset);
      auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_TEX_RESOURCE_WORD3_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * resourceOffset);
      auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_TEX_RESOURCE_WORD5_0 + 4 * resourceOffset);
      auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_TEX_RESOURCE_WORD6_0 + 4 * resourceOffset);
      auto baseAddress = sq_tex_resource_word2.BASE_ADDRESS() << 8;

      if (!baseAddress) {
         if (mPixelTextureCache[i].surfaceObject != 0) {
            glBindTextureUnit(i, 0);
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
      auto samples = 0u;

      if (dim == latte::SQ_TEX_DIM::DIM_2D_MSAA || dim == latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA) {
         samples = 1 << sq_tex_resource_word5.LAST_LEVEL();
      }

      // Check to make sure the incoming swizzle makes sense...  If this assertion ever
      //  fails to hold, it indicates that the pipe bank swizzle bit might be being used
      //  and sending it through swizzle register is how they do that.  I can't find any
      //  case where the swizzle in the registers doesn't match the swizzle in the
      //  baseAddress, but it's confusing why the GPU needs the same information twice.
      decaf_check((baseAddress & 0x7FF) == swizzle);

      // Get the surface
      auto buffer = getSurfaceBuffer(baseAddress, pitch, width, height, depth, samples, dim, format, numFormat, formatComp, degamma, isDepthBuffer, tileMode, false, false);

      if (buffer->active->object != mPixelTextureCache[i].surfaceObject
       || sq_tex_resource_word4.value != mPixelTextureCache[i].word4) {
         mPixelTextureCache[i].surfaceObject = buffer->active->object;
         mPixelTextureCache[i].word4 = sq_tex_resource_word4.value;

         // Setup texture swizzle
         auto dst_sel_x = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_X());
         auto dst_sel_y = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Y());
         auto dst_sel_z = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_Z());
         auto dst_sel_w = getTextureSwizzle(sq_tex_resource_word4.DST_SEL_W());

         setSurfaceSwizzle(buffer, dst_sel_x, dst_sel_y, dst_sel_z, dst_sel_w);

         // In debug mode, first unbind the unit to remove any textures of
         //  different types (again, to reduce clutter in apitrace etc.)
         if (decaf::config::gpu::debug) {
            glBindTextureUnit(i, 0);
         }

         glBindTextureUnit(i, buffer->active->object);
      }
   }

   return true;
}

static GLenum
getTextureWrap(latte::SQ_TEX_CLAMP clamp)
{
   switch (clamp) {
   case latte::SQ_TEX_CLAMP::WRAP:
      return GL_REPEAT;
   case latte::SQ_TEX_CLAMP::MIRROR:
      return GL_MIRRORED_REPEAT;
   case latte::SQ_TEX_CLAMP::CLAMP_LAST_TEXEL:
      return GL_CLAMP_TO_EDGE;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_LAST_TEXEL:
      return GL_MIRROR_CLAMP_TO_EDGE;
   case latte::SQ_TEX_CLAMP::CLAMP_BORDER:
      return GL_CLAMP_TO_BORDER;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_BORDER:
      return GL_MIRROR_CLAMP_TO_BORDER_EXT;
   case latte::SQ_TEX_CLAMP::CLAMP_HALF_BORDER:
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_HALF_BORDER:
   default:
      decaf_abort(fmt::format("Unimplemented texture wrap {}", clamp));
   }
}

static GLenum
getTextureXYFilter(latte::SQ_TEX_XY_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_XY_FILTER::POINT:
      return GL_NEAREST;
   case latte::SQ_TEX_XY_FILTER::BILINEAR:
      return GL_LINEAR;
   default:
      decaf_abort(fmt::format("Unimplemented texture xy filter {}", filter));
   }
}

static GLenum
getTextureCompareFunction(latte::REF_FUNC func)
{
   switch (func) {
   case latte::REF_FUNC::NEVER:
      return GL_NEVER;
   case latte::REF_FUNC::LESS:
      return GL_LESS;
   case latte::REF_FUNC::EQUAL:
      return GL_EQUAL;
   case latte::REF_FUNC::LESS_EQUAL:
      return GL_LEQUAL;
   case latte::REF_FUNC::GREATER:
      return GL_GREATER;
   case latte::REF_FUNC::NOT_EQUAL:
      return GL_NOTEQUAL;
   case latte::REF_FUNC::GREATER_EQUAL:
      return GL_GEQUAL;
   case latte::REF_FUNC::ALWAYS:
      return GL_ALWAYS;
   default:
      decaf_abort(fmt::format("Unimplemented texture compare function {}", func));
   }
}

bool GLDriver::checkActiveSamplers()
{
   // TODO: Vertex Samplers, Geometry Samplers
   // Pixel samplers id 0...15
   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto usage = mActiveShader && mActiveShader->pixel ? mActiveShader->pixel->samplerUsage[i] : glsl2::SamplerUsage::Invalid;

      if (usage == glsl2::SamplerUsage::Invalid) {
         if (mPixelSamplerCache[i].usage != glsl2::SamplerUsage::Invalid) {
            mPixelSamplerCache[i].usage = glsl2::SamplerUsage::Invalid;
            glBindSampler(i, 0);
         }
         continue;
      }

      auto sq_tex_sampler_word0 = getRegister<latte::SQ_TEX_SAMPLER_WORD0_N>(latte::Register::SQ_TEX_SAMPLER_WORD0_0 + 4 * (i * 3));
      auto sq_tex_sampler_word1 = getRegister<latte::SQ_TEX_SAMPLER_WORD1_N>(latte::Register::SQ_TEX_SAMPLER_WORD1_0 + 4 * (i * 3));
      auto sq_tex_sampler_word2 = getRegister<latte::SQ_TEX_SAMPLER_WORD2_N>(latte::Register::SQ_TEX_SAMPLER_WORD2_0 + 4 * (i * 3));
      auto &sampler = mPixelSamplers[i];

      // Create sampler object if this is the first time we're using it
      if (!sampler.object) {
         glCreateSamplers(1, &sampler.object);

         if (decaf::config::gpu::debug) {
            auto label = fmt::format("pixel sampler {}", i);
            glObjectLabel(GL_SAMPLER, sampler.object, -1, label.c_str());
         }
      }

      // Bind sampler if necessary
      if (mPixelSamplerCache[i].usage != usage) {
         if (mPixelSamplerCache[i].usage == glsl2::SamplerUsage::Invalid) {
            glBindSampler(i, sampler.object);
         }

         mPixelSamplerCache[i].usage = usage;
      }

      // Texture clamp
      auto clamp_x = getTextureWrap(sq_tex_sampler_word0.CLAMP_X());
      auto clamp_y = getTextureWrap(sq_tex_sampler_word0.CLAMP_Y());
      auto clamp_z = getTextureWrap(sq_tex_sampler_word0.CLAMP_Z());

      if (mPixelSamplerCache[i].wrapS != clamp_x) {
         mPixelSamplerCache[i].wrapS = clamp_x;
         glSamplerParameteri(sampler.object, GL_TEXTURE_WRAP_S, static_cast<GLint>(clamp_x));
      }

      if (mPixelSamplerCache[i].wrapT != clamp_y) {
         mPixelSamplerCache[i].wrapT = clamp_y;
         glSamplerParameteri(sampler.object, GL_TEXTURE_WRAP_T, static_cast<GLint>(clamp_y));
      }

      if (mPixelSamplerCache[i].wrapR != clamp_z) {
         mPixelSamplerCache[i].wrapR = clamp_z;
         glSamplerParameteri(sampler.object, GL_TEXTURE_WRAP_R, static_cast<GLint>(clamp_z));
      }

      // Texture filter
      auto xy_min_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MIN_FILTER());
      auto xy_mag_filter = getTextureXYFilter(sq_tex_sampler_word0.XY_MAG_FILTER());

      if (mPixelSamplerCache[i].minFilter != xy_min_filter) {
         mPixelSamplerCache[i].minFilter = xy_min_filter;
         glSamplerParameteri(sampler.object, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(xy_min_filter));
      }

      if (mPixelSamplerCache[i].magFilter != xy_mag_filter) {
         mPixelSamplerCache[i].magFilter = xy_mag_filter;
         glSamplerParameteri(sampler.object, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(xy_mag_filter));
      }

      // Setup border color
      auto border_color_type = sq_tex_sampler_word0.BORDER_COLOR_TYPE();

      // Skip the color array setup as well as the GL call if we know we
      //  don't need it (we have to check in any case for the REGISTER type)
      if (mPixelSamplerCache[i].borderColorType != border_color_type
       || border_color_type == latte::SQ_TEX_BORDER_COLOR::REGISTER) {
         std::array<float, 4> colors;

         switch (border_color_type) {
         case latte::SQ_TEX_BORDER_COLOR::TRANS_BLACK:
            colors = { 0.0f, 0.0f, 0.0f, 0.0f };
            break;
         case latte::SQ_TEX_BORDER_COLOR::OPAQUE_BLACK:
            colors = { 0.0f, 0.0f, 0.0f, 1.0f };
            break;
         case latte::SQ_TEX_BORDER_COLOR::OPAQUE_WHITE:
            colors = { 1.0f, 1.0f, 1.0f, 0.0f };
            break;
         case latte::SQ_TEX_BORDER_COLOR::REGISTER:
         {
            auto td_ps_sampler_border_red = getRegister<latte::TD_PS_SAMPLER_BORDERN_RED>(latte::Register::TD_PS_SAMPLER_BORDER0_RED + 4 * (i * 4));
            auto td_ps_sampler_border_green = getRegister<latte::TD_PS_SAMPLER_BORDERN_GREEN>(latte::Register::TD_PS_SAMPLER_BORDER0_GREEN + 4 * (i * 4));
            auto td_ps_sampler_border_blue = getRegister<latte::TD_PS_SAMPLER_BORDERN_BLUE>(latte::Register::TD_PS_SAMPLER_BORDER0_BLUE + 4 * (i * 4));
            auto td_ps_sampler_border_alpha = getRegister<latte::TD_PS_SAMPLER_BORDERN_ALPHA>(latte::Register::TD_PS_SAMPLER_BORDER0_ALPHA + 4 * (i * 4));

            colors = {
               td_ps_sampler_border_red.BORDER_RED(),
               td_ps_sampler_border_green.BORDER_GREEN(),
               td_ps_sampler_border_blue.BORDER_BLUE(),
               td_ps_sampler_border_alpha.BORDER_ALPHA(),
            };

            break;
         }
         default:
            decaf_abort(fmt::format("Impossible border_color_type = {}", border_color_type));
         }

         if (mPixelSamplerCache[i].borderColorType != border_color_type
             || (border_color_type == latte::SQ_TEX_BORDER_COLOR::REGISTER
                 && (mPixelSamplerCache[i].borderColorValue[0] != colors[0]
                  || mPixelSamplerCache[i].borderColorValue[1] != colors[1]
                  || mPixelSamplerCache[i].borderColorValue[2] != colors[2]
                  || mPixelSamplerCache[i].borderColorValue[3] != colors[3]))) {
            mPixelSamplerCache[i].borderColorType = border_color_type;
            mPixelSamplerCache[i].borderColorValue = colors;

            glSamplerParameterfv(sampler.object, GL_TEXTURE_BORDER_COLOR, &colors[0]);
         }
      }


      // Depth compare
      auto mode = usage == glsl2::SamplerUsage::Shadow ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE;

      if (mPixelSamplerCache[i].depthCompareMode != mode) {
         mPixelSamplerCache[i].depthCompareMode = mode;
         glSamplerParameteri(sampler.object, GL_TEXTURE_COMPARE_MODE, static_cast<GLint>(mode));
      }

      if (mode != GL_NONE) {
         auto depth_compare_function = getTextureCompareFunction(sq_tex_sampler_word0.DEPTH_COMPARE_FUNCTION());

         if (mPixelSamplerCache[i].depthCompareFunc != depth_compare_function) {
            mPixelSamplerCache[i].depthCompareFunc = depth_compare_function;
            glSamplerParameteri(sampler.object, GL_TEXTURE_COMPARE_FUNC, static_cast<GLint>(depth_compare_function));
         }
      }

      // Setup texture LOD
      auto min_lod = sq_tex_sampler_word1.MIN_LOD();
      auto max_lod = sq_tex_sampler_word1.MAX_LOD();
      auto lod_bias = sq_tex_sampler_word1.LOD_BIAS();

      if (mPixelSamplerCache[i].minLod != min_lod) {
         mPixelSamplerCache[i].minLod = min_lod;
         glSamplerParameterf(sampler.object, GL_TEXTURE_MIN_LOD, static_cast<float>(min_lod));
      }

      if (mPixelSamplerCache[i].maxLod != max_lod) {
         mPixelSamplerCache[i].maxLod = max_lod;
         glSamplerParameterf(sampler.object, GL_TEXTURE_MAX_LOD, static_cast<float>(max_lod));
      }

      if (mPixelSamplerCache[i].lodBias != lod_bias) {
         mPixelSamplerCache[i].lodBias = lod_bias;
         glSamplerParameterf(sampler.object, GL_TEXTURE_LOD_BIAS, static_cast<float>(lod_bias));
      }
   }

   return true;
}

} // namespace opengl

} // namespace gpu

#endif // DECAF_NOGL
