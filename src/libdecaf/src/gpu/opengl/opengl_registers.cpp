#ifndef DECAF_NOGL

#include <common/decaf_assert.h>
#include <common/gl.h>
#include "opengl_driver.h"

namespace gpu
{

namespace opengl
{

static GLenum
getRefFunc(latte::REF_FUNC func);

static GLenum
getBlendFunc(latte::CB_BLEND_FUNC func);

static GLenum
getBlendEquation(latte::CB_COMB_FUNC func);

static GLenum
getStencilFunc(latte::DB_STENCIL_FUNC func);

void
GLDriver::applyRegister(latte::Register reg)
{
   auto value = getRegister<uint32_t>(reg);

   // Handle optimization with uniform update generation tracking
   if (reg >= latte::Register::AluConstRegisterBase &&
      reg < latte::Register::AluConstRegisterEnd)
   {
      auto offset = (reg - latte::Register::AluConstRegisterBase) / 4 / 4;
      mLastUniformUpdate[offset / 16] = mUniformUpdateGen;
      return;
   }

   // Handle setting OpenGL state for anything else
   switch (reg) {
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
      auto cb_blend_control = latte::CB_BLENDN_CONTROL::get(value);
      auto dstRGB = getBlendFunc(cb_blend_control.COLOR_DESTBLEND());
      auto srcRGB = getBlendFunc(cb_blend_control.COLOR_SRCBLEND());
      auto modeRGB = getBlendEquation(cb_blend_control.COLOR_COMB_FCN());

      if (!cb_blend_control.SEPARATE_ALPHA_BLEND()) {
         glBlendFunci(target, srcRGB, dstRGB);
         glBlendEquationi(target, modeRGB);
      } else {
         auto dstAlpha = getBlendFunc(cb_blend_control.ALPHA_DESTBLEND());
         auto srcAlpha = getBlendFunc(cb_blend_control.ALPHA_SRCBLEND());
         auto modeAlpha = getBlendEquation(cb_blend_control.ALPHA_COMB_FCN());
         glBlendFuncSeparatei(target, srcRGB, dstRGB, srcAlpha, dstAlpha);
         glBlendEquationSeparatei(target, modeRGB, modeAlpha);
      }
   } break;

   case latte::Register::CB_COLOR_CONTROL:
   {
      auto cb_color_control = latte::CB_COLOR_CONTROL::get(value);

      for (auto i = 0u; i < 8; ++i) {
         auto enable = !!(cb_color_control.TARGET_BLEND_ENABLE() & (1 << i));
         if (enable != mGLStateCache.blendEnable[i]) {
            mGLStateCache.blendEnable[i] = enable;
            if (enable) {
               glEnablei(GL_BLEND, i);
            } else {
               glDisablei(GL_BLEND, i);
            }
         }
      }
   } break;

   case latte::Register::DB_STENCILREFMASK_BF:
   {
      auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);

      if (db_depth_control.BACKFACE_ENABLE()) {
         auto db_stencilrefmask_bf = latte::DB_STENCILREFMASK_BF::get(value);
         auto backStencilRef = db_stencilrefmask_bf.STENCILREF_BF();
         auto backStencilMask = db_stencilrefmask_bf.STENCILMASK_BF();

         if (mGLStateCache.backStencilRef != backStencilRef
          || mGLStateCache.backStencilMask != backStencilMask) {
            mGLStateCache.backStencilRef = backStencilRef;
            mGLStateCache.backStencilMask = backStencilMask;

            auto backStencilFunc = getRefFunc(db_depth_control.STENCILFUNC_BF());

            glStencilFuncSeparate(GL_BACK, backStencilFunc, backStencilRef, backStencilMask);
         }
      }
   } break;

   case latte::Register::DB_STENCILREFMASK:
   {
      auto db_stencilrefmask = latte::DB_STENCILREFMASK::get(value);
      auto frontStencilRef = db_stencilrefmask.STENCILREF();
      auto frontStencilMask = db_stencilrefmask.STENCILMASK();

      if (mGLStateCache.frontStencilRef != frontStencilRef
       || mGLStateCache.frontStencilMask != frontStencilMask) {
         mGLStateCache.frontStencilRef = frontStencilRef;
         mGLStateCache.frontStencilMask = frontStencilMask;

         auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
         auto frontStencilFunc = getRefFunc(db_depth_control.STENCILFUNC());

         if (!db_depth_control.BACKFACE_ENABLE()) {
            glStencilFuncSeparate(GL_FRONT_AND_BACK, frontStencilFunc, frontStencilRef, frontStencilMask);
         } else {
            glStencilFuncSeparate(GL_FRONT, frontStencilFunc, frontStencilRef, frontStencilMask);
         }
      }
   } break;

