#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

void Transpiler::translateGenericSample(const ControlFlowInst &cf, const TextureFetchInst &inst, uint32_t sampleMode)
{
   // inst.word0.FETCH_WHOLE_QUAD(); - Optimization which can be ignored
   // inst.word1.LOD_BIAS(); - Only used by certain SAMPLE instructions

   decaf_check(!inst.word0.BC_FRAC_MODE());

   if (mType != ShaderParser::Type::Pixel) {
      // If we are not in the pixel shader, we need to automatically use
      // LOD zero, even if implicit LOD is specified by the instruction.
      if (!(sampleMode & SampleMode::Gather)) {
         sampleMode |= SampleMode::LodZero;
      }
   }

   auto textureId = inst.word0.RESOURCE_ID();
   auto samplerId = inst.word2.SAMPLER_ID();
   auto texDim = mTexDims[textureId];
   auto texFormat = mTexFormats[textureId];

   auto expectedCoordType = SQ_TEX_COORD_TYPE::NORMALIZED;
   if (sampleMode & SampleMode::Load) {
      expectedCoordType = SQ_TEX_COORD_TYPE::UNNORMALIZED;
   }

   switch (texDim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      decaf_check(inst.word1.COORD_TYPE_X() == expectedCoordType);
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      decaf_check(inst.word1.COORD_TYPE_X() == expectedCoordType);
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      decaf_check(inst.word1.COORD_TYPE_X() == expectedCoordType);
      decaf_check(inst.word1.COORD_TYPE_Y() == expectedCoordType);
      decaf_check(inst.word1.COORD_TYPE_Z() == expectedCoordType);
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      decaf_check(inst.word1.COORD_TYPE_X() == expectedCoordType);
      decaf_check(inst.word1.COORD_TYPE_Y() == expectedCoordType);
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

   auto lodBias = inst.word1.LOD_BIAS();
   auto offsetX = inst.word2.OFFSET_X();
   auto offsetY = inst.word2.OFFSET_Y();
   auto offsetZ = inst.word2.OFFSET_Z();

   auto srcGprVal = mSpv->readGprMaskRef(srcGpr);

   uint32_t imageOperands = 0;
   std::vector<unsigned int> operandParams;

   if (sampleMode & SampleMode::LodBias) {
      imageOperands |= spv::ImageOperandsBiasMask;

      auto lodSource = mSpv->makeFloatConstant(lodBias);
      operandParams.push_back(lodSource);
   }

   if (sampleMode & SampleMode::Lod) {
      imageOperands |= spv::ImageOperandsLodMask;

      auto lodSource = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { srcGprVal, 3 });
      operandParams.push_back(lodSource);
   } else if (sampleMode & SampleMode::LodZero) {
      imageOperands |= spv::ImageOperandsLodMask;

      auto lodSource = mSpv->makeFloatConstant(0.0f);
      operandParams.push_back(lodSource);
   }

   if (sampleMode & SampleMode::Gradient) {
      decaf_abort("We do not currently support gradient instructions");
   }

   if (offsetX > 0 || offsetY > 0 || offsetZ > 0) {
      imageOperands |= spv::ImageOperandsConstOffsetMask;

      spv::Id offsetVal = spv::NoResult;
      switch (texDim) {
      case latte::SQ_TEX_DIM::DIM_1D:
         offsetVal = mSpv->makeUintConstant(offsetX);
         break;
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D:
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         offsetVal = mSpv->makeCompositeConstant(mSpv->uint2Type(), {
                                                 mSpv->makeUintConstant(offsetX),
                                                 mSpv->makeUintConstant(offsetY) });
         break;
      case latte::SQ_TEX_DIM::DIM_3D:
         offsetVal = mSpv->makeCompositeConstant(mSpv->uint3Type(), {
                                                 mSpv->makeUintConstant(offsetX),
                                                 mSpv->makeUintConstant(offsetY),
                                                 mSpv->makeUintConstant(offsetZ) });
         break;
      default:
         decaf_abort("Unexpected texture sample dim");
      }

      operandParams.push_back(offsetVal);
   }

   auto texVarType = mSpv->textureVarType(textureId, texDim, texFormat);
   auto texVar = mSpv->textureVar(textureId, texDim, texFormat);
   auto texVal = mSpv->createLoad(texVar);

   // Lets build our actual operation
   spv::Op sampleOp = spv::OpNop;
   std::vector<unsigned int> sampleParams;

   if (!(sampleMode & SampleMode::Load)) {
      auto sampVar = mSpv->samplerVar(samplerId);
      auto sampVal = mSpv->createLoad(sampVar);
      auto sampledType = mSpv->makeSampledImageType(texVarType);
      auto sampledVal = mSpv->createOp(spv::OpSampledImage, sampledType, { texVal, sampVal });
      sampleParams.push_back(sampledVal);

      spv::Id srcCoords = srcGprVal;
      switch (texDim) {
      case latte::SQ_TEX_DIM::DIM_1D:
         srcCoords = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { srcCoords, 0 });
         break;
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D:
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         srcCoords = mSpv->createOp(spv::OpVectorShuffle, mSpv->float2Type(), { srcCoords, srcCoords, 0, 1 });
         break;
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      case latte::SQ_TEX_DIM::DIM_3D:
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         srcCoords = mSpv->createOp(spv::OpVectorShuffle, mSpv->float3Type(), { srcCoords, srcCoords, 0, 1, 2 });
         break;
      default:
         decaf_abort("Unexpected texture sample dim");
      }
      sampleParams.push_back(srcCoords);

      // Pick the operation, and potentially add the comparison.
      if (sampleMode & SampleMode::Gather) {
         // It is not legal to combine this with any other kind of operation
         decaf_check(sampleMode == SampleMode::Gather);

         sampleOp = spv::OpImageGather;

         auto compToFetch = mSpv->makeUintConstant(0);
         sampleParams.push_back(compToFetch);
      } else if (sampleMode & SampleMode::Compare) {
         auto drefVal = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { srcGprVal, 3 });

         if (imageOperands & spv::ImageOperandsLodMask) {
            sampleOp = spv::OpImageSampleDrefExplicitLod;
         } else {
            sampleOp = spv::OpImageSampleDrefImplicitLod;
         }

         sampleParams.push_back(drefVal);
      } else {
         if (imageOperands & spv::ImageOperandsLodMask) {
            sampleOp = spv::OpImageSampleExplicitLod;
         } else {
            sampleOp = spv::OpImageSampleImplicitLod;
         }
      }
   } else {
      // It is not legal to combine this with any other kind of operation
      decaf_check(sampleMode == SampleMode::Load);

      spv::Id srcCoords = srcGprVal;
      srcCoords = mSpv->createUnaryOp(spv::OpBitcast, mSpv->uint4Type(), srcCoords);

      switch (texDim) {
      case latte::SQ_TEX_DIM::DIM_1D:
         srcCoords = mSpv->createOp(spv::OpCompositeExtract, mSpv->uintType(), { srcCoords, 0 });
         break;
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D:
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         srcCoords = mSpv->createOp(spv::OpVectorShuffle, mSpv->uint2Type(), { srcCoords, srcCoords, 0, 1 });
         break;
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      case latte::SQ_TEX_DIM::DIM_3D:
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         srcCoords = mSpv->createOp(spv::OpVectorShuffle, mSpv->uint3Type(), { srcCoords, srcCoords, 0, 1, 2 });
         break;
      default:
         decaf_abort("Unexpected texture sample dim");
      }

      sampleParams.push_back(texVal);
      sampleParams.push_back(srcCoords);

      sampleOp = spv::OpImageFetch;
   }

   decaf_check(sampleOp != spv::OpNop);

   // Merge all the image operands in now.
   if (imageOperands != 0) {
      sampleParams.push_back(imageOperands);

      decaf_check(!operandParams.empty());
      for (auto& param : operandParams) {
         sampleParams.push_back(param);
      }
   } else {
      decaf_check(operandParams.empty());
   }

   auto zeroFVal = mSpv->makeFloatConstant(0.0f);
   auto oneFVal = mSpv->makeFloatConstant(1.0f);

   spv::Id output = spv::NoResult;
   if (sampleMode & SampleMode::Compare) {
      // We do not support doing depth comparison on a UINT depth buffer
      decaf_check(texFormat == TextureInputType::FLOAT);

      auto compareVal = mSpv->createOp(sampleOp, mSpv->floatType(), sampleParams);

      // TODO: This is really cheating, we should just check the write mask
      // and do a single component write instead...
      output = mSpv->createCompositeConstruct(mSpv->float4Type(), { compareVal, zeroFVal, zeroFVal, oneFVal });
   } else {
      if (texFormat == TextureInputType::FLOAT) {
         output = mSpv->createOp(sampleOp, mSpv->float4Type(), sampleParams);
      } else if (texFormat == TextureInputType::INT) {
         output = mSpv->createOp(sampleOp, mSpv->uint4Type(), sampleParams);
      } else {
         decaf_abort("Unexpected texture format type");
      }
   }

   mSpv->writeGprMaskRef(destGpr, output);
}

