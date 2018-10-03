#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

void Transpiler::translateAluOp3_CNDE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto output = genAluCondOp(spv::Op::OpFOrdEqual, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_CNDGT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto output = genAluCondOp(spv::Op::OpFOrdGreaterThan, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_CNDGE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto output = genAluCondOp(spv::Op::OpFOrdGreaterThanEqual, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_CNDE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2, VarRefType::INT);

   auto output = genAluCondOp(spv::Op::OpIEqual, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_CNDGT_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2, VarRefType::INT);

   auto output = genAluCondOp(spv::Op::OpSGreaterThan, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_CNDGE_INT(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0, VarRefType::INT);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1, VarRefType::INT);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2, VarRefType::INT);

   auto output = genAluCondOp(spv::Op::OpSGreaterThanEqual, src0, src1, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_MULADD(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   translateAluOp3_MULADD_IEEE(cf, group, unit, inst);
}

void Transpiler::translateAluOp3_MULADD_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto muled = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), src0, src1);
   auto output = mSpv->createBinOp(spv::Op::OpFAdd, mSpv->floatType(), muled, src2);

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_MULADD_M2(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto mul = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), src0, src1);
   auto muladd = mSpv->createBinOp(spv::Op::OpFAdd, mSpv->floatType(), mul, src2);
   auto output = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), muladd, mSpv->makeFloatConstant(2.0f));

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_MULADD_M4(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto mul = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), src0, src1);
   auto muladd = mSpv->createBinOp(spv::Op::OpFAdd, mSpv->floatType(), mul, src2);
   auto output = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), muladd, mSpv->makeFloatConstant(4.0f));

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

void Transpiler::translateAluOp3_MULADD_D2(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluInstSrc(cf, group, inst, 0);
   auto src1 = mSpv->readAluInstSrc(cf, group, inst, 1);
   auto src2 = mSpv->readAluInstSrc(cf, group, inst, 2);

   auto mul = mSpv->createBinOp(spv::Op::OpFMul, mSpv->floatType(), src0, src1);
   auto muladd = mSpv->createBinOp(spv::Op::OpFAdd, mSpv->floatType(), mul, src2);
   auto output = mSpv->createBinOp(spv::Op::OpFDiv, mSpv->floatType(), muladd, mSpv->makeFloatConstant(2.0f));

   mSpv->writeAluOpDest(cf, group, unit, inst, output);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
