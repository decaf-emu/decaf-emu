#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

void Transpiler::translateTex_FETCH4(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_FETCH4(cf, inst);
}

void Transpiler::translateTex_SAMPLE(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   // inst.word0.FETCH_WHOLE_QUAD(); - Optimization which can be ignored
   // inst.word1.LOD_BIAS(); - Only used by certain SAMPLE instructions

   decaf_check(!inst.word0.BC_FRAC_MODE());

   auto textureId = inst.word0.RESOURCE_ID();
   auto samplerId = inst.word2.SAMPLER_ID();
   auto texDim = mTexInput[textureId];

   switch (texDim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      decaf_check(inst.word1.COORD_TYPE_X() == SQ_TEX_COORD_TYPE::NORMALIZED);
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      decaf_check(inst.word1.COORD_TYPE_X() == SQ_TEX_COORD_TYPE::NORMALIZED);
      decaf_check(inst.word1.COORD_TYPE_Y() == SQ_TEX_COORD_TYPE::UNNORMALIZED);
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      decaf_check(inst.word1.COORD_TYPE_X() == SQ_TEX_COORD_TYPE::NORMALIZED);
      decaf_check(inst.word1.COORD_TYPE_Y() == SQ_TEX_COORD_TYPE::NORMALIZED);
      decaf_check(inst.word1.COORD_TYPE_Z() == SQ_TEX_COORD_TYPE::NORMALIZED);
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      decaf_check(inst.word1.COORD_TYPE_X() == SQ_TEX_COORD_TYPE::NORMALIZED);
      decaf_check(inst.word1.COORD_TYPE_Y() == SQ_TEX_COORD_TYPE::NORMALIZED);
      decaf_check(inst.word1.COORD_TYPE_Z() == SQ_TEX_COORD_TYPE::UNNORMALIZED);
      break;
   default:
      decaf_abort("Unexpected texture sample dim");
   }

   GprMaskRef srcGpr;
   srcGpr.gpr = makeGprRef(inst.word0.SRC_GPR(), inst.word0.SRC_REL(), SQ_INDEX_MODE::LOOP);
   srcGpr.mask[SQ_CHAN::X] = inst.word2.SRC_SEL_X();
   srcGpr.mask[SQ_CHAN::Y] = inst.word2.SRC_SEL_Y();
   srcGpr.mask[SQ_CHAN::Z] = inst.word2.SRC_SEL_Z();
   srcGpr.mask[SQ_CHAN::W] = inst.word2.SRC_SEL_W();

   GprMaskRef destGpr;
   destGpr.gpr = makeGprRef(inst.word1.DST_GPR(), inst.word1.DST_REL(), SQ_INDEX_MODE::LOOP);
   destGpr.mask[SQ_CHAN::X] = inst.word1.DST_SEL_X();
   destGpr.mask[SQ_CHAN::Y] = inst.word1.DST_SEL_Y();
   destGpr.mask[SQ_CHAN::Z] = inst.word1.DST_SEL_Z();
   destGpr.mask[SQ_CHAN::W] = inst.word1.DST_SEL_W();

   auto srcGprVal = mSpv->readGprMaskRef(srcGpr);

   auto texVarType = mSpv->textureVarType(textureId, texDim);
   auto sampVar = mSpv->samplerVar(samplerId);
   auto texVar = mSpv->textureVar(textureId, texDim);

   auto sampVal = mSpv->createLoad(sampVar);
   auto texVal = mSpv->createLoad(texVar);

   auto sampledType = mSpv->makeSampledImageType(texVarType);
   auto sampledVal = mSpv->createOp(spv::OpSampledImage, sampledType, { texVal, sampVal });
   auto output = mSpv->createOp(spv::OpImageSampleImplicitLod, mSpv->float4Type(), { sampledVal, srcGprVal });

   mSpv->writeGprMaskRef(destGpr, output);
}

void Transpiler::translateTex_SAMPLE_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_L(cf, inst);
}

void Transpiler::translateTex_SAMPLE_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_LB(cf, inst);
}

void Transpiler::translateTex_SAMPLE_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_LZ(cf, inst);
}

void Transpiler::translateTex_SAMPLE_G(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_G(cf, inst);
}

void Transpiler::translateTex_SAMPLE_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_G_L(cf, inst);
}

void Transpiler::translateTex_SAMPLE_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_G_LB(cf, inst);
}

void Transpiler::translateTex_SAMPLE_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_G_LZ(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_L(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_LB(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_LZ(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_G(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_G(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_G_L(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_G_LB(cf, inst);
}

void Transpiler::translateTex_SAMPLE_C_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   latte::ShaderParser::translateTex_SAMPLE_C_G_LZ(cf, inst);
}

void Transpiler::translateTex_SET_CUBEMAP_INDEX(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   // TODO: It is possible that we are supposed to somehow force
   //  a specific face to be used in spite of the coordinates.
}

void Transpiler::translateTex_VTX_FETCH(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   // The TextureFetchInst perfectly matches the VertexFetchInst
   translateVtx_FETCH(cf, *reinterpret_cast<const VertexFetchInst*>(&inst));
}

void Transpiler::translateTex_VTX_SEMANTIC(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   // The TextureFetchInst perfectly matches the VertexFetchInst
   translateVtx_SEMANTIC(cf, *reinterpret_cast<const VertexFetchInst*>(&inst));
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
