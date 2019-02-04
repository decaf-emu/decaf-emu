#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

/*

Notes:
1. KILL instructions only are used to discard a pixel in the pixel
   shader according to the ISA.  However, based on empirical evidence
   in some Geometry shaders, its also used to cancel the shader when
   the ringbuffers overflow.

*/

void Transpiler::translateAluOp2_ADD(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBinOp(spv::OpFAdd, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_ADD_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::OpIAdd, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_AND_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::OpBitwiseAnd, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_ASHR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::UINT);

   auto output = mSpv->createBinOp(spv::OpShiftRightArithmetic, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_COS(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   // dst = cos(src0 / 0.1591549367)
   auto halfPiFConst = mSpv->makeFloatConstant(0.1591549367f);
   auto srcDived = mSpv->createBinOp(spv::OpFDiv, mSpv->floatType(), src0, halfPiFConst);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Cos, { srcDived });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_EXP_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Exp, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void
Transpiler::translateAluOp2_FLOOR(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::FLOAT);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Floor, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_FLT_TO_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::FLOAT);

   auto output = mSpv->createUnaryOp(spv::OpConvertFToS, mSpv->intType(), src0);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_FLT_TO_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::FLOAT);

   auto output = mSpv->createUnaryOp(spv::OpConvertFToU, mSpv->uintType(), src0);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_FRACT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Fract, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_INT_TO_FLT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);

   auto output = mSpv->createUnaryOp(spv::OpConvertSToF, mSpv->floatType(), src0);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_KILLE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto pred = mSpv->createBinOp(spv::Op::OpFOrdEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto pred = mSpv->createBinOp(spv::Op::OpIEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto pred = mSpv->createBinOp(spv::Op::OpFOrdNotEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto pred = mSpv->createBinOp(spv::Op::OpINotEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto pred = mSpv->createBinOp(spv::Op::OpFOrdGreaterThan, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto pred = mSpv->createBinOp(spv::Op::OpSGreaterThan, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto pred = mSpv->createBinOp(spv::Op::OpUGreaterThan, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto pred = mSpv->createBinOp(spv::Op::OpFOrdGreaterThanEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto pred = mSpv->createBinOp(spv::Op::OpSGreaterThanEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_KILLGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto pred = mSpv->createBinOp(spv::Op::OpUGreaterThanEqual, mSpv->boolType(), src0, src1);
   auto cond = spv::Builder::If { pred, spv::SelectionControlMaskNone, *mSpv };

   if (mType == ShaderParser::Type::Pixel) {
      mSpv->makeDiscard();
   } else {
      mSpv->makeReturn(false);
   }

   cond.makeEndIf();
}

void Transpiler::translateAluOp2_LOG_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: Implement log clamp-to-maxval
   translateAluOp2_LOG_IEEE(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_LOG_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::FLOAT);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Log, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_LSHL_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::UINT);

   auto output = mSpv->createBinOp(spv::OpShiftLeftLogical, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_LSHR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::UINT);

   auto output = mSpv->createBinOp(spv::OpShiftRightLogical, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MAX(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FMax, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MAX_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: MAX_DX10 has different behaviour to GLSL MAX
   // I believe for dx10 max returns non-NaN value: max(n, NaN) = n max(NaN, n) = n
   // Whereas glsl returns second parameter: max(n, NaN) = NaN, max(NaN, n) = n
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FMax, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MAX_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = mSpv->createBuiltinCall(mSpv->intType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450SMax, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MAX_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = mSpv->createBuiltinCall(mSpv->uintType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450UMax, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MIN(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FMin, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MIN_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: MIN_DX10 has different behaviour to GLSL MIN
   // I believe for dx10 min returns non-NaN value: min(n, NaN) = n min(NaN, n) = n
   // Whereas glsl returns second parameter: min(n, NaN) = NaN, min(NaN, n) = n
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FMin, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MIN_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = mSpv->createBuiltinCall(mSpv->intType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450SMin, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MIN_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = mSpv->createBuiltinCall(mSpv->uintType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450UMin, { src0, src1 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_MOV(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   mSpv->writeAluOpDest(cf, group, unit, inst, src0);
}

void Transpiler::translateAluOp2_MOVA_FLOOR(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto minClampConst = mSpv->makeIntConstant(-256);
   auto maxClampConst = mSpv->makeIntConstant(255);
   auto intVal = mSpv->createUnaryOp(spv::OpConvertFToS, mSpv->intType(), src0);
   auto output = mSpv->createBuiltinCall(mSpv->intType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450SClamp, { intVal, minClampConst, maxClampConst });

   mSpv->writeAluOpDest(cf, group, unit, inst, output, true);
}

void Transpiler::translateAluOp2_MOVA_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   mSpv->writeAluOpDest(cf, group, unit, inst, src0, true);
}

void Transpiler::translateAluOp2_MUL(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // Everyone gets to enjoy the IEEE standards since Vulkan is IEEE
   // TODO: Check if there are any side-effects of making MUL be IEEE compliant.
   translateAluOp2_MUL_IEEE(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_MUL_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void
Transpiler::translateAluOp2_MULLO_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::Op::OpIMul, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void
Transpiler::translateAluOp2_MULLO_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::UINT);

   auto output = mSpv->createBinOp(spv::Op::OpIMul, mSpv->uintType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_NOP(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // No need to perform any form of translation here...
}

void Transpiler::translateAluOp2_NOT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto output = mSpv->createUnaryOp(spv::OpNot, mSpv->intType(), src0);
   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_OR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::OpBitwiseOr, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdEqual, mSpv->floatType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpIEqual, mSpv->intType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::FLOAT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::FLOAT);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThanEqual, mSpv->floatType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpSGreaterThanEqual, mSpv->intType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = genPredSetOp(inst, spv::OpUGreaterThanEqual, mSpv->uintType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::FLOAT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::FLOAT);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThan, mSpv->floatType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpSGreaterThan, mSpv->intType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = genPredSetOp(inst, spv::OpUGreaterThan, mSpv->uintType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdNotEqual, mSpv->floatType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_PRED_SETNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpINotEqual, mSpv->intType(), src0, src1, true);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_RECIP_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: RECIP_CLAMPED
   latte::ShaderParser::translateAluOp2_RECIP_CLAMPED(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RECIP_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto oneFConst = mSpv->makeFloatConstant(1.0f);
   auto output = mSpv->createBinOp(spv::OpFDiv, mSpv->floatType(), oneFConst, src0);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_RECIP_FF(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_RECIP_FF(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RECIP_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_RECIP_INT(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RECIP_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_RECIP_UINT(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RECIPSQRT_CLAMPED(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_RECIPSQRT_CLAMPED(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RECIPSQRT_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450InverseSqrt, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_RECIPSQRT_FF(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: RECIPSQRT_FF
   latte::ShaderParser::translateAluOp2_RECIPSQRT_FF(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_RNDNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450RoundEven, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdEqual, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpIEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThanEqual, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThanEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpSGreaterThanEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGE_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = genPredSetOp(inst, spv::OpUGreaterThanEqual, mSpv->uintType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThan, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGT_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdGreaterThan, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpSGreaterThan, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETGT_UINT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::UINT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::UINT);

   auto output = genPredSetOp(inst, spv::OpUGreaterThan, mSpv->uintType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETNE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdNotEqual, mSpv->floatType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETNE_DX10(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);

   auto output = genPredSetOp(inst, spv::OpFOrdNotEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SETNE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);

   auto output = genPredSetOp(inst, spv::OpINotEqual, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SIN(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   // dst = sin(src0 / 0.1591549367)
   auto halfPiFConst = mSpv->makeFloatConstant(0.1591549367f);
   auto srcDived = mSpv->createBinOp(spv::OpFDiv, mSpv->floatType(), src0, halfPiFConst);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Sin, { srcDived });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SQRT_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Sqrt, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_SUB_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::OpISub, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_TRUNC(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);

   auto output = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Trunc, { src0 });

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_UINT_TO_FLT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::UINT);

   auto output = mSpv->createUnaryOp(spv::OpConvertUToF, mSpv->floatType(), src0);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp2_XOR_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, latte::VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, latte::VarRefType::INT);

   auto output = mSpv->createBinOp(spv::OpBitwiseXor, mSpv->intType(), src0, src1);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
