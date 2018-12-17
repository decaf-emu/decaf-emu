#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

#include <disassemble.h>
#include <regex>

namespace spirv
{

using namespace latte;

void Transpiler::writeCfAluUnitLine(ShaderParser::Type shaderType, int cfId, int groupId, int unitId)
{
   int shaderId;
   switch (shaderType) {
   case ShaderParser::Type::Fetch:
      shaderId = 1;
      break;
   case ShaderParser::Type::Vertex:
      shaderId = 2;
      break;
   case ShaderParser::Type::DataCache:
      shaderId = 3;
      break;
   case ShaderParser::Type::Geometry:
      shaderId = 4;
      break;
   case ShaderParser::Type::Pixel:
      shaderId = 5;
      break;
   default:
      decaf_abort("unexpected shader type.")
   }

   if (cfId < 0) {
      cfId = 999;
   }
   if (groupId < 0) {
      groupId = 999;
   }
   if (unitId < 0) {
      unitId = 9;
   }

   mSpv->setLine(shaderId * 10000000 + cfId * 10000 + groupId * 10 + unitId);
}

void Transpiler::translateTexInst(const ControlFlowInst &cf, const TextureFetchInst &inst)
{
   writeCfAluUnitLine(mType, mCfPC, mTexVtxPC, -1);

   ShaderParser::translateTexInst(cf, inst);
}

void Transpiler::translateVtxInst(const ControlFlowInst &cf, const VertexFetchInst &inst)
{
   writeCfAluUnitLine(mType, mCfPC, mTexVtxPC, -1);

   ShaderParser::translateVtxInst(cf, inst);
}

void Transpiler::translateAluInst(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   writeCfAluUnitLine(mType, mCfPC, mGroupPC, static_cast<uint32_t>(unit));

   ShaderParser::translateAluInst(cf, group, unit, inst);
}

void Transpiler::translateAluGroup(const ControlFlowInst &cf, const AluInstructionGroup &group)
{
   writeCfAluUnitLine(mType, mCfPC, mGroupPC, -1);

   ShaderParser::translateAluGroup(cf, group);

   mSpv->flushAluGroupWrites();
   mSpv->swapPrevRes();
}

void Transpiler::translateCfNormalInst(const ControlFlowInst& cf)
{
   // cf.word1.BARRIER(); - No need to handle coherency in SPIRV
   // cf.word1.WHOLE_QUAD_MODE(); - No need to handle optimization
   // cf.word1.VALID_PIXEL_MODE(); - No need to handle optimization

   writeCfAluUnitLine(mType, mCfPC, -1, -1);

   ShaderParser::translateCfNormalInst(cf);
}
void Transpiler::translateCfExportInst(const ControlFlowInst& cf)
{
   // cf.word1.BARRIER(); - No need to handle coherency in SPIRV
   // cf.word1.WHOLE_QUAD_MODE(); - No need to handle optimization
   // cf.word1.VALID_PIXEL_MODE(); - No need to handle optimization

   writeCfAluUnitLine(mType, mCfPC, -1, -1);

   ShaderParser::translateCfExportInst(cf);
}
void Transpiler::translateCfAluInst(const ControlFlowInst& cf)
{
   // cf.word1.BARRIER(); - No need to handle coherency in SPIRV
   // cf.word1.WHOLE_QUAD_MODE(); - No need to handle optimization

   writeCfAluUnitLine(mType, mCfPC, -1, -1);

   ShaderParser::translateCfAluInst(cf);

   mSpv->resetAr();
}

void Transpiler::translate()
{
   ShaderParser::reset();

   writeCfAluUnitLine(mType, -1, -1, -1);

   ShaderParser::translate();
}

int findVsOutputLocation(const std::array<uint8_t, 40>& semantics, uint32_t semanticId)
{
   for (auto i = 0u; i < semantics.size(); ++i) {
      if (semantics[i] == semanticId) {
         return static_cast<int>(i);
      }
   }
   return -1;
}

void Transpiler::writeGenericProlog(ShaderSpvBuilder &spvGen)
{
   spvGen.createStore(spvGen.makeIntConstant(0), spvGen.stackIndexVar());
   spvGen.createStore(spvGen.stateActive(), spvGen.stateVar());
}

void Transpiler::writeVertexProlog(ShaderSpvBuilder &spvGen, const VertexShaderDesc& desc)
{
   // This is implied as being enabled if we see a write to POS_1
   //desc.regs.pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA();

   // I don't quite understand the semantics of this.
   //desc.regs.pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA();

   // We do not currently support these cases:
   decaf_check(!desc.regs.pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
   decaf_check(!desc.regs.pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());

   writeGenericProlog(spvGen);

   auto oneConst = spvGen.makeIntConstant(1);

   // Note that because we use VertexIndex, we have to subtrack the base value
   // away from gl_VertexIndex to receive the same value we received in GL.
   auto zSpaceMulPtr = spvGen.createAccessChain(spv::StorageClass::StorageClassPushConstant, spvGen.vsPushConstVar(), { oneConst });
   auto zSpaceMulVal = spvGen.createLoad(zSpaceMulPtr);
   auto vertexBaseFlt = spvGen.createOp(spv::OpCompositeExtract, spvGen.floatType(), { zSpaceMulVal, 2 });
   auto vertexBaseVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.intType(), vertexBaseFlt);
   auto instanceBaseFlt = spvGen.createOp(spv::OpCompositeExtract, spvGen.floatType(), { zSpaceMulVal, 3 });
   auto instanceBaseVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.intType(), instanceBaseFlt);