   case latte::Register::DB_DEPTH_CONTROL:
   {
      auto db_depth_control = latte::DB_DEPTH_CONTROL::get(value);

      if (mGLStateCache.depthEnable != db_depth_control.Z_ENABLE()) {
         mGLStateCache.depthEnable = db_depth_control.Z_ENABLE();
         if (db_depth_control.Z_ENABLE()) {
            glEnable(GL_DEPTH_TEST);
         } else {
            glDisable(GL_DEPTH_TEST);
         }
      }

      if (mGLStateCache.depthWrite != db_depth_control.Z_WRITE_ENABLE()) {
         mGLStateCache.depthWrite = db_depth_control.Z_WRITE_ENABLE();
         if (db_depth_control.Z_WRITE_ENABLE()) {
            glDepthMask(GL_TRUE);
         } else {
            glDepthMask(GL_FALSE);
         }
      }

      auto zfunc = getRefFunc(db_depth_control.ZFUNC());
      if (mGLStateCache.depthFunc != zfunc) {
         mGLStateCache.depthFunc = zfunc;
         glDepthFunc(zfunc);
      }

      if (mGLStateCache.stencilEnable != db_depth_control.STENCIL_ENABLE()) {
         mGLStateCache.stencilEnable = db_depth_control.STENCIL_ENABLE();
         if (db_depth_control.STENCIL_ENABLE()) {
            glEnable(GL_STENCIL_TEST);
         } else {
            glDisable(GL_STENCIL_TEST);
         }
      }

      // Rather than doing 9 separate loads, conversions, and comparisons,
      //  we just mask off the stencil backface and operation bits and
      //  compare them as a unit to save time when the state is unchanged.
      auto stencilState = db_depth_control.value & 0xFFFFFF80u;
      if (mGLStateCache.stencilState != stencilState) {
         mGLStateCache.stencilState = stencilState;

         auto frontStencilFunc = getRefFunc(db_depth_control.STENCILFUNC());
         auto frontStencilZPass = getStencilFunc(db_depth_control.STENCILZPASS());
         auto frontStencilZFail = getStencilFunc(db_depth_control.STENCILZFAIL());
         auto frontStencilFail = getStencilFunc(db_depth_control.STENCILFAIL());
         auto db_stencilrefmask = getRegister<latte::DB_STENCILREFMASK>(latte::Register::DB_STENCILREFMASK);

         if (db_depth_control.BACKFACE_ENABLE()) {
            auto backStencilFunc = getRefFunc(db_depth_control.STENCILFUNC_BF());
            auto backStencilZPass = getStencilFunc(db_depth_control.STENCILZPASS_BF());
            auto backStencilZFail = getStencilFunc(db_depth_control.STENCILZFAIL_BF());
            auto backStencilFail = getStencilFunc(db_depth_control.STENCILFAIL_BF());
            auto db_stencilrefmask_bf = getRegister<latte::DB_STENCILREFMASK_BF>(latte::Register::DB_STENCILREFMASK_BF);

            glStencilFuncSeparate(GL_FRONT, frontStencilFunc, db_stencilrefmask_bf.STENCILREF_BF(), db_stencilrefmask_bf.STENCILMASK_BF());
            glStencilOpSeparate(GL_FRONT, frontStencilFail, frontStencilZFail, frontStencilZPass);

            glStencilFuncSeparate(GL_BACK, backStencilFunc, db_stencilrefmask_bf.STENCILREF_BF(), db_stencilrefmask_bf.STENCILMASK_BF());
            glStencilOpSeparate(GL_BACK, backStencilFail, backStencilZFail, backStencilZPass);
         } else {
            glStencilFuncSeparate(GL_FRONT_AND_BACK, frontStencilFunc, db_stencilrefmask.STENCILREF(), db_stencilrefmask.STENCILMASK());
            glStencilOpSeparate(GL_FRONT_AND_BACK, frontStencilFail, frontStencilZFail, frontStencilZPass);
         }
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

      glBlendColor(cb_blend_red.BLEND_RED(),
                       cb_blend_green.BLEND_GREEN(),
                       cb_blend_blue.BLEND_BLUE(),
                       cb_blend_alpha.BLEND_ALPHA());
   } break;

   case latte::Register::PA_CL_VPORT_XSCALE_0:
   case latte::Register::PA_CL_VPORT_XOFFSET_0:
   case latte::Register::PA_CL_VPORT_YSCALE_0:
   case latte::Register::PA_CL_VPORT_YOFFSET_0:
      mViewportDirty = true;
      break;

   case latte::Register::PA_CL_VPORT_ZSCALE_0:
   case latte::Register::PA_CL_VPORT_ZOFFSET_0:
   case latte::Register::PA_SC_VPORT_ZMIN_0:
   case latte::Register::PA_SC_VPORT_ZMAX_0:
      mDepthRangeDirty = true;
      break;

   case latte::Register::PA_SC_GENERIC_SCISSOR_TL:
   case latte::Register::PA_SC_GENERIC_SCISSOR_BR:
      mScissorDirty = true;
      break;

   case latte::Register::PA_SU_SC_MODE_CNTL:
   {
      auto pa_su_sc_mode_cntl = latte::PA_SU_SC_MODE_CNTL::get(value);
      auto cullFace = GL_NONE;

      if (pa_su_sc_mode_cntl.CULL_FRONT() && pa_su_sc_mode_cntl.CULL_BACK()) {
         cullFace = GL_FRONT_AND_BACK;
      } else if (pa_su_sc_mode_cntl.CULL_FRONT()) {
         cullFace = GL_FRONT;
      } else if (pa_su_sc_mode_cntl.CULL_BACK()) {
         cullFace = GL_BACK;
      }

      auto cullFaceEnable = (cullFace != GL_NONE);

      if (mGLStateCache.cullFaceEnable != cullFaceEnable) {
         mGLStateCache.cullFaceEnable = cullFaceEnable;

         if (cullFaceEnable) {
            glEnable(GL_CULL_FACE);
         } else {
            glDisable(GL_CULL_FACE);
         }
      }

      if (cullFaceEnable && mGLStateCache.cullFace != cullFace) {
         mGLStateCache.cullFace = cullFace;
         glCullFace(cullFace);
      }

      auto frontFace = (pa_su_sc_mode_cntl.FACE() == latte::PA_FACE::CW ? GL_CW : GL_CCW);

      if (mGLStateCache.frontFace != frontFace) {
         mGLStateCache.frontFace = frontFace;
         glFrontFace(frontFace);
      }
   } break;

   case latte::Register::PA_CL_CLIP_CNTL:
   {
      auto pa_cl_clip_cntl = latte::PA_CL_CLIP_CNTL::get(value);

      if (mGLStateCache.rasterizerDiscard != pa_cl_clip_cntl.RASTERISER_DISABLE()) {
         mGLStateCache.rasterizerDiscard = pa_cl_clip_cntl.RASTERISER_DISABLE();
         if (pa_cl_clip_cntl.RASTERISER_DISABLE()) {
            glEnable(GL_RASTERIZER_DISCARD);
         } else {
            glDisable(GL_RASTERIZER_DISCARD);
         }
      }

      decaf_assert(pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE() == pa_cl_clip_cntl.ZCLIP_FAR_DISABLE(),
                   fmt::format("Inconsistent near/far depth clamp setting"));

      if (mGLStateCache.depthClamp != !pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE()) {
         mGLStateCache.depthClamp = !pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE();

         if (pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE()) {
            glEnable(GL_DEPTH_CLAMP);
         } else {
            glDisable(GL_DEPTH_CLAMP);
         }
      }

      if (mGLStateCache.halfZClipSpace != pa_cl_clip_cntl.DX_CLIP_SPACE_DEF()) {
         mGLStateCache.halfZClipSpace = pa_cl_clip_cntl.DX_CLIP_SPACE_DEF();

         if (pa_cl_clip_cntl.DX_CLIP_SPACE_DEF()) {
            glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
         } else {
            glClipControl(GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
         }
      }
   } break;

   case latte::Register::VGT_MULTI_PRIM_IB_RESET_EN:
   {
      auto vgt_multi_prim_ib_reset_en = *reinterpret_cast<latte::VGT_MULTI_PRIM_IB_RESET_EN*>(&value);

      if (mGLStateCache.primRestartEnable != vgt_multi_prim_ib_reset_en.RESET_EN()) {
         mGLStateCache.primRestartEnable = vgt_multi_prim_ib_reset_en.RESET_EN();

         if (vgt_multi_prim_ib_reset_en.RESET_EN()) {
            glEnable(GL_PRIMITIVE_RESTART);
         } else {
            glDisable(GL_PRIMITIVE_RESTART);
         }
      }
   } break;

   case latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX:
   {
      auto vgt_multi_prim_ib_reset_indx = *reinterpret_cast<latte::VGT_MULTI_PRIM_IB_RESET_INDX*>(&value);

      if (mGLStateCache.primRestartIndex != vgt_multi_prim_ib_reset_indx.RESET_INDX()) {
         mGLStateCache.primRestartIndex = vgt_multi_prim_ib_reset_indx.RESET_INDX();
         glPrimitiveRestartIndex(vgt_multi_prim_ib_reset_indx.RESET_INDX());
      }
   } break;

   }
}

GLenum
getRefFunc(latte::REF_FUNC func)
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
      decaf_abort(fmt::format("Unimplemented REF_FUNC {}", func));
   }
}