void Transpiler::translateTex_LD(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Load);
}

void Transpiler::translateTex_FETCH4(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Gather);
}

void Transpiler::translateTex_SAMPLE(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::None);
}

void Transpiler::translateTex_SAMPLE_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Lod);
}

void Transpiler::translateTex_SAMPLE_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::LodBias);
}

void Transpiler::translateTex_SAMPLE_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::LodZero);
}

void Transpiler::translateTex_SAMPLE_G(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Gradient);
}

void Transpiler::translateTex_SAMPLE_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Gradient | SampleMode::Lod);
}

void Transpiler::translateTex_SAMPLE_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Gradient | SampleMode::LodBias);
}

void Transpiler::translateTex_SAMPLE_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Gradient | SampleMode::LodZero);
}

void Transpiler::translateTex_SAMPLE_C(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare);
}

void Transpiler::translateTex_SAMPLE_C_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::Lod);
}

void Transpiler::translateTex_SAMPLE_C_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::LodBias);
}

void Transpiler::translateTex_SAMPLE_C_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::LodZero);
}

void Transpiler::translateTex_SAMPLE_C_G(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::Gradient);
}

void Transpiler::translateTex_SAMPLE_C_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::Gradient | SampleMode::Lod);
}

void Transpiler::translateTex_SAMPLE_C_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::Gradient | SampleMode::LodBias);
}

