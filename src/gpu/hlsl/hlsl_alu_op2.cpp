#include "hlsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;
using latte::shadir::AluDest;

/*
Unimplemented:
FREXP_64
RNDNE
MOVA
MOVA_FLOOR
ADD_64
MOVA_INT
NOP
MUL_64
FLT64_TO_FLT32
FLT32_TO_FLT64
PRED_SET_INV
PRED_SET_POP
PRED_SET_CLR
PRED_SET_RESTORE
PRED_SETE_PUSH
PRED_SETGT_PUSH
PRED_SETGE_PUSH
PRED_SETNE_PUSH
KILLE
KILLGT
KILLGE
KILLNE
KILLGT_UINT
KILLGE_UINT
KILLE_INT
KILLGT_INT
KILLGE_INT
KILLNE_INT
PRED_SETE_PUSH_INT
PRED_SETGT_PUSH_INT
PRED_SETGE_PUSH_INT
PRED_SETNE_PUSH_INT
PRED_SETLT_PUSH_INT
PRED_SETLE_PUSH_INT
GROUP_BARRIER
GROUP_SEQ_BEGIN
GROUP_SEQ_END
SET_MODE
SET_CF_IDX0
SET_CF_IDX1
SET_LDS_SIZE
MUL_INT24
MULHI_INT24
MOVA_GPR_INT
LOG_CLAMPED
RECIP_CLAMPED
RECIP_FF
RECIPSQRT_CLAMPED
RECIPSQRT_FF
MULLO_INT
MULHI_INT
MULLO_UINT
MULHI_UINT
RECIP_INT
RECIP_UINT
FLT_TO_UINT
LDEXP_64
FRACT_64
PRED_SETGT_64
PRED_SETE_64
PRED_SETGE_64
*/

