#include "glsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;
using latte::shadir::AluDest;
using latte::shadir::ValueType;

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

namespace gpu
{

namespace opengl
{

namespace glsl
{

static bool ADD(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 + src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " + ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool SUB(GenerateState &state, AluInstruction *ins)
{
   // dst = src1 - src0
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[1]);
   state.out << " - ";
   translateAluSource(state, ins, ins->src[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MUL(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 * src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " * ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool AND(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 & src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " & ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool OR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 | src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " | ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool XOR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 ^ src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " ^ ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool NOT(GenerateState &state, AluInstruction *ins)
{
   // dst = ~src0
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << '~';
   translateAluSource(state, ins, ins->src[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool ASHR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 >> (src1 & 0x1f)
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " >> (";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " & 0x1f)";

   translateAluDestEnd(state, ins);
   return true;
}

static bool LSHR(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 >> (src1 & 0x1f)
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " >> (";
   translateAluSource(state, ins, ins->src[1]);
   state.out << " & 0x1f)";

   translateAluDestEnd(state, ins);
   return true;
}

static bool LSHL(GenerateState &state, AluInstruction *ins)
{
   // dst = src0 << src1
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);
   state.out << " << ";
   translateAluSource(state, ins, ins->src[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MAX(GenerateState &state, AluInstruction *ins)
{
   // dst = max(src0, src1)
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "max(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ", ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MIN(GenerateState &state, AluInstruction *ins)
{
   // dst = min(src0, src1)
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "min(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ", ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool MOV(GenerateState &state, AluInstruction *ins)
{
   // dst = src0
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins, ins->src[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool MOVA_FLOOR(GenerateState &state, AluInstruction *ins)
{
   // AR.chan = floor(src0)
   state.out << "AR.";
   translateChannel(state, ins->dst.chan);

   state.out << " = int(floor(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << "))";
   return true;
}

static bool CEIL(GenerateState &state, AluInstruction *ins)
{
   // dst = ceil(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "ceil(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FLOOR(GenerateState &state, AluInstruction *ins)
{
   // dst = floor(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "floor(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FRACT(GenerateState &state, AluInstruction *ins)
{
   // dst = fract(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "fract(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool TRUNC(GenerateState &state, AluInstruction *ins)
{
   // dst = trunc(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "trunc(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool COS(GenerateState &state, AluInstruction *ins)
{
   // dst = cos(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "cos(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool SIN(GenerateState &state, AluInstruction *ins)
{
   // dst = sin(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "sin(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool EXP(GenerateState &state, AluInstruction *ins)
{
   // dst = exp2(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "exp2(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool LOG(GenerateState &state, AluInstruction *ins)
{
   // dst = log2(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "log2(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool FLT_TO_INT(GenerateState &state, AluInstruction *ins)
{
   // dst = (int)src0
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "int(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool INT_TO_FLT(GenerateState &state, AluInstruction *ins)
{
   // dst = (float)src0
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "float(floatBitsToInt(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << "))";

   translateAluDestEnd(state, ins);
   return true;
}

static bool UINT_TO_FLT(GenerateState &state, AluInstruction *ins)
{
   // dst = (float)src0
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "float(floatBitsToUint(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << "))";

   translateAluDestEnd(state, ins);
   return true;
}

static bool PRED_SETE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == src1) ? 1 : 0
   assert(ins->srcCount == 2);

   if (!ins->writeMask && ins->updateExecuteMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins, ins->src[0]);
      state.out << " == ";
      translateAluSource(state, ins, ins->src[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins, ins->src[0]);
      state.out << " == ";
      translateAluSource(state, ins, ins->src[1]);
      state.out << ") ? ";

      if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);

   if (!ins->writeMask && ins->updateExecuteMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins, ins->src[0]);
      state.out << " >= ";
      translateAluSource(state, ins, ins->src[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins, ins->src[0]);
      state.out << " >= ";
      translateAluSource(state, ins, ins->src[1]);
      state.out << ") ? ";

      if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);

   if (!ins->writeMask && ins->updateExecuteMask && ins->updatePredicate) {
      // Should be inside a conditional
      translateAluSource(state, ins, ins->src[0]);
      state.out << " > ";
      translateAluSource(state, ins, ins->src[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins, ins->src[0]);
      state.out << " > ";
      translateAluSource(state, ins, ins->src[1]);
      state.out << ") ? ";

      if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);

   if (!ins->writeMask && ins->updateExecuteMask && ins->updatePredicate) {
      translateAluSource(state, ins, ins->src[0]);
      state.out << " != ";
      translateAluSource(state, ins, ins->src[1]);
   } else {
      translateAluDestStart(state, ins);
      state.out << '(';
      translateAluSource(state, ins, ins->src[0]);
      state.out << " != ";
      translateAluSource(state, ins, ins->src[1]);
      state.out << ") ? ";

      if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "(1.0 / (";
   translateAluSource(state, ins, ins->src[0]);
   state.out << "))";

   translateAluDestEnd(state, ins);
   return true;
}

static bool RECIPSQRT(GenerateState &state, AluInstruction *ins)
{
   // dst = rsqrt(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "inversesqrt(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool SQRT(GenerateState &state, AluInstruction *ins)
{
   // dst = sqrt(src0)
   assert(ins->srcCount == 1);
   translateAluDestStart(state, ins);

   state.out << "sqrt(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool SETE(GenerateState &state, AluInstruction *ins)
{
   // dst = (src0 == src1) ? 1 : 0
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " == ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ") ? ";

   if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " >= ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ") ? ";

   if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " > ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ") ? ";

   if (ins->dst.type == ValueType::Float) {
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
   assert(ins->srcCount == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins, ins->src[0]);
   state.out << " != ";
   translateAluSource(state, ins, ins->src[1]);
   state.out << ") ? ";

   if (ins->dst.type == ValueType::Float) {
      state.out << "1.0f : 0.0f";
   } else {
      state.out << "1 : 0";
   }

   translateAluDestEnd(state, ins);
   return true;
}

void registerAluOP2()
{
   registerGenerator(latte::SQ_OP2_INST_ADD, ADD);
   registerGenerator(latte::SQ_OP2_INST_ADD_INT, ADD);
   registerGenerator(latte::SQ_OP2_INST_SUB_INT, SUB);
   registerGenerator(latte::SQ_OP2_INST_MUL, MUL);
   registerGenerator(latte::SQ_OP2_INST_MUL_IEEE, MUL);
   registerGenerator(latte::SQ_OP2_INST_AND_INT, AND);
   registerGenerator(latte::SQ_OP2_INST_OR_INT, OR);
   registerGenerator(latte::SQ_OP2_INST_XOR_INT, XOR);
   registerGenerator(latte::SQ_OP2_INST_NOT_INT, NOT);
   registerGenerator(latte::SQ_OP2_INST_ASHR_INT, ASHR);
   registerGenerator(latte::SQ_OP2_INST_LSHR_INT, LSHR);
   registerGenerator(latte::SQ_OP2_INST_LSHL_INT, LSHL);
   registerGenerator(latte::SQ_OP2_INST_MAX, MAX);
   registerGenerator(latte::SQ_OP2_INST_MIN, MIN);
   registerGenerator(latte::SQ_OP2_INST_MAX_DX10, MAX);
   registerGenerator(latte::SQ_OP2_INST_MIN_DX10, MIN);
   registerGenerator(latte::SQ_OP2_INST_MAX_INT, MAX);
   registerGenerator(latte::SQ_OP2_INST_MIN_INT, MIN);
   registerGenerator(latte::SQ_OP2_INST_MAX_UINT, MAX);
   registerGenerator(latte::SQ_OP2_INST_MIN_UINT, MIN);
   registerGenerator(latte::SQ_OP2_INST_MOV, MOV);
   registerGenerator(latte::SQ_OP2_INST_MOVA_FLOOR, MOVA_FLOOR);
   registerGenerator(latte::SQ_OP2_INST_CEIL, CEIL);
   registerGenerator(latte::SQ_OP2_INST_FLOOR, FLOOR);
   registerGenerator(latte::SQ_OP2_INST_TRUNC, TRUNC);
   registerGenerator(latte::SQ_OP2_INST_FRACT, FRACT);
   registerGenerator(latte::SQ_OP2_INST_SIN, SIN);
   registerGenerator(latte::SQ_OP2_INST_COS, COS);
   registerGenerator(latte::SQ_OP2_INST_EXP_IEEE, EXP);
   registerGenerator(latte::SQ_OP2_INST_LOG_IEEE, LOG);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETE, PRED_SETE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGE, PRED_SETGE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGT, PRED_SETGT);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETNE, PRED_SETNE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETE_INT, PRED_SETE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGE_INT, PRED_SETGE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGT_INT, PRED_SETGT);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETNE_INT, PRED_SETNE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGE_UINT, PRED_SETGE);
   registerGenerator(latte::SQ_OP2_INST_PRED_SETGT_UINT, PRED_SETGT);
   registerGenerator(latte::SQ_OP2_INST_RECIPSQRT_IEEE, RECIPSQRT);
   registerGenerator(latte::SQ_OP2_INST_RECIP_IEEE, RECIP);
   registerGenerator(latte::SQ_OP2_INST_SETE, SETE);
   registerGenerator(latte::SQ_OP2_INST_SETGE, SETGE);
   registerGenerator(latte::SQ_OP2_INST_SETGT, SETGT);
   registerGenerator(latte::SQ_OP2_INST_SETNE, SETNE);
   registerGenerator(latte::SQ_OP2_INST_SETE_DX10, SETE);
   registerGenerator(latte::SQ_OP2_INST_SETGE_DX10, SETGE);
   registerGenerator(latte::SQ_OP2_INST_SETGT_DX10, SETGT);
   registerGenerator(latte::SQ_OP2_INST_SETNE_DX10, SETNE);
   registerGenerator(latte::SQ_OP2_INST_SETE_INT, SETE);
   registerGenerator(latte::SQ_OP2_INST_SETGE_INT, SETGE);
   registerGenerator(latte::SQ_OP2_INST_SETGT_INT, SETGT);
   registerGenerator(latte::SQ_OP2_INST_SETNE_INT, SETNE);
   registerGenerator(latte::SQ_OP2_INST_SETGE_UINT, SETGE);
   registerGenerator(latte::SQ_OP2_INST_SETGT_UINT, SETGT);
   registerGenerator(latte::SQ_OP2_INST_SQRT_IEEE, SQRT);
   registerGenerator(latte::SQ_OP2_INST_UINT_TO_FLT, UINT_TO_FLT);
   registerGenerator(latte::SQ_OP2_INST_INT_TO_FLT, INT_TO_FLT);
   registerGenerator(latte::SQ_OP2_INST_FLT_TO_INT, FLT_TO_INT);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
