#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>
#include <fstream>

#include "platform/platform_ui.h"
#include "gpu/commandqueue.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_reader.h"
#include "gpu/latte_registers.h"
#include "gpu/latte_format.h"
#include "opengl_driver.h"
#include "utils/log.h"

namespace gpu
{

namespace opengl
{

bool GLDriver::checkReadyDraw()
{
   if (!checkActiveColorBuffer()) {
      gLog->warn("Skipping draw with invalid color buffer.");
      return false;
   }

   if (!checkActiveDepthBuffer()) {
      gLog->warn("Skipping draw with invalid depth buffer.");
      return false;
   }

   if (!checkActiveShader()) {
      gLog->warn("Skipping draw with invalid shader.");
      return false;
   }

   if (!checkActiveAttribBuffers()) {
      gLog->warn("Skipping draw with invalid attribs.");
      return false;
   }

   if (!checkActiveUniforms()) {
      gLog->warn("Skipping draw with invalid uniforms.");
      return false;
   }

   if (!checkActiveTextures()) {
      gLog->warn("Skipping draw with invalid textures.");
      return false;
   }

   if (!checkActiveSamplers()) {
      gLog->warn("Skipping draw with invalid samplers.");
      return false;
   }

   if (!checkViewport()) {
      gLog->warn("Skipping draw with invalid viewport.");
      return false;
   }

   return true;
}

ColorBuffer *
GLDriver::getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                         latte::CB_COLORN_SIZE cb_color_size,
                         latte::CB_COLORN_INFO cb_color_info)
{
   auto buffer = &mColorBuffers[cb_color_base.value ^ cb_color_size.value ^ cb_color_info.value];
   buffer->cb_color_base = cb_color_base;

   if (!buffer->object) {
      auto format = cb_color_info.FORMAT;
      auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX;
      auto slice_tile_max = cb_color_size.SLICE_TILE_MAX;

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::tile_width);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::tile_width * latte::tile_height)) / pitch);

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

DepthBuffer *
GLDriver::getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                         latte::DB_DEPTH_SIZE db_depth_size,
                         latte::DB_DEPTH_INFO db_depth_info)
{
   auto buffer = &mDepthBuffers[db_depth_base.value ^ db_depth_size.value ^ db_depth_info.value];
   buffer->db_depth_base = db_depth_base;

   if (!buffer->object) {
      auto format = db_depth_info.FORMAT;
      auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX;
      auto slice_tile_max = db_depth_size.SLICE_TILE_MAX;

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::tile_width);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::tile_width * latte::tile_height)) / pitch);

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

bool GLDriver::checkActiveColorBuffer()
{
   for (auto i = 0u; i < mActiveColorBuffers.size(); ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto &active = mActiveColorBuffers[i];

      if (!cb_color_base.BASE_256B) {
         if (active) {
            // Unbind active
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, 0, 0);
            active = nullptr;
         }

         continue;
      }

      auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
      active = getColorBuffer(cb_color_base, cb_color_size, cb_color_info);

      // Bind color buffer
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, active->object, 0);
   }

   return true;
}

bool GLDriver::checkActiveDepthBuffer()
{
   auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
   auto &active = mActiveDepthBuffer;

   if (!db_depth_base.BASE_256B) {
      if (active) {
         // Unbind active
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, 0, 0);
         active = nullptr;
      }

      return true;
   }

   auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
   auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);
   active = getDepthBuffer(db_depth_base, db_depth_size, db_depth_info);

   // Bind depth buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, active->object, 0);
   return true;
}

bool
GLDriver::checkViewport()
{
   if (mViewportDirty) {
      auto pa_cl_vport_xscale = getRegister<latte::PA_CL_VPORT_XSCALE_N>(latte::Register::PA_CL_VPORT_XSCALE_0);
      auto pa_cl_vport_xoffset = getRegister<latte::PA_CL_VPORT_XOFFSET_N>(latte::Register::PA_CL_VPORT_XOFFSET_0);
      auto pa_cl_vport_yscale = getRegister<latte::PA_CL_VPORT_YSCALE_N>(latte::Register::PA_CL_VPORT_YSCALE_0);
      auto pa_cl_vport_yoffset = getRegister<latte::PA_CL_VPORT_YOFFSET_N>(latte::Register::PA_CL_VPORT_YOFFSET_0);
      auto pa_cl_vport_zscale = getRegister<latte::PA_CL_VPORT_ZSCALE_N>(latte::Register::PA_CL_VPORT_ZSCALE_0);
      auto pa_cl_vport_zoffset = getRegister<latte::PA_CL_VPORT_ZOFFSET_N>(latte::Register::PA_CL_VPORT_ZOFFSET_0);
      auto pa_sc_vport_zmin = getRegister<latte::PA_SC_VPORT_ZMIN_N>(latte::Register::PA_SC_VPORT_ZMIN_0);
      auto pa_sc_vport_zmax = getRegister<latte::PA_SC_VPORT_ZMAX_N>(latte::Register::PA_SC_VPORT_ZMAX_0);

      auto width = pa_cl_vport_xscale.VPORT_XSCALE * 2.0f;
      auto height = pa_cl_vport_yscale.VPORT_YSCALE * 2.0f;

      auto x = pa_cl_vport_xoffset.VPORT_XOFFSET - pa_cl_vport_xscale.VPORT_XSCALE;
      auto y = pa_cl_vport_yoffset.VPORT_YOFFSET - pa_cl_vport_yscale.VPORT_YSCALE;

      gl::glViewport(gsl::narrow_cast<gl::GLint>(x),
                     gsl::narrow_cast<gl::GLint>(y),
                     gsl::narrow_cast<gl::GLint>(width),
                     gsl::narrow_cast<gl::GLint>(height));

      float nearZ, farZ;

      if (pa_cl_vport_zscale.VPORT_ZSCALE > 0.0f) {
         nearZ = pa_sc_vport_zmin.VPORT_ZMIN;
         farZ = pa_sc_vport_zmax.VPORT_ZMAX;
      } else {
         farZ = pa_sc_vport_zmin.VPORT_ZMIN;
         nearZ = pa_sc_vport_zmax.VPORT_ZMAX;
      }

      gl::glDepthRangef(nearZ, farZ);
      mViewportDirty = false;
   }

   if (mScissorDirty) {
      auto pa_sc_generic_scissor_tl = getRegister<latte::PA_SC_GENERIC_SCISSOR_TL>(latte::Register::PA_SC_GENERIC_SCISSOR_TL);
      auto pa_sc_generic_scissor_br = getRegister<latte::PA_SC_GENERIC_SCISSOR_BR>(latte::Register::PA_SC_GENERIC_SCISSOR_BR);

      auto x = pa_sc_generic_scissor_tl.TL_X;
      auto y = pa_sc_generic_scissor_tl.TL_Y;
      auto width = pa_sc_generic_scissor_br.BR_X - pa_sc_generic_scissor_tl.TL_X;
      auto height = pa_sc_generic_scissor_br.BR_Y - pa_sc_generic_scissor_tl.TL_Y;

      gl::glEnable(gl::GL_SCISSOR_TEST);
      gl::glScissor(x, y, width, height);
      mScissorDirty = false;
   }

   return true;
}

