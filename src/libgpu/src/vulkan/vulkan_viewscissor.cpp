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
   auto pa_cl_vte_cntl = getRegister<latte::PA_CL_VTE_CNTL>(latte::Register::PA_CL_VTE_CNTL);
   auto pa_cl_clip_cntl = getRegister<latte::PA_CL_CLIP_CNTL>(latte::Register::PA_CL_CLIP_CNTL);

   auto raWidth = static_cast<float>(mCurrentDraw->framebuffer->renderArea.width);
   auto raHeight = static_cast<float>(mCurrentDraw->framebuffer->renderArea.height);

   // NOTE: Our shaders which output positions understand that if the
   // xoffset/yoffset/xscale/yscale are disabled, that we need to
   // pre-transform the render-area down to -1to1 on X and Y.  This
   // means rather than using a disabled viewport here, we need to
   // expand this back out.  The reason to do this is that there is
   // NDC-space clipping occuring in Vulkan which would prevent this
   // from working as it does on GPU7.

   float vportOX, vportOY, vportSX, vportSY;
   if (pa_cl_vte_cntl.VPORT_X_OFFSET_ENA()) {
      vportOX = pa_cl_vport_xoffset.VPORT_XOFFSET();
   } else {
      vportOX = raWidth / 2;
   }
   if (pa_cl_vte_cntl.VPORT_Y_OFFSET_ENA()) {
      vportOY  = pa_cl_vport_yoffset.VPORT_YOFFSET();
   } else {
      vportOY = raHeight / 2;
   }
   if (pa_cl_vte_cntl.VPORT_X_SCALE_ENA()) {
      vportSX = pa_cl_vport_xscale.VPORT_XSCALE();
   } else {
      vportSX = raWidth / 2;
   }
   if (pa_cl_vte_cntl.VPORT_Y_SCALE_ENA()) {
      vportSY = pa_cl_vport_yscale.VPORT_YSCALE();
   } else {
      vportSY = raHeight / 2;
   }

   vk::Viewport viewport;

   viewport.x = vportOX - vportSX;
   viewport.y = vportOY - vportSY;
   viewport.width = vportSX * 2;
   viewport.height = vportSY * 2;

   // TODO: Investigate whether we should be using ZOFFSET/ZSCALE to calculate these?
   viewport.minDepth = pa_sc_vport_zmin.VPORT_ZMIN();
   viewport.maxDepth = pa_sc_vport_zmax.VPORT_ZMAX();

   mCurrentDraw->viewport = viewport;


   // Set up some stuff used by the shaders
   ShaderViewportData shaderViewport;

   // These are not handled as I don't believe we actually scale/offset by the viewport...
   //pa_cl_vte_cntl.VPORT_Z_OFFSET_ENA();
   //pa_cl_vte_cntl.VPORT_Z_SCALE_ENA();

   // TODO: Implement these
   //pa_cl_vte_cntl.VTX_XY_FMT();
   //pa_cl_vte_cntl.VTX_Z_FMT();
   //pa_cl_vte_cntl.VTX_W0_FMT();

   auto screenSizeX = viewport.width - viewport.x;
   auto screenSizeY = viewport.height - viewport.y;

   if (pa_cl_vte_cntl.VPORT_X_OFFSET_ENA()) {
      shaderViewport.xAdd = 0.0f;
   } else {
      shaderViewport.xAdd = -1.0f;
   }
   if (pa_cl_vte_cntl.VPORT_X_SCALE_ENA()) {
      shaderViewport.xMul = 1.0f;
   } else {
      shaderViewport.xMul = 2.0f / screenSizeX;
   }
   if (pa_cl_vte_cntl.VPORT_Y_OFFSET_ENA()) {
      shaderViewport.yAdd = 0.0f;
   } else {
      shaderViewport.yAdd = -1.0f;
   }
   if (pa_cl_vte_cntl.VPORT_Y_SCALE_ENA()) {
      shaderViewport.yMul = 1.0f;
   } else {
      shaderViewport.yMul = 2.0f / screenSizeY;
   }

   if (!pa_cl_clip_cntl.DX_CLIP_SPACE_DEF()) {
      // map gl(-1 to 1) onto vk(0 to 1)
      shaderViewport.zAdd = 1.0f; // Add W
      shaderViewport.zMul = 0.5f; // * 0.5
   } else {
      // maintain 0 to 1
      shaderViewport.zAdd = 0.0f; // Add 0
      shaderViewport.zMul = 1.0f; // * 1.0
   }

   mCurrentDraw->shaderViewportData = shaderViewport;



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

   mCurrentDraw->scissor = scissor;
   return true;
}

void
Driver::bindViewportAndScissor()
{
   mActiveCommandBuffer.setViewport(0, { mCurrentDraw->viewport });
   mActiveCommandBuffer.setScissor(0, { mCurrentDraw->scissor });
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
