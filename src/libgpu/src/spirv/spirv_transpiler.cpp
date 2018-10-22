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
   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      mVsInputBuffers[i].isUsed = false;
   }

   writeCfAluUnitLine(mType, -1, -1, -1);

   ShaderParser::translate();
}

int findVsOutputLocation(const std::vector<uint32_t>& semantics, uint32_t semanticId)
{
   for (auto i = 0u; i < semantics.size(); ++i) {
      if (semantics[i] == semanticId) {
         return static_cast<int>(i);
      }
   }
   return -1;
}

void Transpiler::writeVertexProlog(ShaderSpvBuilder &spvGen, const VertexShaderDesc& desc)
{
   spvGen.createStore(spvGen.makeIntConstant(0), spvGen.stackIndexVar());
   spvGen.createStore(spvGen.stateActive(), spvGen.stateVar());

   auto vertexIdVal = spvGen.createLoad(spvGen.vertexIdVar());
   vertexIdVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), vertexIdVal);

   auto instanceIdVal = spvGen.createLoad(spvGen.instanceIdVar());
   instanceIdVal = spvGen.createUnaryOp(spv::OpBitcast, spvGen.floatType(), instanceIdVal);

   auto zeroConst = spvGen.makeFloatConstant(0.0f);
   auto initialR0 = spvGen.createOp(spv::OpCompositeConstruct, spvGen.float4Type(),
                                   { vertexIdVal, instanceIdVal, zeroConst, zeroConst });

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
   spvGen.createStore(spvGen.makeIntConstant(0), spvGen.stackIndexVar());
   spvGen.createStore(spvGen.stateActive(), spvGen.stateVar());
}

void Transpiler::writePixelProlog(ShaderSpvBuilder &spvGen, const PixelShaderDesc& desc)
{
   spvGen.createStore(spvGen.makeIntConstant(0), spvGen.stackIndexVar());
   spvGen.createStore(spvGen.stateActive(), spvGen.stateVar());

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
      auto semLocation = findVsOutputLocation(desc.vsOutputSemantics, spi_ps_input_cntl.SEMANTIC());

      if (desc.regs.spi_ps_in_control_0.POSITION_ENA() &&
          desc.regs.spi_ps_in_control_0.POSITION_ADDR() == i) {
         // TODO: Handle desc.regs.spi_ps_in_control_0.POSITION_CENTROID();
         // TODO: Handle desc.regs.spi_ps_in_control_0.POSITION_SAMPLE();
         decaf_check(semLocation < 0);
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
   state.mDesc = &shaderDesc;
   state.mBinary = shaderDesc.binary;
   state.mAluInstPreferVector = shaderDesc.aluInstPreferVector;

   if (shaderDesc.type == ShaderType::Vertex) {
      auto &vsDesc = *reinterpret_cast<const VertexShaderDesc*>(&shaderDesc);

      state.mTexInput = vsDesc.texDims;

      spvGen.setDescriptorSetIdx(0);
      Transpiler::writeVertexProlog(spvGen, vsDesc);
   } else if (shaderDesc.type == ShaderType::Geometry) {
      auto &gsDesc = *reinterpret_cast<const GeometryShaderDesc*>(&shaderDesc);

      state.mTexInput = gsDesc.texDims;

      spvGen.setDescriptorSetIdx(1);
      Transpiler::writeGeometryProlog(spvGen, gsDesc);
   } else if (shaderDesc.type == ShaderType::Pixel) {
      auto &psDesc = *reinterpret_cast<const PixelShaderDesc*>(&shaderDesc);

      state.mPixelOutType = psDesc.pixelOutType;
      state.mTexInput = psDesc.texDims;

      spvGen.setDescriptorSetIdx(2);
      Transpiler::writePixelProlog(spvGen, psDesc);
   }

   state.translate();
   spvGen.makeReturn(true);

   if (shaderDesc.type != ShaderType::Vertex) {
      if (spvGen.hasFunction("fs_main")) {
         decaf_abort("Non-vertex-shader called into a FS function, wat?");
      }
   }

   if (shaderDesc.type == ShaderType::Vertex) {
      auto& vsDesc = *reinterpret_cast<const VertexShaderDesc*>(&shaderDesc);
      auto vsShader = reinterpret_cast<VertexShader*>(shader);

      if (spvGen.hasFunction("fs_main")) {
         auto fsFunc = spvGen.getFunction("fs_main");
         spvGen.setBuildPoint(fsFunc->getEntryBlock());

         auto fsState = Transpiler {};
         fsState.mSpv = &spvGen;
         fsState.mDesc = &shaderDesc;
         fsState.mType = ShaderParser::Type::Fetch;
         fsState.mBinary = vsDesc.fsBinary;
         fsState.mAluInstPreferVector = vsDesc.aluInstPreferVector;
         fsState.translate();
         spvGen.makeReturn(true);

         // Copy the FS attribute buffer stuff over.  We check to make sure that
         // the vertex shader didn't also try to read stuff (this would be an error).
         decaf_check(state.mVsInputAttribs.size() == 0);
         state.mVsInputBuffers = fsState.mVsInputBuffers;
         state.mVsInputAttribs = fsState.mVsInputAttribs;
      }

      // For each exported parameter, we need to exports the semantics for
      // later matching up by the pixel shaders
      int numExports = spvGen.getNumParamExports();
      for (auto i = 0; i < numExports; ++i) {
         uint32_t semanticId;

         // TODO: This should probably be moved into the actual export generation
         // code instead of being calculated later and assuming the order of the
         // exports matches up with the code...
         if ((i & 3) == 0) {
            semanticId = vsDesc.regs.spi_vs_out_ids[i >> 2].SEMANTIC_0();
         } else if ((i & 3) == 1) {
            semanticId = vsDesc.regs.spi_vs_out_ids[i >> 2].SEMANTIC_1();
         } else if ((i & 3) == 2) {
            semanticId = vsDesc.regs.spi_vs_out_ids[i >> 2].SEMANTIC_2();
         } else if ((i & 3) == 3) {
            semanticId = vsDesc.regs.spi_vs_out_ids[i >> 2].SEMANTIC_3();
         }

         vsShader->outputSemantics.push_back(semanticId);
      }

      vsShader->inputBuffers = state.mVsInputBuffers;
      vsShader->inputAttribs = state.mVsInputAttribs;
   }

   for (auto i = 0u; i < latte::MaxSamplers; ++i) {
      shader->samplerUsed[i] = spvGen.isSamplerUsed(i);
   }
   for (auto i = 0u; i < latte::MaxTextures; ++i) {
      shader->textureUsed[i] = spvGen.isTextureUsed(i);
   }

   bool cbuffersUsed = false;
   for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
      auto cbufferUsed = spvGen.isUniformBufferUsed(i);
      shader->cbufferUsed[i] = cbufferUsed;
      cbuffersUsed |= cbufferUsed;
   }

   bool cfileUsed = spvGen.isConstantFileUsed();
   if (cfileUsed && cbuffersUsed) {
      decaf_abort("Shader used both constant buffers and the constants file");
   }

   shader->binary.clear();
   spvGen.dump(shader->binary);

   return true;
}

bool translate(const ShaderDesc& shaderDesc, Shader *shader)
{
   return Transpiler::translate(shaderDesc, shader);
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