namespace hlsl
{

static bool ADD(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 + src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " + ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool SUB(GenerateState &state, AluInstruction *ins)
{
   // dst = src1 - src0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[1]);
   state.out << " - ";
   translateAluSource(state, ins->sources[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MUL(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 * src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool AND(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 & src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " & ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool OR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 | src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " | ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool XOR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 ^ src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " ^ ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool NOT(GenerateState &state, AluInstruction *ins)
{
   // dst = ~src0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << '~';
   translateAluSource(state, ins->sources[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool ASHR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 >> (src1 & 0x1f)
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " >> (";
   translateAluSource(state, ins->sources[1]);
   state.out << " & 0x1f)";

   translateAluDestEnd(state, ins);
   return true;
}

static bool LSHR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 >> (src1 & 0x1f)
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " >> (";
   translateAluSource(state, ins->sources[1]);
   state.out << " & 0x1f)";

   translateAluDestEnd(state, ins);
   return true;
}

static bool LSHL(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 << src1
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " << ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MAX(GenerateState &state, AluInstruction *ins)
{
   // dst = max(src0, src1)
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "max(";
   translateAluSource(state, ins->sources[0]);
   state.out << ", ";
   translateAluSource(state, ins->sources[1]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MIN(GenerateState &state, AluInstruction *ins)
{
   // dst = min(src0, src1)
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "min(";
   translateAluSource(state, ins->sources[0]);
   state.out << ", ";
   translateAluSource(state, ins->sources[1]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MOV(GenerateState &state, AluInstruction *ins)
{
   // dst = src0
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool CEIL(GenerateState &state, AluInstruction *ins)
{
   // dst = ceil(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "ceil(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FLOOR(GenerateState &state, AluInstruction *ins)
{
   // dst = floor(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "floor(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FRACT(GenerateState &state, AluInstruction *ins)
{
   // dst = frac(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "frac(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool TRUNC(GenerateState &state, AluInstruction *ins)
{
   // dst = trunc(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "trunc(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool COS(GenerateState &state, AluInstruction *ins)
{
   // dst = cos(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "cos(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool SIN(GenerateState &state, AluInstruction *ins)
{
   // dst = sin(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "sin(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool EXP(GenerateState &state, AluInstruction *ins)
{
   // dst = exp2(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "exp2(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool LOG(GenerateState &state, AluInstruction *ins)
{
   // dst = log2(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "log2(";
   translateAluSource(state, ins->sources[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FLT_TO_INT(GenerateState &state, AluInstruction *ins)
{
   // dst = (int)src0
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "(int)(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool INT_TO_FLT(GenerateState &state, AluInstruction *ins)
{
   // dst = (float)src0
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "(float)asint(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool UINT_TO_FLT(GenerateState &state, AluInstruction *ins)
{
   // dst = (float)src0
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "(float)asuint(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool PRED_SETE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == src1) ? 1 : 0
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins->sources[0]);
      state.out << " == ";
      translateAluSource(state, ins->sources[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins->sources[0]);
      state.out << " == ";
      translateAluSource(state, ins->sources[1]);
      state.out << ") ? ";

      if (ins->dest.valueType == AluDest::Float) {
         state.out << "1.0f : 0.0f";
      } else {
         state.out << "1 : 0";
      }

      translateAluDestEnd(state, ins);
   }

   return true;
}

static bool PRED_SETGE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 >= src1) ? 1 : 0
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins->sources[0]);
      state.out << " >= ";
      translateAluSource(state, ins->sources[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins->sources[0]);
      state.out << " >= ";
      translateAluSource(state, ins->sources[1]);
      state.out << ") ? ";

      if (ins->dest.valueType == AluDest::Float) {
         state.out << "1.0f : 0.0f";
      } else {
         state.out << "1 : 0";
      }

      translateAluDestEnd(state, ins);
   }

   return true;
}

static bool PRED_SETGT(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 > src1) ? 1 : 0
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins->sources[0]);
      state.out << " > ";
      translateAluSource(state, ins->sources[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins->sources[0]);
      state.out << " > ";
      translateAluSource(state, ins->sources[1]);
      state.out << ") ? ";

      if (ins->dest.valueType == AluDest::Float) {
         state.out << "1.0f : 0.0f";
      } else {
         state.out << "1 : 0";
      }

      translateAluDestEnd(state, ins);
   }

   return true;
}

static bool PRED_SETNE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 != src1) ? 1.0f : 0.0f
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      translateAluSource(state, ins->sources[0]);
      state.out << " != ";
      translateAluSource(state, ins->sources[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins->sources[0]);
      state.out << " != ";
      translateAluSource(state, ins->sources[1]);
      state.out << ") ? ";

      if (ins->dest.valueType == AluDest::Float) {
         state.out << "1.0f : 0.0f";
      } else {
         state.out << "1 : 0";
      }

      translateAluDestEnd(state, ins);
   }

   return true;
}

static bool RECIP(GenerateState &state, AluInstruction *ins)
{
   // dst = rcp(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "rcp(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool RECIPSQRT(GenerateState &state, AluInstruction *ins)
{
   // dst = rsqrt(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "rsqrt(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool SQRT(GenerateState &state, AluInstruction *ins)
{
   // dst = sqrt(src0)
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "sqrt(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool SETE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == src1) ? 1 : 0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins->sources[0]);
   state.out << " == ";
   translateAluSource(state, ins->sources[1]);
   state.out << ") ? ";

   if (ins->dest.valueType == AluDest::Float) {
      state.out << "1.0f : 0.0f";
   } else {
      state.out << "1 : 0";
   }

   translateAluDestEnd(state, ins);
   return true;
}

static bool SETGE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 >= src1) ? 1 : 0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins->sources[0]);
   state.out << " >= ";
   translateAluSource(state, ins->sources[1]);
   state.out << ") ? ";

   if (ins->dest.valueType == AluDest::Float) {
      state.out << "1.0f : 0.0f";
   } else {
      state.out << "1 : 0";
   }

   translateAluDestEnd(state, ins);
   return true;
}

static bool SETGT(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 > src1) ? 1 : 0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins->sources[0]);
   state.out << " > ";
   translateAluSource(state, ins->sources[1]);
   state.out << ") ? ";

   if (ins->dest.valueType == AluDest::Float) {
      state.out << "1.0f : 0.0f";
   } else {
      state.out << "1 : 0";
   }

   translateAluDestEnd(state, ins);
   return true;
}

static bool SETNE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 != src1) ? 1 : 0
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins->sources[0]);
   state.out << " != ";
   translateAluSource(state, ins->sources[1]);
   state.out << ") ? ";

   if (ins->dest.valueType == AluDest::Float) {
      state.out << "1.0f : 0.0f";
   } else {
      state.out << "1 : 0";
   }

   translateAluDestEnd(state, ins);
   return true;
}

void registerAluOP2()
{
   using latte::alu::op2;
   registerGenerator(op2::ADD, ADD);
   registerGenerator(op2::ADD_INT, ADD);
   registerGenerator(op2::SUB_INT, SUB);
   registerGenerator(op2::MUL, MUL);
   registerGenerator(op2::MUL_IEEE, MUL);
   registerGenerator(op2::AND_INT, AND);
   registerGenerator(op2::OR_INT, OR);
   registerGenerator(op2::XOR_INT, XOR);
   registerGenerator(op2::NOT_INT, NOT);
   registerGenerator(op2::ASHR_INT, ASHR);
   registerGenerator(op2::LSHR_INT, LSHR);
   registerGenerator(op2::LSHL_INT, LSHL);
   registerGenerator(op2::MAX, MAX);
   registerGenerator(op2::MIN, MIN);
   registerGenerator(op2::MAX_DX10, MAX);
   registerGenerator(op2::MIN_DX10, MIN);
   registerGenerator(op2::MAX_INT, MAX);
   registerGenerator(op2::MIN_INT, MIN);
   registerGenerator(op2::MAX_UINT, MAX);
   registerGenerator(op2::MIN_UINT, MIN);
   registerGenerator(op2::MOV, MOV);
   registerGenerator(op2::CEIL, CEIL);
   registerGenerator(op2::FLOOR, FLOOR);
   registerGenerator(op2::TRUNC, TRUNC);
   registerGenerator(op2::FRACT, FRACT);
   registerGenerator(op2::SIN, SIN);
   registerGenerator(op2::COS, COS);
   registerGenerator(op2::EXP_IEEE, EXP);
   registerGenerator(op2::LOG_IEEE, LOG);
   registerGenerator(op2::PRED_SETE, PRED_SETE);
   registerGenerator(op2::PRED_SETGE, PRED_SETGE);
   registerGenerator(op2::PRED_SETGT, PRED_SETGT);
   registerGenerator(op2::PRED_SETNE, PRED_SETNE);
   registerGenerator(op2::PRED_SETE_INT, PRED_SETE);
   registerGenerator(op2::PRED_SETGE_INT, PRED_SETGE);
   registerGenerator(op2::PRED_SETGT_INT, PRED_SETGT);
   registerGenerator(op2::PRED_SETNE_INT, PRED_SETNE);
   registerGenerator(op2::PRED_SETGE_UINT, PRED_SETGE);
   registerGenerator(op2::PRED_SETGT_UINT, PRED_SETGT);
   registerGenerator(op2::RECIPSQRT_IEEE, RECIPSQRT);
   registerGenerator(op2::RECIP_IEEE, RECIP);
   registerGenerator(op2::SETE, SETE);
   registerGenerator(op2::SETGE, SETGE);
   registerGenerator(op2::SETGT, SETGT);
   registerGenerator(op2::SETNE, SETNE);
   registerGenerator(op2::SETE_DX10, SETE);
   registerGenerator(op2::SETGE_DX10, SETGE);
   registerGenerator(op2::SETGT_DX10, SETGT);
   registerGenerator(op2::SETNE_DX10, SETNE);
   registerGenerator(op2::SETE_INT, SETE);
   registerGenerator(op2::SETGE_INT, SETGE);
   registerGenerator(op2::SETGT_INT, SETGT);
   registerGenerator(op2::SETNE_INT, SETNE);
   registerGenerator(op2::SETGE_UINT, SETGE);
   registerGenerator(op2::SETGT_UINT, SETGT);
   registerGenerator(op2::SQRT_IEEE, SQRT);
   registerGenerator(op2::UINT_TO_FLT, UINT_TO_FLT);
   registerGenerator(op2::INT_TO_FLT, INT_TO_FLT);
   registerGenerator(op2::FLT_TO_INT, FLT_TO_INT);
}

} // namespace hlsl