static gl::GLenum
getRefFunc(latte::REF_FUNC func)
{
   switch (func) {
   case latte::REF_NEVER:
      return gl::GL_NEVER;
   case latte::REF_LESS:
      return gl::GL_LESS;
   case latte::REF_EQUAL:
      return gl::GL_EQUAL;
   case latte::REF_LEQUAL:
      return gl::GL_LEQUAL;
   case latte::REF_GREATER:
      return gl::GL_GREATER;
   case latte::REF_NOTEQUAL:
      return gl::GL_NOTEQUAL;
   case latte::REF_GEQUAL:
      return gl::GL_GEQUAL;
   case latte::REF_ALWAYS:
      return gl::GL_ALWAYS;
   default:
      throw unimplemented_error(fmt::format("Unimplemented REF_FUNC {}", func));
   }
}

static gl::GLenum
getBlendFunc(latte::CB_BLEND_FUNC func)
{
   switch (func) {
   case latte::CB_BLEND_ZERO:
      return gl::GL_ZERO;
   case latte::CB_BLEND_ONE:
      return gl::GL_ONE;
   case latte::CB_BLEND_SRC_COLOR:
      return gl::GL_SRC_COLOR;
   case latte::CB_BLEND_ONE_MINUS_SRC_COLOR:
      return gl::GL_ONE_MINUS_SRC_COLOR;
   case latte::CB_BLEND_SRC_ALPHA:
      return gl::GL_SRC_ALPHA;
   case latte::CB_BLEND_ONE_MINUS_SRC_ALPHA:
      return gl::GL_ONE_MINUS_SRC_ALPHA;
   case latte::CB_BLEND_DST_ALPHA:
      return gl::GL_DST_ALPHA;
   case latte::CB_BLEND_ONE_MINUS_DST_ALPHA:
      return gl::GL_ONE_MINUS_DST_ALPHA;
   case latte::CB_BLEND_DST_COLOR:
      return gl::GL_DST_COLOR;
   case latte::CB_BLEND_ONE_MINUS_DST_COLOR:
      return gl::GL_ONE_MINUS_DST_COLOR;
   case latte::CB_BLEND_SRC_ALPHA_SATURATE:
      return gl::GL_SRC_ALPHA_SATURATE;
   case latte::CB_BLEND_CONSTANT_COLOR:
      return gl::GL_CONSTANT_COLOR;
   case latte::CB_BLEND_ONE_MINUS_CONSTANT_COLOR:
      return gl::GL_ONE_MINUS_CONSTANT_COLOR;
   case latte::CB_BLEND_SRC1_COLOR:
      return gl::GL_SRC1_COLOR;
   case latte::CB_BLEND_ONE_MINUS_SRC1_COLOR:
      return gl::GL_ONE_MINUS_SRC1_COLOR;
   case latte::CB_BLEND_SRC1_ALPHA:
      return gl::GL_SRC1_ALPHA;
   case latte::CB_BLEND_ONE_MINUS_SRC1_ALPHA:
      return gl::GL_ONE_MINUS_SRC1_ALPHA;
   case latte::CB_BLEND_CONSTANT_ALPHA:
      return gl::GL_CONSTANT_ALPHA;
   case latte::CB_BLEND_ONE_MINUS_CONSTANT_ALPHA:
      return gl::GL_ONE_MINUS_CONSTANT_ALPHA;
   default:
      throw unimplemented_error(fmt::format("Unimplemented CB_BLEND_FUNC {}", func));
   }
}

static gl::GLenum
getBlendEquation(latte::CB_COMB_FUNC func)
{
   switch (func) {
   case latte::CB_COMB_DST_PLUS_SRC:
      return gl::GL_FUNC_ADD;
   case latte::CB_COMB_SRC_MINUS_DST:
      return gl::GL_FUNC_SUBTRACT;
   case latte::CB_COMB_MIN_DST_SRC:
      return gl::GL_MIN;
   case latte::CB_COMB_MAX_DST_SRC:
      return gl::GL_MAX;
   case latte::CB_COMB_DST_MINUS_SRC:
      return gl::GL_FUNC_REVERSE_SUBTRACT;
   default:
      throw unimplemented_error(fmt::format("Unimplemented CB_COMB_FUNC {}", func));
   }
}

