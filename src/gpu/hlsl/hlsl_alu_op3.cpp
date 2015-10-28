#include "hlsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;

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

namespace hlsl
{

AluSource cndSrc(AluSource src)
{
   if (src.type == AluSource::ConstantInt || src.type == AluSource::ConstantLiteral) {
      return src;
   }

   src.valueType = AluSource::Float;
   return src;
}

static bool CNDE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == 0) ? src1 : src2
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins->sources[0]);
   state.out << " == ";

   if (ins->sources[0].valueType == AluSource::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, cndSrc(ins->sources[1]));
   state.out << " : ";
   translateAluSource(state, cndSrc(ins->sources[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool CNDGT(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 > 0) ? src1 : src2
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins->sources[0]);
   state.out << " > ";

   if (ins->sources[0].valueType == AluSource::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, cndSrc(ins->sources[1]));
   state.out << " : ";
   translateAluSource(state, cndSrc(ins->sources[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool CNDGE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 >= 0) ? src1 : src2
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins->sources[0]);
   state.out << " >= ";

   if (ins->sources[0].valueType == AluSource::Float) {
      state.out << "0.0f";
   } else {
      state.out << '0';
   }

   state.out << ") ? ";
   translateAluSource(state, cndSrc(ins->sources[1]));
   state.out << " : ";
   translateAluSource(state, cndSrc(ins->sources[2]));
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 * src1 + src2
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);
   state.out << " + ";
   translateAluSource(state, ins->sources[2]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MULADD_M2(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 * src1 + src2) * 2
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);
   state.out << " + ";
   translateAluSource(state, ins->sources[2]);
   state.out << ") * ";

   if (ins->sources[0].valueType == AluSource::Float) {
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
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);
   state.out << " + ";
   translateAluSource(state, ins->sources[2]);
   state.out << ") * ";

   if (ins->sources[0].valueType == AluSource::Float) {
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
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << '(';
   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);
   state.out << " + ";
   translateAluSource(state, ins->sources[2]);
   state.out << ") / ";

   if (ins->sources[0].valueType == AluSource::Float) {
      state.out << "2.0f";
   } else {
      state.out << '2';
   }

   translateAluDestEnd(state, ins);
   return true;
}

void registerAluOP3()
{
   using latte::alu::op3;
   registerGenerator(op3::CNDE, CNDE);
   registerGenerator(op3::CNDGE, CNDGE);
   registerGenerator(op3::CNDGT, CNDGT);
   registerGenerator(op3::CNDE_INT, CNDE);
   registerGenerator(op3::CNDGE_INT, CNDE);
   registerGenerator(op3::CNDGT_INT, CNDE);
   registerGenerator(op3::MULADD, MULADD);
   registerGenerator(op3::MULADD_M2, MULADD_M2);
   registerGenerator(op3::MULADD_M4, MULADD_M4);
   registerGenerator(op3::MULADD_D2, MULADD_D2);
   registerGenerator(op3::MULADD_IEEE, MULADD);
   registerGenerator(op3::MULADD_IEEE_M2, MULADD_M2);
   registerGenerator(op3::MULADD_IEEE_M4, MULADD_M4);
   registerGenerator(op3::MULADD_IEEE_D2, MULADD_D2);
}

} // namespace hlsl
