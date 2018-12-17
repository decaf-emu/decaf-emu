#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

static constexpr bool ForceDescriptorSets = false;

namespace vulkan
{

PipelineDesc
Driver::getPipelineDesc()
{
   PipelineDesc desc;

   desc.renderPass = mCurrentDraw->renderPass;
   desc.vertexShader = mCurrentDraw->vertexShader;
   desc.geometryShader = mCurrentDraw->geometryShader;
   desc.pixelShader = mCurrentDraw->pixelShader;
   desc.rectStubShader = mCurrentDraw->rectStubShader;

   // -- Vertex Strides
   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      // Skip unused input buffers
      if (!desc.vertexShader->shader.meta.attribBuffers[i].isUsed) {
         desc.attribBufferStride[i] = 0;
         continue;
      }

      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + i) * 7;
      auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_RESOURCE_WORD2_0 + 4 * resourceOffset);
      desc.attribBufferStride[i] = sq_vtx_constant_word2.STRIDE();
   }

   // -- Vertex Buffer Divisors
   desc.attribBufferDivisor[0] = 1;
   desc.attribBufferDivisor[1] = 1;

   uint32_t instanceStepRate0 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_0);
   uint32_t instanceStepRate1 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_1);
   for (auto &attribBuffer : desc.vertexShader->shader.meta.attribBuffers) {
      if (attribBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::REGISTER_0) {
         desc.attribBufferDivisor[0] = instanceStepRate0;
      } else if (attribBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::REGISTER_1) {
         desc.attribBufferDivisor[1] = instanceStepRate1;
      }
   }

   // -- Primitive Type
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   desc.primitiveType = vgt_primitive_type.PRIM_TYPE();

   // -- Primitive Reset Stuff
   auto vgt_multi_prim_ib_reset_en = getRegister<latte::VGT_MULTI_PRIM_IB_RESET_EN>(latte::Register::VGT_MULTI_PRIM_IB_RESET_EN);
   auto vgt_multi_prim_ib_reset_idx = getRegister<latte::VGT_MULTI_PRIM_IB_RESET_INDX>(latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX);
   desc.primitiveResetEnabled = vgt_multi_prim_ib_reset_en.RESET_EN();
   desc.primitiveResetIndex = vgt_multi_prim_ib_reset_idx.RESET_INDX();

   // -- Constants mode
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   desc.dx9Consts = sq_config.DX9_CONSTS();

   // -- Rasterization stuff
   auto pa_cl_clip_cntl = getRegister<latte::PA_CL_CLIP_CNTL>(latte::Register::PA_CL_CLIP_CNTL);
   auto pa_su_line_cntl = getRegister<latte::PA_SU_LINE_CNTL>(latte::Register::PA_SU_LINE_CNTL);
   auto pa_su_sc_mode_cntl = getRegister<latte::PA_SU_SC_MODE_CNTL>(latte::Register::PA_SU_SC_MODE_CNTL);
   auto pa_su_poly_offset_front_offset = getRegister<latte::PA_SU_POLY_OFFSET_FRONT_OFFSET>(latte::Register::PA_SU_POLY_OFFSET_FRONT_OFFSET);
   auto pa_su_poly_offset_front_scale = getRegister<latte::PA_SU_POLY_OFFSET_FRONT_SCALE>(latte::Register::PA_SU_POLY_OFFSET_FRONT_SCALE);
   auto pa_su_poly_offset_back_offset = getRegister<latte::PA_SU_POLY_OFFSET_FRONT_OFFSET>(latte::Register::PA_SU_POLY_OFFSET_BACK_OFFSET);
   auto pa_su_poly_offset_back_scale = getRegister<latte::PA_SU_POLY_OFFSET_FRONT_SCALE>(latte::Register::PA_SU_POLY_OFFSET_BACK_SCALE);
   auto pa_su_poly_offset_clamp = getRegister<latte::PA_SU_POLY_OFFSET_CLAMP>(latte::Register::PA_SU_POLY_OFFSET_CLAMP);

   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_0());
   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_1());
   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_2());
   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_3());
   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_4());
   decaf_check_warn(!pa_cl_clip_cntl.UCP_ENA_5());
   decaf_check_warn(!pa_cl_clip_cntl.PS_UCP_Y_SCALE_NEG());
   decaf_check_warn(pa_cl_clip_cntl.PS_UCP_MODE() == latte::PA_PS_UCP_MODE::CULL_DISTANCE);
   decaf_check_warn(!pa_cl_clip_cntl.UCP_CULL_ONLY_ENA());
   decaf_check_warn(!pa_cl_clip_cntl.BOUNDARY_EDGE_FLAG_ENA());
   decaf_check_warn(!pa_cl_clip_cntl.DIS_CLIP_ERR_DETECT());
   decaf_check_warn(!pa_cl_clip_cntl.VTX_KILL_OR());
   decaf_check_warn(!pa_cl_clip_cntl.DX_LINEAR_ATTR_CLIP_ENA());
   decaf_check_warn(!pa_cl_clip_cntl.VTE_VPORT_PROVOKE_DISABLE());

   // pa_cl_clip_cntl.CLIP_DISABLE() is really an optimization which
   // indicates that there will be no draws outside the boundary of
   // the framebuffer.  We don't need to handle this.

   // pa_cl_clip_cntl.DX_CLIP_SPACE_DEF() is handled by the shaders,
   // so it is uploaded in the push constants buffer in the shader
   // resources binding

   desc.rasteriserDisable = pa_cl_clip_cntl.RASTERISER_DISABLE();

   // Line widths
   desc.lineWidth = pa_su_line_cntl.WIDTH();

   desc.cullFront = pa_su_sc_mode_cntl.CULL_FRONT();
   desc.cullBack = pa_su_sc_mode_cntl.CULL_BACK();
   desc.paFace = pa_su_sc_mode_cntl.FACE();

   // We do not support split front/back mode, but we make a best-effort
   // to avoid needing to configure the two differently.  In most cases
   // that there is divergence between front/back configuration, its due
   // to one of them being culled away anyways.  I'm not 100% confident
   // that this is the correct behaviour though, it would be better if
   // we supported splitting the polygons as needed.

   // TODO: Use decaf_check_warn here instead...

   desc.polyPType = latte::PA_PTYPE::TRIANGLES;

   auto polyMode = pa_su_sc_mode_cntl.POLY_MODE();
   if (polyMode == 0) {
      // POLY_MODE is disabled
   } else if (polyMode == 1) {
      // POLY_MODE is dual-triangle
      if (desc.cullBack) {
         desc.polyPType = pa_su_sc_mode_cntl.POLYMODE_FRONT_PTYPE();
      } else if (desc.cullFront) {
         desc.polyPType = pa_su_sc_mode_cntl.POLYMODE_BACK_PTYPE();
      } else {
         decaf_check_warn(pa_su_sc_mode_cntl.POLYMODE_FRONT_PTYPE() == pa_su_sc_mode_cntl.POLYMODE_BACK_PTYPE());
         desc.polyPType = pa_su_sc_mode_cntl.POLYMODE_FRONT_PTYPE();
      }
   } else {
      gLog->warn("Unexpected POLY_MODE value {}.", polyMode);
   }

   desc.polyBiasEnabled = false;
   if (desc.polyPType == latte::PA_PTYPE::TRIANGLES) {
      if (desc.cullBack) {
         desc.polyBiasEnabled = pa_su_sc_mode_cntl.POLY_OFFSET_FRONT_ENABLE();
      } else if (desc.cullFront) {
         desc.polyBiasEnabled = pa_su_sc_mode_cntl.POLY_OFFSET_BACK_ENABLE();
      } else {
         decaf_check_warn(pa_su_sc_mode_cntl.POLY_OFFSET_FRONT_ENABLE() == pa_su_sc_mode_cntl.POLY_OFFSET_BACK_ENABLE());
         desc.polyBiasEnabled = pa_su_sc_mode_cntl.POLY_OFFSET_FRONT_ENABLE();
      }
   } else {
      desc.polyBiasEnabled = pa_su_sc_mode_cntl.POLY_OFFSET_PARA_ENABLE();
   }

   if (desc.polyBiasEnabled) {
      desc.polyBiasClamp = pa_su_poly_offset_clamp.CLAMP();

      if (desc.cullBack) {
         desc.polyBiasOffset = pa_su_poly_offset_front_offset.OFFSET();
         desc.polyBiasScale = pa_su_poly_offset_front_scale.SCALE();
      } else if (desc.cullFront) {
         desc.polyBiasOffset = pa_su_poly_offset_back_offset.OFFSET();
         desc.polyBiasScale = pa_su_poly_offset_back_scale.SCALE();
      } else {
         decaf_check_warn(pa_su_poly_offset_front_offset.value == pa_su_poly_offset_back_offset.value);
         decaf_check_warn(pa_su_poly_offset_front_scale.value == pa_su_poly_offset_back_scale.value);
         desc.polyBiasOffset = pa_su_poly_offset_front_offset.OFFSET();
         desc.polyBiasScale = pa_su_poly_offset_front_scale.SCALE();
      }
   } else {
      desc.polyBiasClamp = 0.0f;
      desc.polyBiasOffset = 0.0f;
      desc.polyBiasScale = 0.0f;
   }

   // We only support zclip being on or off, not individually for near/far.
   decaf_check_warn(pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE() == pa_cl_clip_cntl.ZCLIP_FAR_DISABLE());
   desc.zclipDisabled = pa_cl_clip_cntl.ZCLIP_NEAR_DISABLE();

   // -- Depth/Stencil control stuff
   auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
   auto db_stencilrefmask = getRegister<latte::DB_STENCILREFMASK>(latte::Register::DB_STENCILREFMASK);
   auto db_stencilrefmask_bf = getRegister<latte::DB_STENCILREFMASK_BF>(latte::Register::DB_STENCILREFMASK_BF);

   desc.stencilEnable = db_depth_control.STENCIL_ENABLE();
   if (desc.stencilEnable) {
      desc.stencilFront.compareFunc = db_depth_control.STENCILFUNC();
      desc.stencilFront.failOp = db_depth_control.STENCILFAIL();
      desc.stencilFront.zPassOp = db_depth_control.STENCILZPASS();
      desc.stencilFront.zFailOp = db_depth_control.STENCILZFAIL();
      desc.stencilFront.ref = db_stencilrefmask.STENCILREF();
      desc.stencilFront.mask = db_stencilrefmask.STENCILMASK();
      desc.stencilFront.writeMask = db_stencilrefmask.STENCILWRITEMASK();

      if (db_depth_control.BACKFACE_ENABLE()) {
         desc.stencilBack.compareFunc = db_depth_control.STENCILFUNC_BF();
         desc.stencilBack.failOp = db_depth_control.STENCILFAIL_BF();
         desc.stencilBack.zPassOp = db_depth_control.STENCILZPASS_BF();
         desc.stencilBack.zFailOp = db_depth_control.STENCILZFAIL_BF();
         desc.stencilBack.ref = db_stencilrefmask_bf.STENCILREF_BF();
         desc.stencilBack.mask = db_stencilrefmask_bf.STENCILMASK_BF();
         desc.stencilBack.writeMask = db_stencilrefmask_bf.STENCILWRITEMASK_BF();
      } else {
         desc.stencilBack = desc.stencilFront;
      }
   } else {
      desc.stencilFront.compareFunc = latte::REF_FUNC::NEVER;
      desc.stencilFront.failOp = latte::DB_STENCIL_FUNC::KEEP;
      desc.stencilFront.zPassOp = latte::DB_STENCIL_FUNC::KEEP;
      desc.stencilFront.zFailOp = latte::DB_STENCIL_FUNC::KEEP;
      desc.stencilFront.ref = 0;
      desc.stencilFront.mask = 0;
      desc.stencilFront.writeMask = 0;
      desc.stencilBack = desc.stencilFront;
   }

   desc.zEnable = db_depth_control.Z_ENABLE();
   desc.zWriteEnable = db_depth_control.Z_WRITE_ENABLE();
   if (desc.zEnable) {
      desc.zFunc = db_depth_control.ZFUNC();
   } else {
      desc.zFunc = latte::REF_FUNC::ALWAYS;
   }


   // -- Color control stuff
   auto makeBlendControl = [&](PipelineDesc::BlendControl& blend, const latte::CB_BLENDN_CONTROL& cb_blend_control)
   {
      if (blend.blendingEnabled) {
         blend.opacityWeight = cb_blend_control.OPACITY_WEIGHT();
         blend.colorCombFcn = cb_blend_control.COLOR_COMB_FCN();
         blend.colorSrcBlend = cb_blend_control.COLOR_SRCBLEND();
         blend.colorDstBlend = cb_blend_control.COLOR_DESTBLEND();
         if (cb_blend_control.SEPARATE_ALPHA_BLEND()) {
            blend.alphaCombFcn = cb_blend_control.ALPHA_COMB_FCN();
            blend.alphaSrcBlend = cb_blend_control.ALPHA_SRCBLEND();
            blend.alphaDstBlend = cb_blend_control.ALPHA_DESTBLEND();
         } else {
            blend.alphaCombFcn = blend.colorCombFcn;
            blend.alphaSrcBlend = blend.colorSrcBlend;
            blend.alphaDstBlend = blend.colorDstBlend;
         }
      } else {
         blend.colorCombFcn = latte::CB_COMB_FUNC::DST_PLUS_SRC;
         blend.colorSrcBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.colorDstBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.alphaCombFcn = latte::CB_COMB_FUNC::DST_PLUS_SRC;
         blend.alphaSrcBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.alphaDstBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.opacityWeight = false;
      }

      return blend;
   };

   auto cb_color_control = getRegister<latte::CB_COLOR_CONTROL>(latte::Register::CB_COLOR_CONTROL);

   // TODO: Implement cb_color_control.FOG_ENABLE()
   // TODO: Implement cb_color_control.MULTIWRITE_ENABLE()
   // TODO: Implement cb_color_control.DITHER_ENABLE()
   // TODO: Implement cb_color_control.DEGAMMA_ENABLE()

   desc.rop3 = cb_color_control.ROP3();

   bool shouldUseBlendEnable = false;
   bool shouldUseBlendControls = false;
   bool shouldUseBlendMasks = false;

   if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_OP::NORMAL) {
      shouldUseBlendEnable = true;
      shouldUseBlendControls = true;
      shouldUseBlendMasks = true;
   } else if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_OP::DISABLE) {
      // We disable all backend state with this op...
   } else if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_OP::FAST_CLEAR) {
      shouldUseBlendMasks = true;
   } else if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_OP::FORCE_CLEAR) {
      shouldUseBlendMasks = true;
   } else {
      decaf_abort("Encountered unexpected CB_SPECIAL_OP");
   }

   if (shouldUseBlendEnable) {
      for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
         desc.cbBlendControls[id].blendingEnabled = cb_color_control.TARGET_BLEND_ENABLE() & (1 << id);
      }
   } else {
      for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
         desc.cbBlendControls[id].blendingEnabled = false;
      }
   }

   if (shouldUseBlendControls) {
      if (!cb_color_control.PER_MRT_BLEND()) {
         // For some reason, even though there is a CB_BLEND_CONTROL register, it is
         // not used here like the docs indicate, and BLEND0 is used instead...
         auto cb_blend_control = getRegister<latte::CB_BLENDN_CONTROL>(latte::Register::CB_BLEND0_CONTROL);

         // We need to iterate still, since blending could be off on specific targets
         for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
            makeBlendControl(desc.cbBlendControls[id], cb_blend_control);
         }
      } else {
         for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
            auto cb_blend_control = getRegister<latte::CB_BLENDN_CONTROL>(latte::Register::CB_BLEND0_CONTROL + 4 * id);
            makeBlendControl(desc.cbBlendControls[id], cb_blend_control);
         }
      }
   } else {
      for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
         auto& blend = desc.cbBlendControls[id];
         blend.colorCombFcn = latte::CB_COMB_FUNC::DST_PLUS_SRC;
         blend.colorSrcBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.colorDstBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.alphaCombFcn = latte::CB_COMB_FUNC::DST_PLUS_SRC;
         blend.alphaSrcBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.alphaDstBlend = latte::CB_BLEND_FUNC::ZERO;
         blend.opacityWeight = false;
      }
   }

   if (shouldUseBlendMasks) {
      auto cb_target_mask = getRegister<latte::CB_TARGET_MASK>(latte::Register::CB_TARGET_MASK);
      decaf_check(latte::MaxRenderTargets == 8);
      desc.cbBlendControls[0].targetMask = cb_target_mask.TARGET0_ENABLE();
      desc.cbBlendControls[1].targetMask = cb_target_mask.TARGET1_ENABLE();
      desc.cbBlendControls[2].targetMask = cb_target_mask.TARGET2_ENABLE();
      desc.cbBlendControls[3].targetMask = cb_target_mask.TARGET3_ENABLE();
      desc.cbBlendControls[4].targetMask = cb_target_mask.TARGET4_ENABLE();
      desc.cbBlendControls[5].targetMask = cb_target_mask.TARGET5_ENABLE();
      desc.cbBlendControls[6].targetMask = cb_target_mask.TARGET6_ENABLE();
      desc.cbBlendControls[7].targetMask = cb_target_mask.TARGET7_ENABLE();
   } else {
      for (auto id = 0u; id < latte::MaxRenderTargets; ++id) {
         desc.cbBlendControls[id].targetMask = 1 | 2 | 4 | 8;
      }
   }

   auto cb_blend_red = getRegister<latte::CB_BLEND_RED>(latte::Register::CB_BLEND_RED);
   auto cb_blend_green = getRegister<latte::CB_BLEND_GREEN>(latte::Register::CB_BLEND_GREEN);
   auto cb_blend_blue = getRegister<latte::CB_BLEND_BLUE>(latte::Register::CB_BLEND_BLUE);
   auto cb_blend_alpha = getRegister<latte::CB_BLEND_ALPHA>(latte::Register::CB_BLEND_ALPHA);
   desc.cbBlendConstants = {
      cb_blend_red.BLEND_RED(),
      cb_blend_green.BLEND_GREEN(),
      cb_blend_blue.BLEND_BLUE(),
      cb_blend_alpha.BLEND_ALPHA() };

   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
   auto sx_alpha_ref = getRegister<latte::SX_ALPHA_REF>(latte::Register::SX_ALPHA_REF);
   desc.alphaFunc = sx_alpha_test_control.ALPHA_FUNC();
   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE()) {
      desc.alphaFunc = latte::REF_FUNC::ALWAYS;
   }
   if (sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      desc.alphaFunc = latte::REF_FUNC::ALWAYS;
   }
   desc.alphaRef = sx_alpha_ref.ALPHA_REF();

   return desc;
}