static gl::GLenum
getStencilFunc(latte::DB_STENCIL_FUNC func)
{
   switch (func) {
   case latte::DB_STENCIL_KEEP:
      return gl::GL_KEEP;
   case latte::DB_STENCIL_ZERO:
      return gl::GL_ZERO;
   case latte::DB_STENCIL_REPLACE:
      return gl::GL_REPLACE;
   case latte::DB_STENCIL_INCR_CLAMP:
      return gl::GL_INCR;
   case latte::DB_STENCIL_DECR_CLAMP:
      return gl::GL_DECR;
   case latte::DB_STENCIL_INVERT:
      return gl::GL_INVERT;
   case latte::DB_STENCIL_INCR_WRAP:
      return gl::GL_INCR_WRAP;
   case latte::DB_STENCIL_DECR_WRAP:
      return gl::GL_DECR_WRAP;
   default:
      throw unimplemented_error(fmt::format("Unimplemented DB_STENCIL_FUNC {}", func));
   }
}

void GLDriver::setRegister(latte::Register::Value reg, uint32_t value)
{
   // Save to my state
   mRegisters[reg / 4] = value;

   // Save to shadowed context state
   if (mContextState) {
      mContextState->setRegister(reg, value);
   }

   // For the following registers, we apply their state changes
   //   directly to the OpenGL context...
   switch (reg) {
   case latte::Register::SQ_VTX_SEMANTIC_CLEAR:
      for (auto i = 0u; i < 32; ++i) {
         setRegister(static_cast<latte::Register::Value>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4), 0xffffffff);
      }
      break;

   case latte::Register::CB_BLEND0_CONTROL:
   case latte::Register::CB_BLEND1_CONTROL:
   case latte::Register::CB_BLEND2_CONTROL:
   case latte::Register::CB_BLEND3_CONTROL:
   case latte::Register::CB_BLEND4_CONTROL:
   case latte::Register::CB_BLEND5_CONTROL:
   case latte::Register::CB_BLEND6_CONTROL:
   case latte::Register::CB_BLEND7_CONTROL:
   {
      auto target = (reg - latte::Register::CB_BLEND0_CONTROL) / 4;
      auto cb_blend_control = latte::CB_BLENDN_CONTROL { value };
      auto dstRGB = getBlendFunc(cb_blend_control.COLOR_DESTBLEND);
      auto srcRGB = getBlendFunc(cb_blend_control.COLOR_SRCBLEND);
      auto modeRGB = getBlendEquation(cb_blend_control.COLOR_COMB_FCN);

      if (!cb_blend_control.SEPARATE_ALPHA_BLEND) {
         gl::glBlendFunci(target, srcRGB, dstRGB);
         gl::glBlendEquationi(target, modeRGB);
      } else {
         auto dstAlpha = getBlendFunc(cb_blend_control.ALPHA_DESTBLEND);
         auto srcAlpha = getBlendFunc(cb_blend_control.ALPHA_SRCBLEND);
         auto modeAlpha = getBlendEquation(cb_blend_control.ALPHA_COMB_FCN);
         gl::glBlendFuncSeparatei(target, srcRGB, dstRGB, srcAlpha, dstAlpha);
         gl::glBlendEquationSeparatei(target, modeRGB, modeAlpha);
      }
   } break;

   case latte::Register::CB_COLOR_CONTROL:
   {
      auto cb_color_control = latte::CB_COLOR_CONTROL { value };

      for (auto i = 0u; i < 8; ++i) {
         if (cb_color_control.TARGET_BLEND_ENABLE & (1 << i)) {
            gl::glEnablei(gl::GL_BLEND, i);
         } else {
            gl::glDisablei(gl::GL_BLEND, i);
         }
      }
   } break;

   case latte::Register::CB_TARGET_MASK:
   {
      auto cb_target_mask = latte::CB_TARGET_MASK { value };
      auto mask = cb_target_mask.value;

      for (auto i = 0; i < 8; ++i, mask >>= 4) {
         auto red    = mask & (1 << 0);
         auto green  = mask & (1 << 1);
         auto blue   = mask & (1 << 2);
         auto alpha  = mask & (1 << 3);

         gl::glColorMaski(i,
                          red ? gl::GL_TRUE : gl::GL_FALSE,
                          green ? gl::GL_TRUE : gl::GL_FALSE,
                          blue ? gl::GL_TRUE : gl::GL_FALSE,
                          alpha ? gl::GL_TRUE : gl::GL_FALSE);
      }
   } break;

   case latte::Register::DB_STENCILREFMASK_BF:
   {
      auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
      auto db_stencilrefmask_bf = latte::DB_STENCILREFMASK_BF { value };
      auto backStencilFunc = getRefFunc(db_depth_control.STENCILFUNC_BF);

      if (db_depth_control.BACKFACE_ENABLE) {
         gl::glStencilFuncSeparate(gl::GL_BACK, backStencilFunc, db_stencilrefmask_bf.STENCILREF_BF, db_stencilrefmask_bf.STENCILMASK_BF);
      }
   } break;

   case latte::Register::DB_STENCILREFMASK:
   {
      auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
      auto db_stencilrefmask = latte::DB_STENCILREFMASK { value };
      auto frontStencilFunc = getRefFunc(db_depth_control.STENCILFUNC);

      if (!db_depth_control.BACKFACE_ENABLE) {
         gl::glStencilFuncSeparate(gl::GL_FRONT_AND_BACK, frontStencilFunc, db_stencilrefmask.STENCILREF, db_stencilrefmask.STENCILMASK);
      } else {
         gl::glStencilFuncSeparate(gl::GL_FRONT, frontStencilFunc, db_stencilrefmask.STENCILREF, db_stencilrefmask.STENCILMASK);
      }
   } break;

   case latte::Register::DB_DEPTH_CONTROL:
   {
      auto db_depth_control = latte::DB_DEPTH_CONTROL { value };
      auto db_stencilrefmask = getRegister<latte::DB_STENCILREFMASK>(latte::Register::DB_STENCILREFMASK);
      auto db_stencilrefmask_bf = getRegister<latte::DB_STENCILREFMASK_BF>(latte::Register::DB_STENCILREFMASK_BF);

      if (db_depth_control.Z_ENABLE) {
         gl::glEnable(gl::GL_DEPTH_TEST);
      } else {
         gl::glDisable(gl::GL_DEPTH_TEST);
      }

      if (db_depth_control.Z_WRITE_ENABLE) {
         gl::glDepthMask(gl::GL_TRUE);
      } else {
         gl::glDepthMask(gl::GL_FALSE);
      }

      auto zfunc = getRefFunc(db_depth_control.ZFUNC);
      gl::glDepthFunc(zfunc);

      if (db_depth_control.STENCIL_ENABLE) {
         gl::glEnable(gl::GL_STENCIL_TEST);
      } else {
         gl::glDisable(gl::GL_STENCIL_TEST);
      }

      auto frontStencilFunc = getRefFunc(db_depth_control.STENCILFUNC);
      auto frontStencilZPass = getStencilFunc(db_depth_control.STENCILZPASS);
      auto frontStencilZFail = getStencilFunc(db_depth_control.STENCILZFAIL);
      auto frontStencilFail = getStencilFunc(db_depth_control.STENCILFAIL);

      if (!db_depth_control.BACKFACE_ENABLE) {
         gl::glStencilFuncSeparate(gl::GL_FRONT_AND_BACK, frontStencilFunc, db_stencilrefmask.STENCILREF, db_stencilrefmask.STENCILMASK);
         gl::glStencilOpSeparate(gl::GL_FRONT_AND_BACK, frontStencilFail, frontStencilZFail, frontStencilZPass);
      } else {
         auto backStencilFunc = getRefFunc(db_depth_control.STENCILFUNC_BF);
         auto backStencilZPass = getStencilFunc(db_depth_control.STENCILZPASS_BF);
         auto backStencilZFail = getStencilFunc(db_depth_control.STENCILZFAIL_BF);
         auto backStencilFail = getStencilFunc(db_depth_control.STENCILFAIL_BF);

         gl::glStencilFuncSeparate(gl::GL_FRONT, frontStencilFunc, db_stencilrefmask.STENCILREF, db_stencilrefmask.STENCILMASK);
         gl::glStencilOpSeparate(gl::GL_FRONT, frontStencilFail, frontStencilZFail, frontStencilZPass);

         gl::glStencilFuncSeparate(gl::GL_BACK, backStencilFunc, db_stencilrefmask_bf.STENCILREF_BF, db_stencilrefmask_bf.STENCILMASK_BF);
         gl::glStencilOpSeparate(gl::GL_BACK, backStencilFail, backStencilZFail, backStencilZPass);
      }
   } break;

   case latte::Register::CB_BLEND_RED:
   case latte::Register::CB_BLEND_GREEN:
   case latte::Register::CB_BLEND_BLUE:
   case latte::Register::CB_BLEND_ALPHA:
   {
      auto cb_blend_red = getRegister<latte::CB_BLEND_RED>(latte::Register::CB_BLEND_RED);
      auto cb_blend_green = getRegister<latte::CB_BLEND_GREEN>(latte::Register::CB_BLEND_GREEN);
      auto cb_blend_blue = getRegister<latte::CB_BLEND_BLUE>(latte::Register::CB_BLEND_BLUE);
      auto cb_blend_alpha = getRegister<latte::CB_BLEND_ALPHA>(latte::Register::CB_BLEND_ALPHA);

      gl::glBlendColor(cb_blend_red.BLEND_RED / 255.0f,
                       cb_blend_green.BLEND_GREEN / 255.0f,
                       cb_blend_blue.BLEND_BLUE / 255.0f,
                       cb_blend_alpha.BLEND_ALPHA / 255.0f);
   } break;

   case latte::Register::PA_CL_VPORT_XSCALE_0:
   case latte::Register::PA_CL_VPORT_XOFFSET_0:
   case latte::Register::PA_CL_VPORT_YSCALE_0:
   case latte::Register::PA_CL_VPORT_YOFFSET_0:
   case latte::Register::PA_CL_VPORT_ZSCALE_0:
   case latte::Register::PA_CL_VPORT_ZOFFSET_0:
   case latte::Register::PA_SC_VPORT_ZMIN_0:
   case latte::Register::PA_SC_VPORT_ZMAX_0:
      mViewportDirty = true;
      break;

   case latte::Register::PA_SC_GENERIC_SCISSOR_TL:
   case latte::Register::PA_SC_GENERIC_SCISSOR_BR:
      mScissorDirty = true;
      break;

   case latte::Register::SX_ALPHA_REF:
   case latte::Register::SX_ALPHA_TEST_CONTROL:
   {
      auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
      auto sx_alpha_ref = getRegister<latte::SX_ALPHA_REF>(latte::Register::SX_ALPHA_REF);

      if (sx_alpha_test_control.ALPHA_TEST_ENABLE) {
         gl::glEnable(gl::GL_ALPHA_TEST);
      } else {
         gl::glDisable(gl::GL_ALPHA_TEST);
      }

      gl::glAlphaFunc(getRefFunc(sx_alpha_test_control.ALPHA_FUNC), sx_alpha_ref.ALPHA_REF);
   } break;

   case latte::Register::PA_SU_SC_MODE_CNTL:
   {
      auto pa_su_sc_mode_cntl = latte::PA_SU_SC_MODE_CNTL { value };

      if (pa_su_sc_mode_cntl.FACE == latte::FACE_CW) {
         gl::glFrontFace(gl::GL_CW);
      } else {
         gl::glFrontFace(gl::GL_CCW);
      }

      if (pa_su_sc_mode_cntl.CULL_FRONT && pa_su_sc_mode_cntl.CULL_BACK) {
         gl::glEnable(gl::GL_CULL_FACE);
         gl::glCullFace(gl::GL_FRONT_AND_BACK);
      } else if (pa_su_sc_mode_cntl.CULL_FRONT) {
         gl::glEnable(gl::GL_CULL_FACE);
         gl::glCullFace(gl::GL_FRONT);
      } else if (pa_su_sc_mode_cntl.CULL_BACK) {
         gl::glEnable(gl::GL_CULL_FACE);
         gl::glCullFace(gl::GL_BACK);
      } else {
         gl::glDisable(gl::GL_CULL_FACE);
      }
   } break;
   }
}

