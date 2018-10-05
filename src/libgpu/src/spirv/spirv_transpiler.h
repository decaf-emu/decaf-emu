#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_shaderparser.h"
#include "spirv_translate.h"
#include "spirv_shaderspvbuilder.h"

namespace spirv
{

using namespace latte;

class Transpiler : public latte::ShaderParser
{
public:
   void translateTex_FETCH4(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_L(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_LB(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_G(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_L(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_LB(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_G(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_G_L(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_G_LB(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SAMPLE_C_G_LZ(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateTex_SET_CUBEMAP_INDEX(const ControlFlowInst &cf, const TextureFetchInst &inst) override;

   void translateVtx_SEMANTIC(const ControlFlowInst &cf, const VertexFetchInst &inst) override;

   void translateAluOp2_ADD(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_ADD_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_AND_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_ASHR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_COS(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_EXP_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_FLT_TO_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_FRACT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_INT_TO_FLT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_KILLGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_LOG_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_LOG_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_LSHL_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MAX(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MAX_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MAX_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MAX_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MIN(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MIN_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MIN_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MIN_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MOV(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MOVA_FLOOR(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MUL(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_MUL_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_NOP(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_NOT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_OR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_PRED_SETNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIP_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIP_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIP_FF(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIP_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIP_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIPSQRT_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIPSQRT_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RECIPSQRT_FF(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_RNDNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGT_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETNE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SETNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SIN(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SQRT_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_SUB_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_TRUNC(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_XOR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;

   void translateAluOp2_CUBE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_DOT4(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp2_DOT4_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;

   void translateAluOp3_CNDE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_CNDGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_CNDGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_CNDE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_CNDGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_CNDGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_MULADD(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_MULADD_M2(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_MULADD_M4(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluOp3_MULADD_D2(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;

   void translateTexInst(const ControlFlowInst &cf, const TextureFetchInst &inst) override;
   void translateVtxInst(const ControlFlowInst &cf, const VertexFetchInst &inst) override;
   void translateAluInst(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) override;
   void translateAluGroup(const ControlFlowInst &cf, const AluInstructionGroup &group) override;

   void translateCf_ALU(const ControlFlowInst &cf) override;
   void translateCf_ALU_PUSH_BEFORE(const ControlFlowInst &cf) override;
   void translateCf_ALU_POP_AFTER(const ControlFlowInst &cf) override;
   void translateCf_ALU_POP2_AFTER(const ControlFlowInst &cf) override;
   void translateCf_ALU_EXT(const ControlFlowInst &cf) override;
   void translateCf_ALU_CONTINUE(const ControlFlowInst &cf) override;
   void translateCf_ALU_BREAK(const ControlFlowInst &cf) override;
   void translateCf_ALU_ELSE_AFTER(const ControlFlowInst &cf) override;
   void translateCf_CALL_FS(const ControlFlowInst &cf) override;
   void translateCf_ELSE(const ControlFlowInst &cf) override;
   void translateCf_JUMP(const ControlFlowInst &cf) override;
   void translateCf_KILL(const ControlFlowInst &cf) override;
   void translateCf_NOP(const ControlFlowInst &cf) override;
   void translateCf_PUSH(const ControlFlowInst &cf) override;
   void translateCf_POP(const ControlFlowInst &cf) override;
   void translateCf_RETURN(const ControlFlowInst &cf) override;
   void translateCf_TEX(const ControlFlowInst &cf) override;
   void translateCf_VTX(const ControlFlowInst &cf) override;
   void translateCf_VTX_TC(const ControlFlowInst &cf) override;

   void translateCf_EXP(const ControlFlowInst &cf) override;
   void translateCf_EXP_DONE(const ControlFlowInst &cf) override;

   void translateCfNormalInst(const ControlFlowInst& cf) override;
   void translateCfExportInst(const ControlFlowInst& cf) override;
   void translateCfAluInst(const ControlFlowInst& cf) override;

   // Our own helpers
   spv::Id genAluCondOp(spv::Op predOp, spv::Id lhsVal, spv::Id trueVal, spv::Id falseVal);
   spv::Id genPredSetOp(const AluInst &inst, spv::Op predOp, spv::Id typeId, spv::Id lhsVal, spv::Id rhsVal, bool updatesPredicate = false);
   void translateGenericExport(const ControlFlowInst &cf);

   static void writeVertexProlog(ShaderSpvBuilder &spv, const VertexShaderDesc& desc);
   static void writeGeometryProlog(ShaderSpvBuilder &spv, const GeometryShaderDesc& desc);
   static void writePixelProlog(ShaderSpvBuilder &spv, const PixelShaderDesc& desc);

   void translate() override;
   static bool translate(const ShaderDesc& shaderDesc, Shader *shader);

protected:
   void writeCfAluUnitLine(ShaderParser::Type shaderType, int cfId, int groupId, int unitId);

   const ShaderDesc *mDesc;
   ShaderSpvBuilder *mSpv;

   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> mTexInput;
   std::array<InputBuffer, latte::MaxAttribBuffers> mVsInputBuffers;
   std::vector<InputAttrib> mVsInputAttribs;
   std::vector<uint32_t> mVsOutputParams;


};

} // namespace spirv

#endif // ifdef DECAF_VULKAN