GLenum
getBlendFunc(latte::CB_BLEND_FUNC func)
{
   switch (func) {
   case latte::CB_BLEND_FUNC::ZERO:
      return GL_ZERO;
   case latte::CB_BLEND_FUNC::ONE:
      return GL_ONE;
   case latte::CB_BLEND_FUNC::SRC_COLOR:
      return GL_SRC_COLOR;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_COLOR:
      return GL_ONE_MINUS_SRC_COLOR;
   case latte::CB_BLEND_FUNC::SRC_ALPHA:
      return GL_SRC_ALPHA;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_ALPHA:
      return GL_ONE_MINUS_SRC_ALPHA;
   case latte::CB_BLEND_FUNC::DST_ALPHA:
      return GL_DST_ALPHA;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_ALPHA:
      return GL_ONE_MINUS_DST_ALPHA;
   case latte::CB_BLEND_FUNC::DST_COLOR:
      return GL_DST_COLOR;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_COLOR:
      return GL_ONE_MINUS_DST_COLOR;
   case latte::CB_BLEND_FUNC::SRC_ALPHA_SATURATE:
      return GL_SRC_ALPHA_SATURATE;
   case latte::CB_BLEND_FUNC::CONSTANT_COLOR:
      return GL_CONSTANT_COLOR;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_COLOR:
      return GL_ONE_MINUS_CONSTANT_COLOR;
   case latte::CB_BLEND_FUNC::SRC1_COLOR:
      return GL_SRC1_COLOR;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_COLOR:
      return GL_ONE_MINUS_SRC1_COLOR;
   case latte::CB_BLEND_FUNC::SRC1_ALPHA:
      return GL_SRC1_ALPHA;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_ALPHA:
      return GL_ONE_MINUS_SRC1_ALPHA;
   case latte::CB_BLEND_FUNC::CONSTANT_ALPHA:
      return GL_CONSTANT_ALPHA;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_ALPHA:
      return GL_ONE_MINUS_CONSTANT_ALPHA;
   default:
      decaf_abort(fmt::format("Unimplemented CB_BLEND_FUNC {}", func));
   }
}