static std::string
readFileToString(const std::string &filename)
{
   std::ifstream in { filename, std::ifstream::binary };
   std::string result;

   if (in.is_open()) {
      in.seekg(0, in.end);
      auto size = in.tellg();
      result.resize(size);
      in.seekg(0, in.beg);
      in.read(&result[0], size);
   }

   return result;
}

void GLDriver::initGL()
{
   platform::ui::activateContext();
   glbinding::Binding::initialize();

   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
      auto error = gl::glGetError();

      if (error != gl::GL_NO_ERROR) {
         gLog->error("OpenGL {} error: {}", call.toString(), error);
      }
   });

   // Set initial viewport
   gl::glViewport(0, 0, platform::ui::getWindowWidth(), platform::ui::getWindowHeight());

   // Set a background color for emu window.
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
   // Sneakily assume double-buffering...
   platform::ui::swapBuffers();
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Clear active state
   mRegisters.fill(0);
   mActiveShader = nullptr;
   mActiveDepthBuffer = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(ColorBuffer *) * mActiveColorBuffers.size());

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer.object);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);

   auto vertexCode = readFileToString("resources/shaders/screen_vertex.glsl");
   if (!vertexCode.size()) {
      gLog->error("Could not load resources/shaders/screen_vertex.glsl");
   }

   auto pixelCode = readFileToString("resources/shaders/screen_pixel.glsl");
   if (!pixelCode.size()) {
      gLog->error("Could not load resources/shaders/screen_pixel.glsl");
   }

   // Create vertex program
   auto code = vertexCode.c_str();
   mScreenDraw.vertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &code);

   // Create pixel program
   code = pixelCode.c_str();
   mScreenDraw.pixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &code);
   gl::glBindFragDataLocation(mScreenDraw.pixelProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &mScreenDraw.pipeline);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_VERTEX_SHADER_BIT, mScreenDraw.vertexProgram);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_FRAGMENT_SHADER_BIT, mScreenDraw.pixelProgram);

   float wndWidth = static_cast<float>(platform::ui::getWindowWidth());
   float wndHeight = static_cast<float>(platform::ui::getWindowHeight());
   float tvWidth = static_cast<float>(platform::ui::getTvWidth());
   float tvHeight = static_cast<float>(platform::ui::getTvHeight());
   float drcWidth = static_cast<float>(platform::ui::getDrcWidth());
   float drcHeight = static_cast<float>(platform::ui::getDrcHeight());

   auto tvTop = 0;
   auto tvLeft = (wndWidth - tvWidth) / 2;
   auto tvBottom = tvTop + tvHeight;
   auto tvRight = tvLeft + tvWidth;
   auto drcTop = tvBottom;
   auto drcLeft = (wndWidth - drcWidth) / 2;
   auto drcBottom = drcTop + drcHeight;
   auto drcRight = drcLeft + drcWidth;