   auto vertexIdVal = spvGen.createLoad(spvGen.vertexIdVar());
   vertexIdVal = spvGen.createBinOp(spv::OpISub, spvGen.intType(), vertexIdVal, vertexBaseVal);
   vertexIdVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), vertexIdVal);

   auto instanceIdVal = spvGen.createLoad(spvGen.instanceIdVar());
   instanceIdVal = spvGen.createBinOp(spv::OpISub, spvGen.intType(), instanceIdVal, instanceBaseVal);
   instanceIdVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), instanceIdVal);

   auto zeroFConst = spvGen.makeFloatConstant(0.0f);
   auto initialR0 = spvGen.createOp(spv::OpCompositeConstruct, spvGen.float4Type(),
                                   { vertexIdVal, instanceIdVal, zeroFConst, zeroFConst });

   GprMaskRef gpr0;
   gpr0.gpr.number = 0;
   gpr0.gpr.indexMode = GprIndexMode::None;
   gpr0.mask[0] = latte::SQ_SEL::SEL_X;
   gpr0.mask[1] = latte::SQ_SEL::SEL_Y;
   gpr0.mask[2] = latte::SQ_SEL::SEL_Z;
   gpr0.mask[3] = latte::SQ_SEL::SEL_W;
   spvGen.writeGprMaskRef(gpr0, initialR0);
}

void Transpiler::writeGeometryProlog(ShaderSpvBuilder &spvGen, const GeometryShaderDesc& desc)
{
   // This is implied as being enabled if we see a write to POS_1
   //desc.regs.pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA();

   // I don't quite understand the semantics of this.
   //desc.regs.pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA();

   // We do not currently support these cases:
   decaf_check(!desc.regs.pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
   decaf_check(!desc.regs.pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());

   spvGen.addCapability(spv::CapabilityGeometry);

   auto mainFn = spvGen.getFunction("main");
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeInvocations, 1);
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputVertices, 64);

   spvGen.addExecutionMode(mainFn, spv::ExecutionModeTriangles);

   switch (desc.regs.vgt_gs_out_prim_type) {
   case latte::VGT_GS_OUT_PRIMITIVE_TYPE::POINTLIST:
      spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputPoints);
      break;
   case latte::VGT_GS_OUT_PRIMITIVE_TYPE::LINESTRIP:
      spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputLineStrip);
      break;
   case latte::VGT_GS_OUT_PRIMITIVE_TYPE::TRISTRIP:
      spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputTriangleStrip);
      break;
   default:
      decaf_abort("Unexpected geometry shader primitive type");
   }

   writeGenericProlog(spvGen);

   // Initialize the ring index to 0
   spvGen.createStore(spvGen.makeUintConstant(0), spvGen.ringOffsetVar());

   auto zeroFConst = spvGen.makeFloatConstant(0.0f);
   auto zeroConstAsF = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), spvGen.makeUintConstant(0));
   auto oneConstAsF = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), spvGen.makeUintConstant(1));
   auto twoConstAsF = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), spvGen.makeUintConstant(2));
   auto initialR0 = spvGen.createOp(spv::OpCompositeConstruct, spvGen.float4Type(),
                                    { zeroConstAsF, oneConstAsF, zeroFConst, twoConstAsF });

   GprMaskRef gpr0;
   gpr0.gpr.number = 0;
   gpr0.gpr.indexMode = GprIndexMode::None;
   gpr0.mask[0] = latte::SQ_SEL::SEL_X;
   gpr0.mask[1] = latte::SQ_SEL::SEL_Y;
   gpr0.mask[2] = latte::SQ_SEL::SEL_Z;
   gpr0.mask[3] = latte::SQ_SEL::SEL_W;
   spvGen.writeGprMaskRef(gpr0, initialR0);
}

