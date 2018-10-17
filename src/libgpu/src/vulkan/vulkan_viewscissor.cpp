#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

bool
Driver::checkCurrentViewportAndScissor()
{
   // GPU7 actually supports many viewports and many scissors, but it
   // seems that CafeOS itself only supports a single one.

   // ------------------------------------------------------------
   // Viewport
   // ------------------------------------------------------------

   auto pa_cl_vport_xscale = getRegister<latte::PA_CL_VPORT_XSCALE_N>(latte::Register::PA_CL_VPORT_XSCALE_0);
   auto pa_cl_vport_yscale = getRegister<latte::PA_CL_VPORT_YSCALE_N>(latte::Register::PA_CL_VPORT_YSCALE_0);
   auto pa_cl_vport_zscale = getRegister<latte::PA_CL_VPORT_ZSCALE_N>(latte::Register::PA_CL_VPORT_ZSCALE_0);
   auto pa_cl_vport_xoffset = getRegister<latte::PA_CL_VPORT_XOFFSET_N>(latte::Register::PA_CL_VPORT_XOFFSET_0);
   auto pa_cl_vport_yoffset = getRegister<latte::PA_CL_VPORT_YOFFSET_N>(latte::Register::PA_CL_VPORT_YOFFSET_0);
   auto pa_cl_vport_zoffset = getRegister<latte::PA_CL_VPORT_ZOFFSET_N>(latte::Register::PA_CL_VPORT_ZOFFSET_0);
   auto pa_sc_vport_zmin = getRegister<latte::PA_SC_VPORT_ZMIN_N>(latte::Register::PA_SC_VPORT_ZMIN_0);
   auto pa_sc_vport_zmax = getRegister<latte::PA_SC_VPORT_ZMAX_N>(latte::Register::PA_SC_VPORT_ZMAX_0);

   vk::Viewport viewport;
   viewport.width = pa_cl_vport_xscale.VPORT_XSCALE() * 2.0f;
   viewport.height = pa_cl_vport_yscale.VPORT_YSCALE() * 2.0f;
   viewport.x = pa_cl_vport_xoffset.VPORT_XOFFSET() - pa_cl_vport_xscale.VPORT_XSCALE();
   viewport.y = pa_cl_vport_yoffset.VPORT_YOFFSET() - pa_cl_vport_yscale.VPORT_YSCALE();

   // TODO: Investigate whether we should be using ZOFFSET/ZSCALE to calculate these?
   if (pa_cl_vport_zscale.VPORT_ZSCALE() > 0.0f) {
      viewport.minDepth = pa_sc_vport_zmin.VPORT_ZMIN();
      viewport.maxDepth = pa_sc_vport_zmax.VPORT_ZMAX();
   } else {
      viewport.maxDepth = pa_sc_vport_zmin.VPORT_ZMIN();
      viewport.minDepth = pa_sc_vport_zmax.VPORT_ZMAX();
   }

   mCurrentViewport = viewport;


   // ------------------------------------------------------------
   // Scissoring
   // ------------------------------------------------------------

   auto pa_sc_generic_scissor_tl = getRegister<latte::PA_SC_GENERIC_SCISSOR_TL>(latte::Register::PA_SC_GENERIC_SCISSOR_TL);
   auto pa_sc_generic_scissor_br = getRegister<latte::PA_SC_GENERIC_SCISSOR_BR>(latte::Register::PA_SC_GENERIC_SCISSOR_BR);

   vk::Rect2D scissor;
   scissor.offset.x = pa_sc_generic_scissor_tl.TL_X();
   scissor.offset.y = pa_sc_generic_scissor_tl.TL_Y();
   scissor.extent.width = pa_sc_generic_scissor_br.BR_X() - scissor.offset.x;
   scissor.extent.height = pa_sc_generic_scissor_br.BR_Y() - scissor.offset.y;

   mCurrentScissor = scissor;
   return true;
}

void
Driver::bindViewportAndScissor()
{
   mActiveCommandBuffer.setViewport(0, { mCurrentViewport });
   mActiveCommandBuffer.setScissor(0, { mCurrentScissor });
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