#define SX(x) (((x)/wndWidth*2)-1)
#define SY(y) -(((y)/wndHeight*2)-1)

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      // TV
      SX(tvLeft),  SY(tvTop),       0.0f, 1.0f,
      SX(tvRight), SY(tvTop),       1.0f, 1.0f,
      SX(tvRight), SY(tvBottom),    1.0f, 0.0f,

      SX(tvRight), SY(tvBottom),    1.0f, 0.0f,
      SX(tvLeft),  SY(tvBottom),    0.0f, 0.0f,
      SX(tvLeft),  SY(tvTop),       0.0f, 1.0f,

      // DRC
      SX(drcLeft),  SY(drcTop),     0.0f, 1.0f,
      SX(drcRight), SY(drcTop),     1.0f, 1.0f,
      SX(drcRight), SY(drcBottom),  1.0f, 0.0f,

      SX(drcRight), SY(drcBottom),  1.0f, 0.0f,
      SX(drcLeft),  SY(drcBottom),  0.0f, 0.0f,
      SX(drcLeft),  SY(drcTop),     0.0f, 1.0f
   };
#undef SX
#undef SY

   gl::glCreateBuffers(1, &mScreenDraw.vertBuffer);
   gl::glNamedBufferData(mScreenDraw.vertBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &mScreenDraw.vertArray);

   auto fs_position = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_position);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_texCoord, 0);
}