void Transpiler::translateTex_SAMPLE_C_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   translateGenericSample(cf, inst, SampleMode::Compare | SampleMode::Gradient | SampleMode::LodZero);
}

void Transpiler::translateTex_GET_TEXTURE_INFO(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   auto textureId = inst.word0.RESOURCE_ID();
   auto samplerId = inst.word2.SAMPLER_ID();
   auto texDim = mTexDims[textureId];
   auto texFormat = mTexFormats[textureId];

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

   // We have to register the image-query capability in order to query texture data.
   mSpv->addCapability(spv::CapabilityImageQuery);

   auto texVar = mSpv->textureVar(textureId, texDim, texFormat);
   auto image = mSpv->createLoad(texVar);

   auto srcGprVal = mSpv->readGprMaskRef(srcGpr);
   auto srcLodValFloat = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { srcGprVal, 0 });
   auto srcLodVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->intType(), srcLodValFloat);

   auto oneIConst = mSpv->makeIntConstant(1);
   auto sizeIConst = mSpv->makeIntConstant(6);

   spv::Id sizeInfo;
   switch (texDim) {
   case latte::SQ_TEX_DIM::DIM_1D: {
      auto sizeInfo1d = mSpv->createBinOp(spv::OpImageQuerySizeLod, mSpv->intType(), image, srcLodVal);
      sizeInfo = mSpv->createOp(spv::OpCompositeConstruct, mSpv->int3Type(), { sizeInfo1d, oneIConst, oneIConst });
      break;
   }
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA: {
      auto sizeInfo2d = mSpv->createBinOp(spv::OpImageQuerySizeLod, mSpv->int2Type(), image, srcLodVal);
      sizeInfo = mSpv->createOp(spv::OpCompositeConstruct, mSpv->int3Type(), { sizeInfo2d, oneIConst });
      break;
   }
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
   case latte::SQ_TEX_DIM::DIM_3D: {
      sizeInfo = mSpv->createBinOp(spv::OpImageQuerySizeLod, mSpv->int3Type(), image, srcLodVal);
      break;
   }
   case latte::SQ_TEX_DIM::DIM_CUBEMAP: {
      auto sizeInfoCube = mSpv->createBinOp(spv::OpImageQuerySizeLod, mSpv->int3Type(), image, srcLodVal);
      auto cubemapArrSideSize = mSpv->createOp(spv::OpCompositeExtract, mSpv->intType(), { sizeInfoCube, 3 });
      auto cubemapArrSize = mSpv->createBinOp(spv::OpSDiv, mSpv->intType(), cubemapArrSideSize, sizeIConst);
      sizeInfo = mSpv->createOp(spv::OpCompositeInsert, mSpv->int3Type(), { cubemapArrSize, sizeInfoCube, 3 });
      break;
   }
   default:
      decaf_abort("Unexpected texture sample dim");
   }

   auto levelInfo = mSpv->createUnaryOp(spv::OpImageQueryLevels, mSpv->intType(), image);
   auto output = mSpv->createOp(spv::OpCompositeConstruct, mSpv->int4Type(), { sizeInfo, levelInfo });

   mSpv->writeGprMaskRef(destGpr, output);
}

void Transpiler::translateTex_SET_CUBEMAP_INDEX(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   // TODO: It is possible that we are supposed to somehow force
   //  a specific face to be used in spite of the coordinates.
   //decaf_abort("Unsupported SET_CUBEMAP_INDEX")
}

void Transpiler::translateTex_GET_GRADIENTS_H(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   auto textureId = inst.word0.RESOURCE_ID();
   auto samplerId = inst.word2.SAMPLER_ID();
   decaf_check(textureId == 0);
   decaf_check(samplerId == 0);

   // TODO: Make a latte decoder for reading these registers...
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

   mSpv->addCapability(spv::CapabilityDerivativeControl);
   auto output = mSpv->createUnaryOp(spv::OpDPdx, mSpv->float4Type(), srcGprVal);

   mSpv->writeGprMaskRef(destGpr, output);
}

void Transpiler::translateTex_GET_GRADIENTS_V(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   auto textureId = inst.word0.RESOURCE_ID();
   auto samplerId = inst.word2.SAMPLER_ID();
   decaf_check(textureId == 0);
   decaf_check(samplerId == 0);

   // TODO: Make a latte decoder for reading these registers...
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

   mSpv->addCapability(spv::CapabilityDerivativeControl);
   auto output = mSpv->createUnaryOp(spv::OpDPdy, mSpv->float4Type(), srcGprVal);

   mSpv->writeGprMaskRef(destGpr, output);
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