void Transpiler::writePixelProlog(ShaderSpvBuilder &spvGen, const PixelShaderDesc& desc)
{
   writeGenericProlog(spvGen);

   auto mainFn = spvGen.getFunction("main");
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeOriginUpperLeft);

   // We don't currently support baryocentric sampling
   decaf_check(!desc.regs.spi_ps_in_control_0.BARYC_AT_SAMPLE_ENA());

   // These simply control feature enablement for the interpolation
   //  of the PS inputs down below:
   // psDesc.regs.spi_ps_in_control_0.BARYC_SAMPLE_CNTL()
   // psDesc.regs.spi_ps_in_control_0.LINEAR_GRADIENT_ENA()
   // psDesc.regs.spi_ps_in_control_0.PERSP_GRADIENT_ENA()

   // We do not currently support fixed point positions
   decaf_check(!desc.regs.spi_ps_in_control_1.FIXED_PT_POSITION_ENA());
   //psDesc.regs.spi_ps_in_control_1.FIXED_PT_POSITION_ADDR();

   // I don't even know how to handle this being off!?
   //psDesc.regs.spi_ps_in_control_1.FOG_ADDR();

   // We do not currently support pixel indexing
   decaf_check(!desc.regs.spi_ps_in_control_1.GEN_INDEX_PIX());
   //psDesc.regs.spi_ps_in_control_1.GEN_INDEX_PIX_ADDR();

   // I don't believe this is used on our GPU...
   //psDesc.regs.spi_ps_in_control_1.POSITION_ULC();

   std::map<uint32_t, latte::SPI_PS_INPUT_CNTL_N*> inputCntlMap;

   auto numInputs = desc.regs.spi_ps_in_control_0.NUM_INTERP();
   for (auto i = 0u; i < numInputs; ++i) {
      auto &spi_ps_input_cntl = desc.regs.spi_ps_input_cntls[i];
      auto gprRef = spvGen.getGprRef({ i, latte::GprIndexMode::None });
      auto inputSemantic = spi_ps_input_cntl.SEMANTIC();

      // Locate the vertex shader output that matches
      int semLocation = -1;
      for (auto i = 0; i < 10; ++i) {
         if (inputSemantic == desc.regs.spi_vs_out_ids[i].SEMANTIC_0()) {
            semLocation = i * 4 + 0;
            break;
         } else if (inputSemantic == desc.regs.spi_vs_out_ids[i].SEMANTIC_1()) {
            semLocation = i * 4 + 1;
            break;
         } else if (inputSemantic == desc.regs.spi_vs_out_ids[i].SEMANTIC_2()) {
            semLocation = i * 4 + 2;
            break;
         } else if (inputSemantic == desc.regs.spi_vs_out_ids[i].SEMANTIC_3()) {
            semLocation = i * 4 + 3;
            break;
         }
      }

      if (desc.regs.spi_ps_in_control_0.POSITION_ENA() &&
          desc.regs.spi_ps_in_control_0.POSITION_ADDR() == i) {
         // TODO: Handle desc.regs.spi_ps_in_control_0.POSITION_CENTROID();
         // TODO: Handle desc.regs.spi_ps_in_control_0.POSITION_SAMPLE();
         spvGen.createStore(spvGen.createLoad(spvGen.fragCoordVar()), gprRef);
         continue;
      }

      if (semLocation < 0) {
         // There was no matching semantic output from the VS...
         auto zeroConst = spvGen.makeFloatConstant(0.0f);
         auto oneConst = spvGen.makeFloatConstant(1.0f);

         spv::Id defaultVal;
         switch (spi_ps_input_cntl.DEFAULT_VAL()) {
         case 0:
            defaultVal = spvGen.makeCompositeConstant(spvGen.float4Type(),
                                                      { zeroConst, zeroConst, zeroConst, zeroConst });
            break;
         case 1:
            defaultVal = spvGen.makeCompositeConstant(spvGen.float4Type(),
                                                      { zeroConst, zeroConst, zeroConst, oneConst });
            break;
         case 2:
            defaultVal = spvGen.makeCompositeConstant(spvGen.float4Type(),
                                                      { oneConst, oneConst, oneConst, zeroConst });
            break;
         case 3:
            defaultVal = spvGen.makeCompositeConstant(spvGen.float4Type(),
                                                      { oneConst, oneConst, oneConst, oneConst });
            break;
         default:
            decaf_abort("Unexpected PS input semantic default");
         }

         spvGen.createStore(defaultVal, gprRef);

         continue;
      }

      auto inputVar = spvGen.inputParamVar(static_cast<uint32_t>(semLocation));

      decaf_check(!spi_ps_input_cntl.CYL_WRAP());
      decaf_check(!spi_ps_input_cntl.PT_SPRITE_TEX());

      if (spi_ps_input_cntl.FLAT_SHADE()) {
         decaf_check(!spi_ps_input_cntl.SEL_LINEAR());
         decaf_check(!spi_ps_input_cntl.SEL_SAMPLE());

         // Using centroid qualifier with flat shading doesn't make sense
         decaf_check(!spi_ps_input_cntl.SEL_CENTROID());

         spvGen.addDecoration(inputVar, spv::DecorationFlat);
      } else if (spi_ps_input_cntl.SEL_LINEAR()) {
         decaf_check(!spi_ps_input_cntl.FLAT_SHADE());
         decaf_check(!spi_ps_input_cntl.SEL_SAMPLE());

         spvGen.addDecoration(inputVar, spv::DecorationNoPerspective);

         if (spi_ps_input_cntl.SEL_CENTROID()) {
            spvGen.addDecoration(inputVar, spv::DecorationCentroid);
         }
      } else if (spi_ps_input_cntl.SEL_SAMPLE()) {
         decaf_check(!spi_ps_input_cntl.FLAT_SHADE());
         decaf_check(!spi_ps_input_cntl.SEL_LINEAR());

         // Using centroid qualifier with sample shading doesn't make sense
         decaf_check(!spi_ps_input_cntl.SEL_CENTROID());

         spvGen.addDecoration(inputVar, spv::DecorationSample);
      } else {
         // The default will be fine...
         if (spi_ps_input_cntl.SEL_CENTROID()) {
            spvGen.addDecoration(inputVar, spv::DecorationCentroid);
         }
      }

      auto inputVal = spvGen.createLoad(inputVar);
      spvGen.createStore(inputVal, gprRef);
   }

   if (desc.regs.spi_ps_in_control_1.FRONT_FACE_ENA()) {
      auto ffGprIdx = desc.regs.spi_ps_in_control_1.FRONT_FACE_ADDR();
      auto ffChanIdx = desc.regs.spi_ps_in_control_1.FRONT_FACE_CHAN();

      latte::GprChanRef ffRef;
      ffRef.gpr = latte::makeGprRef(ffGprIdx);
      ffRef.chan = static_cast<SQ_CHAN>(ffChanIdx);

      auto frontFacingVar = spvGen.frontFacingVar();
      auto frontFacingVal = spvGen.createLoad(frontFacingVar);

      spv::Id output = spv::NoResult;

      auto ffBitMode = desc.regs.spi_ps_in_control_1.FRONT_FACE_ALL_BITS();
      if (ffBitMode == 0) {
         // Sign bit represents front facing (-1.0f Back, +1.0f Front)
         auto backConst = spvGen.makeFloatConstant(-1.0f);
         auto frontConst = spvGen.makeFloatConstant(+1.0f);
         output = spvGen.createTriOp(spv::OpSelect, spvGen.floatType(), frontFacingVal, frontConst, backConst);
      } else if (ffBitMode == 1) {
         // Full value represents front facing (0 Back, 1 Front)
         auto backConst = spvGen.makeUintConstant(0);
         auto frontConst = spvGen.makeUintConstant(1);
         output = spvGen.createTriOp(spv::OpSelect, spvGen.uintType(), frontFacingVal, frontConst, backConst);
      } else {
         decaf_abort("Unexpected front face bit mode.");
      }

      spvGen.writeGprChanRef(ffRef, output);
   }
}