enum
{
   SCANTARGET_TV = 1,
   SCANTARGET_DRC = 4,
};

void GLDriver::decafCopyColorToScan(const pm4::DecafCopyColorToScan &data)
{
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Unbind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);

   // Setup viewport
   gl::glViewport(0, 0, platform::ui::getWindowWidth(), platform::ui::getWindowHeight());

   // Setup screen draw shader
   gl::glBindVertexArray(mScreenDraw.vertArray);
   gl::glBindVertexBuffer(0, mScreenDraw.vertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mScreenDraw.pipeline);

   // Set active shader to nullptr so it has to rebind.
   mActiveShader = nullptr;

   // Draw screen quad
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glBindSampler(0, 0);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);
   gl::glDisable(gl::GL_ALPHA_TEST);
   gl::glBindTextureUnit(0, buffer->object);

   if (data.scanTarget == SCANTARGET_TV) {
      gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
   } else if (data.scanTarget == SCANTARGET_DRC) {
      gl::glDrawArrays(gl::GL_TRIANGLES, 6, 6);
   } else {
      gLog->error("decafCopyColorToScan called for unknown scanTarget.");
   }

   // Rebind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);
}

void GLDriver::decafSwapBuffers(const pm4::DecafSwapBuffers &data)
{
   platform::ui::swapBuffers();
}

void GLDriver::decafClearColor(const pm4::DecafClearColor &data)
{
   float colors[] = {
      data.red,
      data.green,
      data.blue,
      data.alpha
   };

   // Check if the color buffer is actively bound
   for (auto i = 0; i < 8; ++i) {
      auto active = mActiveColorBuffers[i];

      if (!active) {
         continue;
      }

      if (active->cb_color_base.BASE_256B == data.bufferAddr) {
         gl::glClearBufferfv(gl::GL_COLOR, i, colors);
         return;
      }
   }

   // Find our colorbuffer to clear
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Bind color buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, buffer->object, 0);

   // Clear color buffer
   gl::glClearBufferfv(gl::GL_COLOR, 0, colors);

   // Restore original color buffer
   if (mActiveColorBuffers[0]) {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, mActiveColorBuffers[0]->object, 0);
   } else {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, 0, 0);
   }
}

void GLDriver::decafClearDepthStencil(const pm4::DecafClearDepthStencil &data)
{
   auto db_depth_clear = getRegister<latte::DB_DEPTH_CLEAR>(latte::Register::DB_DEPTH_CLEAR);
   auto db_stencil_clear = getRegister<latte::DB_STENCIL_CLEAR>(latte::Register::DB_STENCIL_CLEAR);

   // Check if this is the active depth buffer
   if (mActiveDepthBuffer && mActiveDepthBuffer->db_depth_base.BASE_256B == data.bufferAddr) {
      // Clear active
      gl::glClearBufferfi(gl::GL_DEPTH_STENCIL, 0, db_depth_clear.DEPTH_CLEAR, db_stencil_clear.CLEAR);
      return;
   }

   // Find our depthbuffer to clear
   auto db_depth_base = bit_cast<latte::DB_DEPTH_BASE>(data.bufferAddr);
   auto buffer = getDepthBuffer(db_depth_base, data.db_depth_size, data.db_depth_info);

   // Bind depth buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, buffer->object, 0);

   // Clear depth buffer
   gl::glClearBufferfi(gl::GL_DEPTH_STENCIL, 0, db_depth_clear.DEPTH_CLEAR, db_stencil_clear.CLEAR);

   // Restore original depth buffer
   if (mActiveDepthBuffer) {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, mActiveDepthBuffer->object, 0);
   } else {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, 0, 0);
   }
}

void GLDriver::decafSetContextState(const pm4::DecafSetContextState &data)
{
   mContextState = reinterpret_cast<latte::ContextState *>(data.context.get());
}

static gl::GLenum
getPrimitiveMode(latte::VGT_DI_PRIMITIVE_TYPE type)
{
   switch (type) {
   case latte::VGT_DI_PT_POINTLIST:
      return gl::GL_POINTS;
   case latte::VGT_DI_PT_LINELIST:
      return gl::GL_LINES;
   case latte::VGT_DI_PT_LINESTRIP:
      return gl::GL_LINE_STRIP;
   case latte::VGT_DI_PT_TRILIST:
      return gl::GL_TRIANGLES;
   case latte::VGT_DI_PT_TRIFAN:
      return gl::GL_TRIANGLE_FAN;
   case latte::VGT_DI_PT_TRISTRIP:
      return gl::GL_TRIANGLE_STRIP;
   case latte::VGT_DI_PT_LINELOOP:
      return gl::GL_LINE_LOOP;
   default:
      throw unimplemented_error(fmt::format("Unimplemented VGT_PRIMITIVE_TYPE {}", type));
   }
}

template<typename IndexType>
static void
drawPrimitives2(gl::GLenum mode, uint32_t count, const IndexType *indices, uint32_t offset)
{
   if (!indices) {
      gl::glDrawArrays(mode, offset, count);
   } else if (std::is_same<IndexType, uint16_t>()) {
      gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_SHORT, indices, offset);
   } else if (std::is_same<IndexType, uint32_t>()) {
      gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_INT, indices, offset);
   }
}

