#ifdef DECAF_GL
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace opengl
{

bool
GLDriver::checkViewport()
{
   if (mActiveShader->vertex->isScreenSpace) {
      auto pa_cl_vport_xscale = getRegister<latte::PA_CL_VPORT_XSCALE_N>(latte::Register::PA_CL_VPORT_XSCALE_0);
      auto pa_cl_vport_xoffset = getRegister<latte::PA_CL_VPORT_XOFFSET_N>(latte::Register::PA_CL_VPORT_XOFFSET_0);
      auto pa_cl_vport_yscale = getRegister<latte::PA_CL_VPORT_YSCALE_N>(latte::Register::PA_CL_VPORT_YSCALE_0);
      auto pa_cl_vport_yoffset = getRegister<latte::PA_CL_VPORT_YOFFSET_N>(latte::Register::PA_CL_VPORT_YOFFSET_0);

      gl::glProgramUniform4f(
         mActiveShader->vertex->object,
         mActiveShader->vertex->uniformViewport,
         pa_cl_vport_xoffset.VPORT_XOFFSET(),
         pa_cl_vport_yoffset.VPORT_YOFFSET(),
         1.0f / pa_cl_vport_xscale.VPORT_XSCALE(),
         1.0f / pa_cl_vport_yscale.VPORT_YSCALE());
   }

   if (mViewportDirty) {
      auto pa_cl_vport_xscale = getRegister<latte::PA_CL_VPORT_XSCALE_N>(latte::Register::PA_CL_VPORT_XSCALE_0);
      auto pa_cl_vport_xoffset = getRegister<latte::PA_CL_VPORT_XOFFSET_N>(latte::Register::PA_CL_VPORT_XOFFSET_0);
      auto pa_cl_vport_yscale = getRegister<latte::PA_CL_VPORT_YSCALE_N>(latte::Register::PA_CL_VPORT_YSCALE_0);
      auto pa_cl_vport_yoffset = getRegister<latte::PA_CL_VPORT_YOFFSET_N>(latte::Register::PA_CL_VPORT_YOFFSET_0);

      auto width = pa_cl_vport_xscale.VPORT_XSCALE() * 2.0f;
      auto height = pa_cl_vport_yscale.VPORT_YSCALE() * 2.0f;

      auto x = pa_cl_vport_xoffset.VPORT_XOFFSET() - pa_cl_vport_xscale.VPORT_XSCALE();
      auto y = pa_cl_vport_yoffset.VPORT_YOFFSET() - pa_cl_vport_yscale.VPORT_YSCALE();

      gl::glViewport(gsl::narrow_cast<gl::GLint>(x),
                     gsl::narrow_cast<gl::GLint>(y),
                     gsl::narrow_cast<gl::GLint>(width),
                     gsl::narrow_cast<gl::GLint>(height));
      mViewportDirty = false;
   }

   if (mDepthRangeDirty) {
      auto pa_cl_vport_zscale = getRegister<latte::PA_CL_VPORT_ZSCALE_N>(latte::Register::PA_CL_VPORT_ZSCALE_0);
      auto pa_cl_vport_zoffset = getRegister<latte::PA_CL_VPORT_ZOFFSET_N>(latte::Register::PA_CL_VPORT_ZOFFSET_0);
      auto pa_sc_vport_zmin = getRegister<latte::PA_SC_VPORT_ZMIN_N>(latte::Register::PA_SC_VPORT_ZMIN_0);
      auto pa_sc_vport_zmax = getRegister<latte::PA_SC_VPORT_ZMAX_N>(latte::Register::PA_SC_VPORT_ZMAX_0);

      float nearZ, farZ;

      if (pa_cl_vport_zscale.VPORT_ZSCALE() > 0.0f) {
         nearZ = pa_sc_vport_zmin.VPORT_ZMIN();
         farZ = pa_sc_vport_zmax.VPORT_ZMAX();
      } else {
         farZ = pa_sc_vport_zmin.VPORT_ZMIN();
         nearZ = pa_sc_vport_zmax.VPORT_ZMAX();
      }

      gl::glDepthRangef(nearZ, farZ);
      mDepthRangeDirty = false;
   }

   if (mScissorDirty) {
      auto pa_sc_generic_scissor_tl = getRegister<latte::PA_SC_GENERIC_SCISSOR_TL>(latte::Register::PA_SC_GENERIC_SCISSOR_TL);
      auto pa_sc_generic_scissor_br = getRegister<latte::PA_SC_GENERIC_SCISSOR_BR>(latte::Register::PA_SC_GENERIC_SCISSOR_BR);

      auto x = pa_sc_generic_scissor_tl.TL_X();
      auto y = pa_sc_generic_scissor_tl.TL_Y();
      auto width = pa_sc_generic_scissor_br.BR_X() - x;
      auto height = pa_sc_generic_scissor_br.BR_Y() - y;

      gl::glScissor(x, y, width, height);
      mScissorDirty = false;
   }

   return true;
}

} // namespace opengl

#endif // ifdef DECAF_GL