bool Transpiler::translate(const ShaderDesc& shaderDesc, Shader *shader)
{
   auto state = Transpiler {};

   if (shaderDesc.type == ShaderType::Vertex) {
      state.mType = Transpiler::Type::Vertex;
   } else if (shaderDesc.type == ShaderType::Geometry) {
      state.mType = Transpiler::Type::Geometry;
   } else if (shaderDesc.type == ShaderType::Pixel) {
      state.mType = Transpiler::Type::Pixel;
   } else {
      decaf_abort("Unexpected shader type");
   }

   spv::ExecutionModel spvExecModel;
   if (shaderDesc.type == ShaderType::Vertex) {
      spvExecModel = spv::ExecutionModel::ExecutionModelVertex;
   } else if (shaderDesc.type == ShaderType::Geometry) {
      spvExecModel = spv::ExecutionModel::ExecutionModelGeometry;
   } else if (shaderDesc.type == ShaderType::Pixel) {
      spvExecModel = spv::ExecutionModel::ExecutionModelFragment;
   } else {
      decaf_abort("Unexpected shader type");
   }

   auto spvGen = ShaderSpvBuilder(spvExecModel);
   spvGen.setSourceFile("none");

   state.mSpv = &spvGen;
   state.mBinary = shaderDesc.binary;
   state.mAluInstPreferVector = shaderDesc.aluInstPreferVector;

   state.mTexDims = shaderDesc.texDims;
   state.mTexFormats = shaderDesc.texFormat;

   if (shaderDesc.type == ShaderType::Vertex) {
      auto &vsDesc = *reinterpret_cast<const VertexShaderDesc*>(&shaderDesc);

      spvGen.setBindingBase(32 * 0);

      state.mSqVtxSemantics = vsDesc.regs.sq_vtx_semantics;
      state.mPaClVsOutCntl = vsDesc.regs.pa_cl_vs_out_cntl;
      state.mStreamOutStride = vsDesc.streamOutStride;

      Transpiler::writeVertexProlog(spvGen, vsDesc);
   } else if (shaderDesc.type == ShaderType::Geometry) {
      auto &gsDesc = *reinterpret_cast<const GeometryShaderDesc*>(&shaderDesc);

      spvGen.setBindingBase(32 * 1);

      state.mPaClVsOutCntl = gsDesc.regs.pa_cl_vs_out_cntl;
      state.mStreamOutStride = gsDesc.streamOutStride;

      Transpiler::writeGeometryProlog(spvGen, gsDesc);
   } else if (shaderDesc.type == ShaderType::Pixel) {
      auto &psDesc = *reinterpret_cast<const PixelShaderDesc*>(&shaderDesc);

      spvGen.setBindingBase(32 * 2);

      state.mPixelOutType = psDesc.pixelOutType;
      state.mSqPgmExportsPs = psDesc.regs.sq_pgm_exports_ps;
      state.mCbShaderControl = psDesc.regs.cb_shader_control;
      state.mCbShaderMask = psDesc.regs.cb_shader_mask;
      state.mDbShaderControl = psDesc.regs.db_shader_control;

      Transpiler::writePixelProlog(spvGen, psDesc);
   }

   state.translate();
   spvGen.makeReturn(true);

   if (shaderDesc.type != ShaderType::Vertex) {
      if (spvGen.hasFunction("fs_main")) {
         decaf_abort("Non-vertex-shader called into a FS function, wat?");
      }
   }

   if (shaderDesc.type != ShaderType::Geometry) {
      if (spvGen.hasFunction("dc_main")) {
         decaf_abort("Non-vertex-shader called into a DC function, wat?");
      }
   }

   ShaderMeta genericMeta;

   for (auto i = 0u; i < latte::MaxSamplers; ++i) {
      genericMeta.samplerUsed[i] = spvGen.isSamplerUsed(i);
   }
   for (auto i = 0u; i < latte::MaxTextures; ++i) {
      genericMeta.textureUsed[i] = spvGen.isTextureUsed(i);
   }

   genericMeta.cfileUsed = spvGen.isConstantFileUsed();

   for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
      auto cbufferUsed = spvGen.isUniformBufferUsed(i);
      genericMeta.cbufferUsed[i] = cbufferUsed;
      if (genericMeta.cfileUsed && cbufferUsed) {
         decaf_abort("Shader used both constant buffers and the constants file");
      }
   }

   if (shaderDesc.type == ShaderType::Vertex) {
      auto& vsDesc = *reinterpret_cast<const VertexShaderDesc*>(&shaderDesc);
      auto vsShader = reinterpret_cast<VertexShader*>(shader);

      if (spvGen.hasFunction("fs_main")) {
         auto fsFunc = spvGen.getFunction("fs_main");
         spvGen.setBuildPoint(fsFunc->getEntryBlock());

         state.mType = ShaderParser::Type::Fetch;
         state.mBinary = vsDesc.fsBinary;
         state.translate();

         // Write the return statement at the end of the function
         spvGen.makeReturn(true);
      }

      static_cast<ShaderMeta&>(vsShader->meta) = genericMeta;
      vsShader->meta.numExports = spvGen.getNumParamExports();
      vsShader->meta.attribBuffers = state.mAttribBuffers;
      vsShader->meta.attribElems = state.mAttribElems;

      for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
         vsShader->meta.streamOutUsed[i] = spvGen.isStreamOutUsed(i);
      }
   } else if (shaderDesc.type == ShaderType::Geometry) {
      auto& gsDesc = *reinterpret_cast<const GeometryShaderDesc*>(&shaderDesc);
      auto gsShader = reinterpret_cast<GeometryShader*>(shader);

      if (spvGen.hasFunction("dc_main")) {
         auto dcFunc = spvGen.getFunction("dc_main");
         spvGen.setBuildPoint(dcFunc->getEntryBlock());

         // Need to save our GPRs first
         auto gprType = spvGen.makeArrayType(spvGen.float4Type(), spvGen.makeUintConstant(128), 0);
         auto gprSaveVar = spvGen.createVariable(spv::StorageClass::StorageClassPrivate, gprType, "RVarSave");
         spvGen.createNoResultOp(spv::OpCopyMemory, { gprSaveVar, spvGen.gprVar() });

         // Translate the shader
         state.mType = ShaderParser::Type::DataCache;
         state.mBinary = gsDesc.dcBinary;
         state.translate();

         // We need to increment the ring offset at the end of each.  Note that we only
         // support striding in vec4 intervals.
         auto gsDesc = reinterpret_cast<const GeometryShaderDesc*>(&shaderDesc);
         auto ringItemStride = gsDesc->regs.sq_gs_vert_itemsize.ITEMSIZE();
         decaf_check(ringItemStride % 4 == 0);
         auto ringStride = ringItemStride / 4;
         auto ringStrideConst = spvGen.makeIntConstant(ringStride);
         auto ringOffsetVal = spvGen.createLoad(spvGen.ringOffsetVar());
         auto newRingOffset = spvGen.createBinOp(spv::OpIAdd, spvGen.uintType(), ringOffsetVal, ringStrideConst);
         spvGen.createStore(newRingOffset, spvGen.ringOffsetVar());

         // Restore our GPRs
         spvGen.createNoResultOp(spv::OpCopyMemory, { spvGen.gprVar(), gprSaveVar });

         // Write the return statement at the end of the function
         spvGen.makeReturn(true);
      }

      static_cast<ShaderMeta&>(gsShader->meta) = genericMeta;

      for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
         gsShader->meta.streamOutUsed[i] = spvGen.isStreamOutUsed(i);
      }
   } else if (shaderDesc.type == ShaderType::Pixel) {
      auto psShader = reinterpret_cast<PixelShader*>(shader);

      static_cast<ShaderMeta&>(psShader->meta) = genericMeta;

      for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
         psShader->meta.pixelOutUsed[i] = spvGen.isPixelOutUsed(i);
      }
   }

   shader->binary.clear();
   spvGen.dump(shader->binary);

   return true;
}