template<typename IndexType>
static void
unpackQuadList(uint32_t count, const IndexType *src, uint32_t offset)
{
   auto tris = (count / 4) * 6;
   auto unpacked = std::vector<IndexType>(tris);
   auto dst = &unpacked[0];

   // Unpack quad indices into triangle indices
   if (src) {
      for (auto i = 0u; i < count / 4; ++i) {
         auto index_tl = *src++;
         auto index_tr = *src++;
         auto index_br = *src++;
         auto index_bl = *src++;

         *(dst++) = index_tl;
         *(dst++) = index_tr;
         *(dst++) = index_bl;

         *(dst++) = index_bl;
         *(dst++) = index_tr;
         *(dst++) = index_br;
      }
   } else {
      auto index_tl = 1u;
      auto index_tr = 0u;
      auto index_br = 2u;
      auto index_bl = 3u;

      for (auto i = 0u; i < count / 4; ++i) {
         *(dst++) = index_tl;
         *(dst++) = index_tr;
         *(dst++) = index_bl;

         *(dst++) = index_bl;
         *(dst++) = index_tr;
         *(dst++) = index_br;
      }
   }

   drawPrimitives2(gl::GL_TRIANGLES, tris, unpacked.data(), offset);
}

static void
drawPrimitives(latte::VGT_DI_PRIMITIVE_TYPE primType,
               uint32_t offset,
               uint32_t count,
               const void *indices,
               latte::VGT_INDEX indexFmt)
{
   if (primType == latte::VGT_DI_PT_QUADLIST) {
      if (indexFmt == latte::VGT_INDEX_16) {
         unpackQuadList(count, reinterpret_cast<const uint16_t*>(indices), offset);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         unpackQuadList(count, reinterpret_cast<const uint32_t*>(indices), offset);
      }
   } else {
      auto mode = getPrimitiveMode(primType);

      if (indexFmt == latte::VGT_INDEX_16) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint16_t*>(indices), offset);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint32_t*>(indices), offset);
      }
   }
}

void GLDriver::drawIndexAuto(const pm4::DrawIndexAuto &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);

   drawPrimitives(vgt_primitive_type.PRIM_TYPE,
                  sq_vtx_base_vtx_loc.OFFSET,
                  data.indexCount,
                  nullptr,
                  latte::VGT_INDEX_32);
}