GLenum
getBlendEquation(latte::CB_COMB_FUNC func)
{
   switch (func) {
   case latte::CB_COMB_FUNC::DST_PLUS_SRC:
      return GL_FUNC_ADD;
   case latte::CB_COMB_FUNC::SRC_MINUS_DST:
      return GL_FUNC_SUBTRACT;
   case latte::CB_COMB_FUNC::MIN_DST_SRC:
      return GL_MIN;
   case latte::CB_COMB_FUNC::MAX_DST_SRC:
      return GL_MAX;
   case latte::CB_COMB_FUNC::DST_MINUS_SRC:
      return GL_FUNC_REVERSE_SUBTRACT;
   default:
      decaf_abort(fmt::format("Unimplemented CB_COMB_FUNC {}", func));
   }
}

GLenum
getStencilFunc(latte::DB_STENCIL_FUNC func)
{
   switch (func) {
   case latte::DB_STENCIL_FUNC::KEEP:
      return GL_KEEP;
   case latte::DB_STENCIL_FUNC::ZERO:
      return GL_ZERO;
   case latte::DB_STENCIL_FUNC::REPLACE:
      return GL_REPLACE;
   case latte::DB_STENCIL_FUNC::INCR_CLAMP:
      return GL_INCR;
   case latte::DB_STENCIL_FUNC::DECR_CLAMP:
      return GL_DECR;
   case latte::DB_STENCIL_FUNC::INVERT:
      return GL_INVERT;
   case latte::DB_STENCIL_FUNC::INCR_WRAP:
      return GL_INCR_WRAP;
   case latte::DB_STENCIL_FUNC::DECR_WRAP:
      return GL_DECR_WRAP;
   default:
      decaf_abort(fmt::format("Unimplemented DB_STENCIL_FUNC {}", func));
   }
}

} // namespace opengl

} // namespace gpu

#endif // DECAF_NOGL