bool translate(const ShaderDesc& shaderDesc, Shader *shader)
{
   return Transpiler::translate(shaderDesc, shader);
}

RectStubShaderDesc
generateRectSubShaderDesc(VertexShader *vertexShader)
{
   RectStubShaderDesc desc;
   desc.numVsExports = vertexShader->meta.numExports;
   return desc;
}

bool generateRectStub(const RectStubShaderDesc& shaderDesc, RectStubShader *shader)
{
   SpvBuilder spvGen;

   spvGen.setMemoryModel(spv::AddressingModel::AddressingModelLogical, spv::MemoryModel::MemoryModelGLSL450);
   spvGen.addCapability(spv::CapabilityShader);
   spvGen.addCapability(spv::CapabilityGeometry);

   auto mainFn = spvGen.makeEntryPoint("main");
   auto entry = spvGen.addEntryPoint(spv::ExecutionModelGeometry, mainFn, "main");

   spvGen.addExecutionMode(mainFn, spv::ExecutionModeInputLinesAdjacency);
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeInvocations, 1);
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputTriangleStrip);
   spvGen.addExecutionMode(mainFn, spv::ExecutionModeOutputVertices, 6);

   auto zeroConst = spvGen.makeIntConstant(0);
   auto oneConst = spvGen.makeIntConstant(1);
   auto twoConst = spvGen.makeIntConstant(2);
   auto threeConst = spvGen.makeIntConstant(3);
   auto fourConst = spvGen.makeIntConstant(4);

   auto glInType = spvGen.makeStructType({ spvGen.float4Type() }, "gl_in");
   spvGen.addDecoration(glInType, spv::DecorationBlock);
   spvGen.addMemberDecoration(glInType, 0, spv::DecorationBuiltIn, spv::BuiltInPosition);
   spvGen.addMemberName(glInType, 0, "gl_Position");
   auto glInArrType = spvGen.makeArrayType(glInType, fourConst, 0);
   auto glInArrVar = spvGen.createVariable(spv::StorageClassInput, glInArrType, "gl_in");
   entry->addIdOperand(glInArrVar);

   auto perVertexType = spvGen.makeStructType({ spvGen.float4Type() }, "gl_PerVertex");
   spvGen.addDecoration(perVertexType, spv::DecorationBlock);
   spvGen.addMemberDecoration(perVertexType, 0, spv::DecorationBuiltIn, spv::BuiltInPosition);
   spvGen.addMemberName(perVertexType, 0, "gl_Position");
   auto perVertexVar = spvGen.createVariable(spv::StorageClassOutput, perVertexType, "gl_PerVertex");
   entry->addIdOperand(perVertexVar);

   std::vector<spv::Id> inputParams;
   std::vector<spv::Id> outputParams;

   for (auto i = 0u; i < shaderDesc.numVsExports; ++i) {
      auto paramType = spvGen.float4Type();
      auto paramInVarType = spvGen.makeArrayType(paramType, fourConst, 0);
      auto paramInVar = spvGen.createVariable(spv::StorageClassInput, paramInVarType, fmt::format("PARAM_{}_IN", i).c_str());
      spvGen.addDecoration(paramInVar, spv::DecorationLocation, i);
      entry->addIdOperand(paramInVar);
      inputParams.push_back(paramInVar);

      auto paramOutVar = spvGen.createVariable(spv::StorageClassOutput, paramType, fmt::format("PARAM_{}_OUT", i).c_str());
      spvGen.addDecoration(paramOutVar, spv::DecorationLocation, i);
      entry->addIdOperand(paramOutVar);
      outputParams.push_back(paramOutVar);
   }

   auto emitVertex = [&](int vertexId){
      auto vtxIdConst = spvGen.makeIntConstant(vertexId);

      auto posIn0Ptr = spvGen.createAccessChain(spv::StorageClassInput, glInArrVar, { vtxIdConst, zeroConst });
      auto posOutPtr = spvGen.createAccessChain(spv::StorageClassOutput, perVertexVar, { zeroConst });
      auto posInVal = spvGen.createLoad(posIn0Ptr);
      spvGen.createStore(posInVal, posOutPtr);

      for (auto i = 0u; i < shaderDesc.numVsExports; ++i) {
         auto paramInPtr = spvGen.createAccessChain(spv::StorageClassInput, inputParams[i], { vtxIdConst });
         auto paramOutPtr = outputParams[i];
         auto paramInVal = spvGen.createLoad(paramInPtr);
         spvGen.createStore(paramInVal, paramOutPtr);
      }

      spvGen.createNoResultOp(spv::OpEmitVertex);
   };

   auto posIn0Ptr = spvGen.createAccessChain(spv::StorageClassInput, glInArrVar, { zeroConst, zeroConst });
   auto posIn0Val = spvGen.createLoad(posIn0Ptr);
   auto posIn1Ptr = spvGen.createAccessChain(spv::StorageClassInput, glInArrVar, { oneConst, zeroConst });
   auto posIn1Val = spvGen.createLoad(posIn1Ptr);
   auto posIn2Ptr = spvGen.createAccessChain(spv::StorageClassInput, glInArrVar, { twoConst, zeroConst });
   auto posIn2Val = spvGen.createLoad(posIn2Ptr);
   auto posIn3Ptr = spvGen.createAccessChain(spv::StorageClassInput, glInArrVar, { threeConst, zeroConst });
   auto posIn3Val = spvGen.createLoad(posIn3Ptr);

   auto posDiff0to3 = spvGen.createBinOp(spv::OpFSub, spvGen.float4Type(), posIn0Val, posIn3Val);
   auto posDiff1to3 = spvGen.createBinOp(spv::OpFSub, spvGen.float4Type(), posIn1Val, posIn3Val);
   auto posDiff2to3 = spvGen.createBinOp(spv::OpFSub, spvGen.float4Type(), posIn2Val, posIn3Val);

   auto posLen0to3 = spvGen.createBuiltinCall(spvGen.floatType(), spvGen.glslStd450(), GLSLstd450::GLSLstd450Length, { posDiff0to3 });
   auto posLen1to3 = spvGen.createBuiltinCall(spvGen.floatType(), spvGen.glslStd450(), GLSLstd450::GLSLstd450Length, { posDiff1to3 });
   auto posLen2to3 = spvGen.createBuiltinCall(spvGen.floatType(), spvGen.glslStd450(), GLSLstd450::GLSLstd450Length, { posDiff2to3 });

   auto len0gt1 = spvGen.createBinOp(spv::OpFOrdGreaterThan, spvGen.boolType(), posLen0to3, posLen1to3);
   auto len0gt2 = spvGen.createBinOp(spv::OpFOrdGreaterThan, spvGen.boolType(), posLen0to3, posLen2to3);
   auto len1gt2 = spvGen.createBinOp(spv::OpFOrdGreaterThan, spvGen.boolType(), posLen1to3, posLen2to3);

   emitVertex(0);
   emitVertex(1);
   emitVertex(2);

   spvGen.createNoResultOp(spv::OpEndPrimitive);

   /*
   if (len0 > len1) {
      if (len0 > len2) {
         // len 0 biggest
      } else {
         // len 2 biggest
      }
   } else {
      if (len1 > len2) {
         // len 1 biggest
      } else {
         // len 2 biggest
      }
   }
   */

   auto block = spv::Builder::If { len0gt1, spv::SelectionControlMaskNone, spvGen };
   {
      auto block = spv::Builder::If { len0gt2, spv::SelectionControlMaskNone, spvGen };
      {
         // pos 0 is the hypotenuse
         emitVertex(1);
         emitVertex(2);
         emitVertex(3);
      }
      block.makeBeginElse();
      {
         // pos 2 is the hypotenuse
         emitVertex(0);
         emitVertex(1);
         emitVertex(3);
      }
      block.makeEndIf();
   }
   block.makeBeginElse();
   {
      auto block = spv::Builder::If { len1gt2, spv::SelectionControlMaskNone, spvGen };
      {
         // len 1 is the hypotenuse
         emitVertex(0);
         emitVertex(2);
         emitVertex(3);
      }
      block.makeBeginElse();
      {
         // len 2 is the hypotenuse
         emitVertex(0);
         emitVertex(1);
         emitVertex(3);
      }
      block.makeEndIf();
   }
   block.makeEndIf();

   spvGen.createNoResultOp(spv::OpEndPrimitive);

   spvGen.makeReturn(true);

   shader->binary.clear();
   spvGen.dump(shader->binary);

   return true;
}