void GLDriver::drawIndex2(const pm4::DrawIndex2 &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);

   // Swap and indexBytes are separate because you can have 32-bit swap,
   //   but 16-bit indices in some cases...  This is also why we pre-swap
   //   the data before intercepting QUAD and POLYGON draws.
   if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_16_BIT) {
      auto *src = static_cast<uint16_t*>(data.addr.get());
      auto indices = std::vector<uint16_t>(data.numIndices);

      if (vgt_dma_index_type.INDEX_TYPE != latte::VGT_INDEX_16) {
         throw std::logic_error(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_16_BIT", vgt_dma_index_type.INDEX_TYPE));
      }

      for (auto i = 0u; i < data.numIndices; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(vgt_primitive_type.PRIM_TYPE,
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.numIndices,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE);
   } else if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_32_BIT) {
      auto *src = static_cast<uint32_t*>(data.addr.get());
      auto indices = std::vector<uint32_t>(data.numIndices);

      if (vgt_dma_index_type.INDEX_TYPE != latte::VGT_INDEX_32) {
         throw std::logic_error(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_32_BIT", vgt_dma_index_type.INDEX_TYPE));
      }

      for (auto i = 0u; i < data.numIndices; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(vgt_primitive_type.PRIM_TYPE,
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.numIndices,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE);
   } else if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_NONE) {
      drawPrimitives(vgt_primitive_type.PRIM_TYPE,
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.numIndices,
                     data.addr,
                     vgt_dma_index_type.INDEX_TYPE);
   } else {
      throw unimplemented_error(fmt::format("Unimplemented vgt_dma_index_type.SWAP_MODE {}", vgt_dma_index_type.SWAP_MODE));
   }
}

void GLDriver::indexType(const pm4::IndexType &data)
{
   mRegisters[latte::Register::VGT_DMA_INDEX_TYPE / 4] = data.type.value;
}

void GLDriver::numInstances(const pm4::NumInstances &data)
{
   mRegisters[latte::Register::VGT_DMA_NUM_INSTANCES / 4] = data.count;
}

void GLDriver::setAluConsts(const pm4::SetAluConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setConfigRegs(const pm4::SetConfigRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setContextRegs(const pm4::SetContextRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setControlConstants(const pm4::SetControlConstants &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setLoopConsts(const pm4::SetLoopConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setSamplers(const pm4::SetSamplers &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setResources(const pm4::SetResources &data)
{
   auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(id + i * 4), data.values[i]);
   }
}

void GLDriver::loadRegisters(latte::Register::Value base, uint32_t *src, const gsl::span<std::pair<uint32_t, uint32_t>> &registers)
{
   for (auto &range : registers) {
      auto start = range.first;
      auto count = range.second;

      for (auto j = start; j < start + count; ++j) {
         setRegister(static_cast<latte::Register::Value>(base + j * 4), src[j]);
      }
   }
}

void GLDriver::loadAluConsts(const pm4::LoadAluConst &data)
{
   loadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadBoolConsts(const pm4::LoadBoolConst &data)
{
   loadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadConfigRegs(const pm4::LoadConfigReg &data)
{
   loadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
}

void GLDriver::loadContextRegs(const pm4::LoadContextReg &data)
{
   loadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
}

void GLDriver::loadControlConstants(const pm4::LoadControlConst &data)
{
   loadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
}

void GLDriver::loadLoopConsts(const pm4::LoadLoopConst &data)
{
   loadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadSamplers(const pm4::LoadSampler &data)
{
   loadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
}

void GLDriver::loadResources(const pm4::LoadResource &data)
{
   loadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
}

void GLDriver::indirectBufferCall(const pm4::IndirectBufferCall &data)
{
   auto buffer = reinterpret_cast<uint32_t*>(data.addr.get());
   runCommandBuffer(buffer, data.size);
}

void GLDriver::handlePacketType3(pm4::Packet3 header, const gsl::span<uint32_t> &data)
{
   pm4::PacketReader reader { data };

   switch (header.opcode) {
   case pm4::Opcode3::DECAF_COPY_COLOR_TO_SCAN:
      decafCopyColorToScan(pm4::read<pm4::DecafCopyColorToScan>(reader));
      break;
   case pm4::Opcode3::DECAF_SWAP_BUFFERS:
      decafSwapBuffers(pm4::read<pm4::DecafSwapBuffers>(reader));
      break;
   case pm4::Opcode3::DECAF_CLEAR_COLOR:
      decafClearColor(pm4::read<pm4::DecafClearColor>(reader));
      break;
   case pm4::Opcode3::DECAF_CLEAR_DEPTH_STENCIL:
      decafClearDepthStencil(pm4::read<pm4::DecafClearDepthStencil>(reader));
      break;
   case pm4::Opcode3::DECAF_SET_CONTEXT_STATE:
      decafSetContextState(pm4::read<pm4::DecafSetContextState>(reader));
      break;
   case pm4::Opcode3::DRAW_INDEX_AUTO:
      drawIndexAuto(pm4::read<pm4::DrawIndexAuto>(reader));
      break;
   case pm4::Opcode3::DRAW_INDEX_2:
      drawIndex2(pm4::read<pm4::DrawIndex2>(reader));
      break;
   case pm4::Opcode3::INDEX_TYPE:
      indexType(pm4::read<pm4::IndexType>(reader));
      break;
   case pm4::Opcode3::NUM_INSTANCES:
      numInstances(pm4::read<pm4::NumInstances>(reader));
      break;
   case pm4::Opcode3::SET_ALU_CONST:
      setAluConsts(pm4::read<pm4::SetAluConsts>(reader));
      break;
   case pm4::Opcode3::SET_CONFIG_REG:
      setConfigRegs(pm4::read<pm4::SetConfigRegs>(reader));
      break;
   case pm4::Opcode3::SET_CONTEXT_REG:
      setContextRegs(pm4::read<pm4::SetContextRegs>(reader));
      break;
   case pm4::Opcode3::SET_CTL_CONST:
      setControlConstants(pm4::read<pm4::SetControlConstants>(reader));
      break;
   case pm4::Opcode3::SET_LOOP_CONST:
      setLoopConsts(pm4::read<pm4::SetLoopConsts>(reader));
      break;
   case pm4::Opcode3::SET_SAMPLER:
      setSamplers(pm4::read<pm4::SetSamplers>(reader));
      break;
   case pm4::Opcode3::SET_RESOURCE:
      setResources(pm4::read<pm4::SetResources>(reader));
      break;
   case pm4::Opcode3::LOAD_CONFIG_REG:
      loadConfigRegs(pm4::read<pm4::LoadConfigReg>(reader));
      break;
   case pm4::Opcode3::LOAD_CONTEXT_REG:
      loadContextRegs(pm4::read<pm4::LoadContextReg>(reader));
      break;
   case pm4::Opcode3::LOAD_ALU_CONST:
      loadAluConsts(pm4::read<pm4::LoadAluConst>(reader));
      break;
   case pm4::Opcode3::LOAD_BOOL_CONST:
      loadBoolConsts(pm4::read<pm4::LoadBoolConst>(reader));
      break;
   case pm4::Opcode3::LOAD_LOOP_CONST:
      loadLoopConsts(pm4::read<pm4::LoadLoopConst>(reader));
      break;
   case pm4::Opcode3::LOAD_RESOURCE:
      loadResources(pm4::read<pm4::LoadResource>(reader));
      break;
   case pm4::Opcode3::LOAD_SAMPLER:
      loadSamplers(pm4::read<pm4::LoadSampler>(reader));
      break;
   case pm4::Opcode3::LOAD_CTL_CONST:
      loadControlConstants(pm4::read<pm4::LoadControlConst>(reader));
      break;
   case pm4::Opcode3::INDIRECT_BUFFER_PRIV:
      indirectBufferCall(pm4::read<pm4::IndirectBufferCall>(reader));
      break;
   }
}

void GLDriver::start()
{
   mRunning = true;
   mThread = std::thread(&GLDriver::run, this);
}

void GLDriver::setTvDisplay(size_t width, size_t height)
{
}

void GLDriver::setDrcDisplay(size_t width, size_t height)
{
}

void GLDriver::runCommandBuffer(uint32_t *buffer, uint32_t buffer_size)
{
   for (auto pos = 0u; pos < buffer_size; ) {
      auto header = *reinterpret_cast<pm4::PacketHeader *>(&buffer[pos]);
      auto size = 0u;

      switch (header.type) {
      case pm4::PacketType::Type3:
      {
         auto header3 = pm4::Packet3{ header.value };
         size = header3.size + 1;
         handlePacketType3(header3, gsl::as_span(&buffer[pos + 1], size));
         break;
      }
      case pm4::PacketType::Type0:
      case pm4::PacketType::Type1:
      case pm4::PacketType::Type2:
      default:
         throw std::logic_error("What the fuck son");
      }

      pos += size + 1;
   }
}

void GLDriver::run()
{
   initGL();

   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();
      runCommandBuffer(buffer->buffer, buffer->curSize);
      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