bool
Driver::checkCurrentPipeline()
{
   decaf_check(mCurrentDraw->vertexShader);
   decaf_check(mCurrentDraw->renderPass);

   HashedDesc<PipelineDesc> currentDesc = getPipelineDesc();

   if (mCurrentDraw->pipeline && mCurrentDraw->pipeline->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundPipeline = mPipelines[currentDesc.hash()];
   if (foundPipeline) {
      mCurrentDraw->pipeline = foundPipeline;
      return true;
   }

   foundPipeline = new PipelineObject();
   foundPipeline->desc = currentDesc;

   // ------------------------------------------------------------
   // Pipeline Layout
   // ------------------------------------------------------------

   vk::PipelineLayout pipelineLayout;

   HashedDesc<PipelineLayoutDesc> pipelineLayoutDesc = generatePipelineLayoutDesc(*currentDesc);

   if (!ForceDescriptorSets && pipelineLayoutDesc->numDescriptors < 32) {
      auto pipelineLayoutObj = getPipelineLayout(pipelineLayoutDesc, true);
      foundPipeline->pipelineLayout = pipelineLayoutObj;
      pipelineLayout = pipelineLayoutObj->pipelineLayout;
   } else {
      // Too many descriptors to take advantage of using push descriptors, we have to
      // fall back to using dynamically generated descriptor sets.
      foundPipeline->pipelineLayout = nullptr;
      pipelineLayout = mPipelineLayout;
   }


   // ------------------------------------------------------------
   // Shader Stages
   // ------------------------------------------------------------

   std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
   if (currentDesc->vertexShader) {
      vk::PipelineShaderStageCreateInfo shaderStageDesc;
      shaderStageDesc.stage = vk::ShaderStageFlagBits::eVertex;
      shaderStageDesc.module = currentDesc->vertexShader->module;
      shaderStageDesc.pName = "main";
      shaderStageDesc.pSpecializationInfo = nullptr;
      shaderStages.push_back(shaderStageDesc);
   }
   if (currentDesc->geometryShader) {
      vk::PipelineShaderStageCreateInfo shaderStageDesc;
      shaderStageDesc.stage = vk::ShaderStageFlagBits::eGeometry;
      shaderStageDesc.module = currentDesc->geometryShader->module;
      shaderStageDesc.pName = "main";
      shaderStageDesc.pSpecializationInfo = nullptr;
      shaderStages.push_back(shaderStageDesc);
   }
   if (currentDesc->pixelShader) {
      vk::PipelineShaderStageCreateInfo shaderStageDesc;
      shaderStageDesc.stage = vk::ShaderStageFlagBits::eFragment;
      shaderStageDesc.module = currentDesc->pixelShader->module;
      shaderStageDesc.pName = "main";
      shaderStageDesc.pSpecializationInfo = nullptr;
      shaderStages.push_back(shaderStageDesc);
   }
   if (currentDesc->rectStubShader) {
      decaf_check(!currentDesc->geometryShader);

      vk::PipelineShaderStageCreateInfo rectStageDesc;
      rectStageDesc.stage = vk::ShaderStageFlagBits::eGeometry;
      rectStageDesc.module = currentDesc->rectStubShader->module;
      rectStageDesc.pName = "main";
      rectStageDesc.pSpecializationInfo = nullptr;
      shaderStages.push_back(rectStageDesc);
   }


   // ------------------------------------------------------------
   // Attribute buffers and shader attributes
   // ------------------------------------------------------------

   std::vector<vk::VertexInputBindingDescription> bindingDescs;
   std::vector<vk::VertexInputBindingDivisorDescriptionEXT> divisorDescs;

   const auto& inputBuffers = currentDesc->vertexShader->shader.meta.attribBuffers;
   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      const auto &inputBuffer = inputBuffers[i];

      if (!inputBuffer.isUsed) {
         continue;
      }

      vk::VertexInputBindingDescription bindingDesc;
      bindingDesc.binding = i;
      bindingDesc.stride = currentDesc->attribBufferStride[i];
      if (inputBuffer.indexMode == spirv::AttribBuffer::IndexMode::PerVertex) {
         decaf_check(inputBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::CONST_1);
         bindingDesc.inputRate = vk::VertexInputRate::eVertex;
      } else if (inputBuffer.indexMode == spirv::AttribBuffer::IndexMode::PerInstance) {
         uint32_t divisorValue;
         if (inputBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::CONST_1) {
            divisorValue = 1;
         } else if (inputBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::REGISTER_0) {
            divisorValue = currentDesc->attribBufferDivisor[0];
         } else if (inputBuffer.divisorMode == spirv::AttribBuffer::DivisorMode::REGISTER_1) {
            divisorValue = currentDesc->attribBufferDivisor[1];
         } else {
            decaf_abort("Unexpected divisor mode in vertex shader attributes");
         }

         bindingDesc.inputRate = vk::VertexInputRate::eInstance;

         if (divisorValue != 1) {
            vk::VertexInputBindingDivisorDescriptionEXT divisorDesc;
            divisorDesc.binding = bindingDesc.binding;
            divisorDesc.divisor = divisorValue;
            divisorDescs.push_back(divisorDesc);
         }
      } else {
         decaf_abort("Unexpected indexing mode for buffer");
      }
      bindingDescs.push_back(bindingDesc);
   }

   std::vector<vk::VertexInputAttributeDescription> attribDescs;

   const auto& inputAttribs = currentDesc->vertexShader->shader.meta.attribElems;
   for (auto i = 0u; i < inputAttribs.size(); ++i) {
      const auto &inputAttrib = inputAttribs[i];

      vk::VertexInputAttributeDescription attribDesc;
      attribDesc.location = i;
      attribDesc.binding = inputAttrib.bufferIndex;

      if (inputAttrib.elemWidth == 8) {
         if (inputAttrib.elemCount == 1) {
            attribDesc.format = vk::Format::eR8Uint;
         } else if (inputAttrib.elemCount == 2) {
            attribDesc.format = vk::Format::eR8G8Uint;
         } else if (inputAttrib.elemCount == 3) {
            attribDesc.format = vk::Format::eR8G8B8Uint;
         } else if (inputAttrib.elemCount == 4) {
            attribDesc.format = vk::Format::eR8G8B8A8Uint;
         } else {
            decaf_abort("Unexpected vertex attribute element count");
         }
      } else if (inputAttrib.elemWidth == 16) {
         if (inputAttrib.elemCount == 1) {
            attribDesc.format = vk::Format::eR16Uint;
         } else if (inputAttrib.elemCount == 2) {
            attribDesc.format = vk::Format::eR16G16Uint;
         } else if (inputAttrib.elemCount == 3) {
            attribDesc.format = vk::Format::eR16G16B16Uint;
         } else if (inputAttrib.elemCount == 4) {
            attribDesc.format = vk::Format::eR16G16B16A16Uint;
         } else {
            decaf_abort("Unexpected vertex attribute element count");
         }
      } else if (inputAttrib.elemWidth == 32) {
         if (inputAttrib.elemCount == 1) {
            attribDesc.format = vk::Format::eR32Uint;
         } else if (inputAttrib.elemCount == 2) {
            attribDesc.format = vk::Format::eR32G32Uint;
         } else if (inputAttrib.elemCount == 3) {
            attribDesc.format = vk::Format::eR32G32B32Uint;
         } else if (inputAttrib.elemCount == 4) {
            attribDesc.format = vk::Format::eR32G32B32A32Uint;
         } else {
            decaf_abort("Unexpected vertex attribute element count");
         }
      } else {
         decaf_abort("Unexpected vertex attribute element width");
      }

      attribDesc.offset = inputAttrib.offset;
      attribDescs.push_back(attribDesc);
   }

   vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
   vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
   vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
   vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

   if (divisorDescs.size() > 0) {
      vk::PipelineVertexInputDivisorStateCreateInfoEXT divisorBindingDesc;
      divisorBindingDesc.vertexBindingDivisorCount = static_cast<uint32_t>(divisorDescs.size());
      divisorBindingDesc.pVertexBindingDivisors = divisorDescs.data();

      vertexInputInfo.pNext = &divisorBindingDesc;
   }

   // ------------------------------------------------------------
   // Input assembly
   // ------------------------------------------------------------

   vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
   switch (currentDesc->primitiveType) {
   case latte::VGT_DI_PRIMITIVE_TYPE::POINTLIST:
      inputAssembly.topology = vk::PrimitiveTopology::ePointList;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::LINELIST:
      inputAssembly.topology = vk::PrimitiveTopology::eLineList;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::LINESTRIP:
      inputAssembly.topology = vk::PrimitiveTopology::eLineStrip;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::LINELIST_ADJ:
      inputAssembly.topology = vk::PrimitiveTopology::eLineListWithAdjacency;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::LINESTRIP_ADJ:
      inputAssembly.topology = vk::PrimitiveTopology::eLineStripWithAdjacency;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::TRILIST:
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::TRIFAN:
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleFan;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::TRISTRIP:
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::TRILIST_ADJ:
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::TRISTRIP_ADJ:
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleStripWithAdjacency;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::RECTLIST:
      // We use a custom geometry shader to translate these
      inputAssembly.topology = vk::PrimitiveTopology::eLineListWithAdjacency;
      break;
   case latte::VGT_DI_PRIMITIVE_TYPE::QUADLIST:
   case latte::VGT_DI_PRIMITIVE_TYPE::QUADSTRIP:
      // We handle translation of these types during draw
      inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
      break;
      //case latte::VGT_DI_PRIMITIVE_TYPE::NONE:
      //case latte::VGT_DI_PRIMITIVE_TYPE::TRI_WITH_WFLAGS:
      //case latte::VGT_DI_PRIMITIVE_TYPE::LINELOOP:
      //case latte::VGT_DI_PRIMITIVE_TYPE::POLYGON:
      //case latte::VGT_DI_PRIMITIVE_TYPE::COPY_RECT_LIST_2D_V0:
      //case latte::VGT_DI_PRIMITIVE_TYPE::COPY_RECT_LIST_2D_V1:
      //case latte::VGT_DI_PRIMITIVE_TYPE::COPY_RECT_LIST_2D_V2:
      //case latte::VGT_DI_PRIMITIVE_TYPE::COPY_RECT_LIST_2D_V3:
      //case latte::VGT_DI_PRIMITIVE_TYPE::FILL_RECT_LIST_2D:
      //case latte::VGT_DI_PRIMITIVE_TYPE::LINE_STRIP_2D:
      //case latte::VGT_DI_PRIMITIVE_TYPE::TRI_STRIP_2D:
   default:
      decaf_abort("Unexpected VGT primitive type");
   }

   // Technically there is another valid configuration using 32-bit indices and a reset of 0xFFFF,
   // but checking for this requires more state input to the pipeline...
   inputAssembly.primitiveRestartEnable = currentDesc->primitiveResetEnabled;
   decaf_check(currentDesc->primitiveResetIndex == 0xFFFF || currentDesc->primitiveResetIndex == 0xFFFFFFFF);

   // ------------------------------------------------------------
   // Viewports and Scissors
   // ------------------------------------------------------------

   vk::PipelineViewportStateCreateInfo viewportState;
   viewportState.viewportCount = 1;
   viewportState.pViewports = nullptr;
   viewportState.scissorCount = 1;
   viewportState.pScissors = nullptr;


   // ------------------------------------------------------------
   // Rasterizer controls
   // ------------------------------------------------------------
   // TODO: Implement support for doing multi-sampled rendering.

   vk::PipelineRasterizationStateCreateInfo rasterizer;
   rasterizer.depthClampEnable = currentDesc->zclipDisabled;
   rasterizer.rasterizerDiscardEnable = currentDesc->rasteriserDisable;

   if (currentDesc->polyPType == latte::PA_PTYPE::LINES) {
      rasterizer.polygonMode = vk::PolygonMode::eLine;
   } else if (currentDesc->polyPType == latte::PA_PTYPE::POINTS) {
      rasterizer.polygonMode = vk::PolygonMode::ePoint;
   } else if (currentDesc->polyPType == latte::PA_PTYPE::TRIANGLES) {
      rasterizer.polygonMode = vk::PolygonMode::eFill;
   } else {
      decaf_abort("Unexpected rasterization ptype");
   }

   if (currentDesc->cullFront && currentDesc->cullBack) {
      rasterizer.cullMode = vk::CullModeFlagBits::eFrontAndBack;
   } else if (currentDesc->cullFront) {
      rasterizer.cullMode = vk::CullModeFlagBits::eFront;
   } else if (currentDesc->cullBack) {
      rasterizer.cullMode = vk::CullModeFlagBits::eBack;
   } else {
      rasterizer.cullMode = vk::CullModeFlagBits::eNone;
   }

   if (currentDesc->paFace == latte::PA_FACE::CW) {
      rasterizer.frontFace = vk::FrontFace::eClockwise;
   } else if (currentDesc->paFace == latte::PA_FACE::CCW) {
      rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
   } else {
      decaf_abort("Unexpected pipeline cull mode");
   }

   rasterizer.depthBiasEnable = currentDesc->polyBiasEnabled;
   if (rasterizer.depthBiasEnable) {
      rasterizer.depthBiasConstantFactor = currentDesc->polyBiasOffset;
      rasterizer.depthBiasClamp = currentDesc->polyBiasClamp;
      rasterizer.depthBiasSlopeFactor = currentDesc->polyBiasScale;
   }

   rasterizer.lineWidth = static_cast<float>(currentDesc->lineWidth) / 8.0f;

   vk::PipelineMultisampleStateCreateInfo multisampling;
   multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
   multisampling.sampleShadingEnable = false;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = false;
   multisampling.alphaToOneEnable = false;


   // ------------------------------------------------------------
   // Color Blending
   // ------------------------------------------------------------

   std::array<vk::PipelineColorBlendAttachmentState, latte::MaxRenderTargets> colorBlendAttachments;
   std::array<bool, latte::MaxRenderTargets> targetIsPremultiplied = { false };
   auto needsPremultipliedTargets = false;

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      const auto& blendData = currentDesc->cbBlendControls[i];

      vk::PipelineColorBlendAttachmentState colorBlendAttachment;
      colorBlendAttachment.blendEnable = blendData.blendingEnabled;
      if (colorBlendAttachment.blendEnable) {
         colorBlendAttachment.srcColorBlendFactor = getVkBlendFactor(blendData.colorSrcBlend);
         colorBlendAttachment.dstColorBlendFactor = getVkBlendFactor(blendData.colorDstBlend);
         colorBlendAttachment.colorBlendOp = getVkBlendOp(blendData.colorCombFcn);
         colorBlendAttachment.srcAlphaBlendFactor = getVkBlendFactor(blendData.alphaSrcBlend);
         colorBlendAttachment.dstAlphaBlendFactor = getVkBlendFactor(blendData.alphaDstBlend);
         colorBlendAttachment.alphaBlendOp = getVkBlendOp(blendData.alphaCombFcn);

         if (blendData.opacityWeight) {
            needsPremultipliedTargets = true;
            targetIsPremultiplied[i] = true;
         }
      }

      if (blendData.targetMask & 1) {
         colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
      }
      if (blendData.targetMask & 2) {
         colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
      }
      if (blendData.targetMask & 4) {
         colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
      }
      if (blendData.targetMask & 8) {
         colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;
      }

      colorBlendAttachments[i] = colorBlendAttachment;
   }

   vk::PipelineColorBlendStateCreateInfo colorBlendState;

   if (currentDesc->rop3 == 0xCC) {
      // COPY
      colorBlendState.logicOpEnable = false;
      colorBlendState.logicOp = vk::LogicOp::eCopy;
   } else {
      colorBlendState.logicOpEnable = true;
      switch (currentDesc->rop3) {
      case 0x00: // BLACKNESS
         colorBlendState.logicOp = vk::LogicOp::eClear;
         break;
      case 0x11: // NOTSRCERASE
         colorBlendState.logicOp = vk::LogicOp::eNor;
         break;
      case 0x22:
         colorBlendState.logicOp = vk::LogicOp::eAndInverted;
         break;
      case 0x33: // NOTSRCCOPY
         colorBlendState.logicOp = vk::LogicOp::eCopyInverted;
         break;
      case 0x44: // SRCERASE
         colorBlendState.logicOp = vk::LogicOp::eAndReverse;
         break;
      case 0x55: // DSTINVERT
         colorBlendState.logicOp = vk::LogicOp::eInvert;
         break;
      case 0x66: // SRCINVERT
         colorBlendState.logicOp = vk::LogicOp::eXor;
         break;
      case 0x77:
         colorBlendState.logicOp = vk::LogicOp::eNand;
         break;
      case 0x88: // SRCAND
         colorBlendState.logicOp = vk::LogicOp::eAnd;
         break;
      case 0x99:
         colorBlendState.logicOp = vk::LogicOp::eEquivalent;
         break;
      case 0xAA:
         colorBlendState.logicOp = vk::LogicOp::eNoOp;
         break;
      case 0xBB: // MERGEPAINT
         colorBlendState.logicOp = vk::LogicOp::eOrInverted;
         break;
      case 0xDD:
         colorBlendState.logicOp = vk::LogicOp::eOrReverse;
         break;
      case 0xEE: // SRCPAINT
         colorBlendState.logicOp = vk::LogicOp::eOr;
         break;
      case 0xFF: // WHITENESS
         colorBlendState.logicOp = vk::LogicOp::eSet;
         break;
      case 0x5A: // PATINVERT
      case 0xF0: // PATCOPY
      default:
         decaf_abort("Unexpected ROP3 operation");
      }
   }

   colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
   colorBlendState.pAttachments = colorBlendAttachments.data();
   colorBlendState.blendConstants[0] = currentDesc->cbBlendConstants[0];
   colorBlendState.blendConstants[1] = currentDesc->cbBlendConstants[1];
   colorBlendState.blendConstants[2] = currentDesc->cbBlendConstants[2];
   colorBlendState.blendConstants[3] = currentDesc->cbBlendConstants[3];

   vk::PipelineColorBlendAdvancedStateCreateInfoEXT advancedColorBlendState;
   if (needsPremultipliedTargets) {
      advancedColorBlendState.dstPremultiplied = true;
      advancedColorBlendState.srcPremultiplied = false;
      advancedColorBlendState.blendOverlap = vk::BlendOverlapEXT::eUncorrelated;

      colorBlendState.pNext = &advancedColorBlendState;
   }

   // ------------------------------------------------------------
   // Depth/Stencil State
   // ------------------------------------------------------------
   vk::PipelineDepthStencilStateCreateInfo depthStencil;
   depthStencil.depthBoundsTestEnable = false;

   depthStencil.depthTestEnable = currentDesc->zEnable;
   depthStencil.depthWriteEnable = currentDesc->zWriteEnable;
   depthStencil.depthCompareOp = getVkCompareOp(currentDesc->zFunc);

   depthStencil.stencilTestEnable = currentDesc->stencilEnable;
   depthStencil.front.compareOp = getVkCompareOp(currentDesc->stencilFront.compareFunc);
   depthStencil.front.failOp = getVkStencilOp(currentDesc->stencilFront.failOp);
   depthStencil.front.passOp = getVkStencilOp(currentDesc->stencilFront.zPassOp);
   depthStencil.front.depthFailOp = getVkStencilOp(currentDesc->stencilFront.zFailOp);
   depthStencil.front.reference = static_cast<uint32_t>(currentDesc->stencilFront.ref);
   depthStencil.front.compareMask = static_cast<uint32_t>(currentDesc->stencilFront.mask);
   depthStencil.front.writeMask = static_cast<uint32_t>(currentDesc->stencilFront.writeMask);
   depthStencil.back.compareOp = getVkCompareOp(currentDesc->stencilBack.compareFunc);
   depthStencil.back.failOp = getVkStencilOp(currentDesc->stencilBack.failOp);
   depthStencil.back.passOp = getVkStencilOp(currentDesc->stencilBack.zPassOp);
   depthStencil.back.depthFailOp = getVkStencilOp(currentDesc->stencilBack.zFailOp);
   depthStencil.back.reference = static_cast<uint32_t>(currentDesc->stencilBack.ref);
   depthStencil.back.compareMask = static_cast<uint32_t>(currentDesc->stencilBack.mask);
   depthStencil.back.writeMask = static_cast<uint32_t>(currentDesc->stencilBack.writeMask);


   // ------------------------------------------------------------
   // Dynamic states
   // ------------------------------------------------------------

   auto dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
   };

   vk::PipelineDynamicStateCreateInfo dynamicDesc;
   dynamicDesc.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicDesc.pDynamicStates = dynamicStates.begin();


   // ------------------------------------------------------------
   // Shader stuff
   // ------------------------------------------------------------

   // TODO: I don't understand why the vulkan drivers use the color from the
   // shader when rendering this stuff... We have to do this for now.
   auto shaderLopMode = 0;
   if (currentDesc->rop3 == 0xFF) {
      shaderLopMode = 1;
   } else if (currentDesc->rop3 == 0x00) {
      shaderLopMode = 2;
   }


   // ------------------------------------------------------------
   // Pipeline
   // ------------------------------------------------------------

   vk::GraphicsPipelineCreateInfo pipelineInfo;
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = &depthStencil;
   pipelineInfo.pColorBlendState = &colorBlendState;
   pipelineInfo.pDynamicState = &dynamicDesc;
   pipelineInfo.layout = pipelineLayout;
   pipelineInfo.renderPass = mCurrentDraw->renderPass->renderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = vk::Pipeline();
   pipelineInfo.basePipelineIndex = -1;
   auto pipeline = mDevice.createGraphicsPipeline(mPipelineCache, pipelineInfo);

   foundPipeline->pipeline = pipeline;
   foundPipeline->needsPremultipliedTargets = needsPremultipliedTargets;
   foundPipeline->targetIsPremultiplied = targetIsPremultiplied;
   foundPipeline->shaderLopMode = shaderLopMode;
   foundPipeline->shaderAlphaFunc = currentDesc->alphaFunc;
   foundPipeline->shaderAlphaRef = currentDesc->alphaRef;
   mCurrentDraw->pipeline = foundPipeline;
   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