std::string
shaderToString(const Shader *shader)
{
   std::ostringstream outputAssemblyStream;
   spv::Disassemble(outputAssemblyStream, shader->binary);

   auto outputAssembly = outputAssemblyStream.str();

   // To improve readability, translate the OpLine instructions to something readable...
   // This code assumes that all OpLine lines are in the format of `c0aaa0uu`
   // where c is the CF unit, a is the ALU and u is the unit.  Note that c can be longer
   // than a single character.
   outputAssembly = std::regex_replace(outputAssembly,
                                       std::regex("(Line [^ ]+ (\\d{1})(\\d{3})(\\d{3})((\\d{1})) 0)"),
                                       "// %IMATYPE$2%; CF: %IMACF$3%; GROUP: %IMAGROUP$4%; UNIT: %IMAUNIT$5%;");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMATYPE1%"), "FS"); // Convert shader type to text
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMATYPE2%"), "VS");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMATYPE3%"), "DC");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMATYPE4%"), "GS");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMATYPE5%"), "PS");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("CF: %IMACF999%;"), "");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMACF(0*)(\\d+)%"), "$2"); // Drop trailing 0's
   outputAssembly = std::regex_replace(outputAssembly, std::regex("GROUP: %IMAGROUP999%;"), "");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAGROUP(0*)(\\d+)%"), "$2"); // Drop trailing 0's
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAUNIT0%"), "X"); // Convert unit numbers to text
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAUNIT1%"), "Y");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAUNIT2%"), "Z");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAUNIT3%"), "W");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("%IMAUNIT4%"), "T");
   outputAssembly = std::regex_replace(outputAssembly, std::regex("UNIT: %IMAUNIT9%;"), "");

   return outputAssembly;
}

} // namespace hlsl2

#endif // ifdef DECAF_VULKAN
