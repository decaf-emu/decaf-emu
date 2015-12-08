#include "glsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;
using latte::shadir::ValueType;

/*
Unimplemented:
BFE_UINT
BFE_INT
BFI_INT
FMA
MULADD_64
MULADD_64_M2
MULADD_64_M4
MULADD_64_D2
MUL_LIT
MUL_LIT_M2
MUL_LIT_M4
MUL_LIT_D2
*/

namespace gpu
{

namespace opengl
{

namespace glsl
{

AluSource cndSrc(AluSource src)
{
   if (src.sel == latte::SQ_ALU_SRC_LITERAL || src.sel == latte::SQ_ALU_SRC_1_INT || src.sel == latte::SQ_ALU_SRC_M_1_INT) {
      return src;
   }

   src.type = ValueType::Float;
   return src;
}

static bool CNDE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == 0) ? src1 : src2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " == ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, ins, cndSrc(ins->src[1]));
   state.out << " : ";
   translateAluSource(state, ins, cndSrc(ins->src[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool CNDGT(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 > 0) ? src1 : src2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " > ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, ins, cndSrc(ins->src[1]));
   state.out << " : ";
   translateAluSource(state, ins, cndSrc(ins->src[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool CNDGE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 >= 0) ? src1 : src2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " >= ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, ins, cndSrc(ins->src[1]));
   state.out << " : ";
   translateAluSource(state, ins, cndSrc(ins->src[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 * src1 + src2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " * ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " + ";
   translateAluSource(state, ins, ins->src[2]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD_M2(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 * src1 + src2) * 2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins, ins->src[0]);
   state.out << " * ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " + ";
   translateAluSource(state, ins, ins->src[2]);
   state.out << ") * ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "2.0f";
   } else {
      state.out << '2';
   }

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD_M4(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 * src1 + src2) * 4
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins, ins->src[0]);
   state.out << " * ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " + ";
   translateAluSource(state, ins, ins->src[2]);
   state.out << ") * ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "4.0f";
   } else {
      state.out << '4';
   }

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD_D2(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 * src1 + src2) / 2
   assert(ins->srcCount == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins, ins->src[0]);
   state.out << " * ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " + ";
   translateAluSource(state, ins, ins->src[2]);
   state.out << ") / ";

   if (ins->src[0].type == ValueType::Float) {
      state.out << "2.0f";
   } else {
      state.out << '2';
   }

   translateAluDestEnd(state, ins);
   return true;
}

void registerAluOP3()
{
   registerGenerator(latte::SQ_OP3_INST_CNDE, CNDE);
   registerGenerator(latte::SQ_OP3_INST_CNDGE, CNDGE);
   registerGenerator(latte::SQ_OP3_INST_CNDGT, CNDGT);
   registerGenerator(latte::SQ_OP3_INST_CNDE_INT, CNDE);
   registerGenerator(latte::SQ_OP3_INST_CNDGE_INT, CNDE);
   registerGenerator(latte::SQ_OP3_INST_CNDGT_INT, CNDE);
   registerGenerator(latte::SQ_OP3_INST_MULADD, MULADD);
   registerGenerator(latte::SQ_OP3_INST_MULADD_M2, MULADD_M2);
   registerGenerator(latte::SQ_OP3_INST_MULADD_M4, MULADD_M4);
   registerGenerator(latte::SQ_OP3_INST_MULADD_D2, MULADD_D2);
   registerGenerator(latte::SQ_OP3_INST_MULADD_IEEE, MULADD);
   registerGenerator(latte::SQ_OP3_INST_MULADD_IEEE_M2, MULADD_M2);
   registerGenerator(latte::SQ_OP3_INST_MULADD_IEEE_M4, MULADD_M4);
   registerGenerator(latte::SQ_OP3_INST_MULADD_IEEE_D2, MULADD_D2);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
