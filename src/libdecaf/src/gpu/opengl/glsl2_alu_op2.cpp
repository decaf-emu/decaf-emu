#include "glsl2_alu.h"
#include "glsl2_translate.h"

using namespace latte;

/*
Unimplemented:
FREXP_64
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
PRED_SETE_PUSH_INT
PRED_SETGT_PUSH_INT
PRED_SETGE_PUSH_INT
PRED_SETNE_PUSH_INT
PRED_SETLT_PUSH_INT
PRED_SETLE_PUSH_INT
CUBE
MAX4
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
MULLO_INT
MULHI_INT
MULLO_UINT
MULHI_UINT
LDEXP_64
FRACT_64
PRED_SETGT_64
PRED_SETE_64
PRED_SETGE_64
*/

namespace glsl2
{

static void
unaryFunction(State &state,
               const ControlFlowInst &cf,
               const AluInst &alu,
               const std::string &func)
{
   // dst = func(src0)
   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   state.out << func << "(";
   insertSource0(state.out, state.literals, cf, alu);
   state.out << ")";

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
}

static void
binaryFunction(State &state,
               const ControlFlowInst &cf,
               const AluInst &alu,
               const std::string &func)
{
   // dst = func(src0, src1)
   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   state.out << func << "(";
   insertSource0(state.out, state.literals, cf, alu);
   state.out << ", ";
   insertSource1(state.out, state.literals, cf, alu);
   state.out << ")";

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
}

static void
binaryOperator(State &state,
               const ControlFlowInst &cf,
               const AluInst &alu,
               const std::string &op)
{
   // dst = src0 op src1
   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   insertSource0(state.out, state.literals, cf, alu);
   state.out << op;
   insertSource1(state.out, state.literals, cf, alu);

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
}

static void
binaryPredicate(State &state,
                const ControlFlowInst &cf,
                const AluInst &alu,
                const char *op)
{
   // predicate (src0 op src1)
   fmt::MemoryWriter condition;
   insertSource0(condition, state.literals, cf, alu);
   condition << op;
   insertSource1(condition, state.literals, cf, alu);
   updatePredicate(state, cf, alu, condition.str());
}

static void
binaryCompareSet(State &state,
                 const ControlFlowInst &cf,
                 const AluInst &alu,
                 const char *op)
{
   // dst = (src0 op src1) ? 1 : 0
   auto flags = getInstructionFlags(alu.op2.ALU_INST());

   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   state.out << "(";
   insertSource0(state.out, state.literals, cf, alu);
   state.out << op;
   insertSource1(state.out, state.literals, cf, alu);
   state.out << ") ? ";

   if ((flags & SQ_ALU_FLAG_INT_OUT) || (flags & SQ_ALU_FLAG_UINT_OUT)) {
      state.out << "1 : 0";
   } else {
      state.out << "1.0f : 0.0f";
   }

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
}

static void
binaryCompareKill(State &state,
                  const ControlFlowInst &cf,
                  const AluInst &alu,
                  const char *op)
{
   // if (src0 op src1) { discard; } else { dst = 0.0f; }
   auto flags = getInstructionFlags(alu.op2.ALU_INST());

   insertLineStart(state);
   state.out << "if (";
   insertSource0(state.out, state.literals, cf, alu);
   state.out << op;
   insertSource1(state.out, state.literals, cf, alu);
   state.out << ") {";
   insertLineEnd(state);

   increaseIndent(state);
   insertLineStart(state);
   state.out << "discard;";
   insertLineEnd(state);
   decreaseIndent(state);

   insertLineStart(state);
   state.out << "} else {";
   insertLineEnd(state);

   increaseIndent(state);
   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   if ((flags & SQ_ALU_FLAG_INT_OUT) || (flags & SQ_ALU_FLAG_UINT_OUT)) {
      state.out << "0";
   } else {
      state.out << "0.0f";
   }

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
   decreaseIndent(state);

   insertLineStart(state);
   state.out << '}';
   insertLineEnd(state);
}

static void
ADD(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " + ");
}

static void
AND(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " & ");
}

static void
CEIL(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "ceil");
}

static void
COS(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "cos");
}

static void
EXP(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "exp");
}

static void
FLOOR(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "floor");
}

static void
FLT_TO_INT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "int");
}

static void
FLT_TO_UINT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "uint");
}

static void
FRACT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "fract");
}

static void
INT_TO_FLT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "float");
}

static void
LSHL(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " << ");
}

static void
LSHR(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " >> ");
}

static void
LOG(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "log2");
}

static void
KILLE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareKill(state, cf, alu, " == ");
}

static void
KILLNE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareKill(state, cf, alu, " != ");
}

static void
KILLGE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareKill(state, cf, alu, " >= ");
}

static void
KILLGT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareKill(state, cf, alu, " > ");
}

static void
MAX(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryFunction(state, cf, alu, "max");
}

static void
MIN(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryFunction(state, cf, alu, "min");
}

static void
MOV(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   // dst = src0
   insertLineStart(state);
   insertDestBegin(state.out, cf, alu, state.unit);

   insertSource0(state.out, state.literals, cf, alu);

   insertDestEnd(state.out, cf, alu);
   state.out << ';';
   insertLineEnd(state);
}

static void
MOVA_FLOOR(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   // ar.x = dst = int(clamp(floor(src0), -256, 256))

   // TODO: Figure out how to handle these following cases
   if (state.unit != SQ_CHAN_X) {
      throw std::logic_error("Unexpected MOVA_FLOOR with unit != x");
   }

   if (alu.op2.WRITE_MASK()) {
      throw std::logic_error("Unexpected MOVA_FLOOR with WRITE_MASK = true");
   }

   insertLineStart(state);
   state.out << "AR.x = int(clamp(floor(";
   insertSource0(state.out, state.literals, cf, alu);
   state.out << "), -256, 256));";
   insertLineEnd(state);
}

static void
MUL(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " * ");
}

static void
NOT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "~");
}

static void
NOP(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
}

static void
OR(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " | ");
}

static void
PRED_SETE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryPredicate(state, cf, alu, " == ");
}

static void
PRED_SETNE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryPredicate(state, cf, alu, " != ");
}

static void
PRED_SETGE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryPredicate(state, cf, alu, " >= ");
}

static void
PRED_SETGT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryPredicate(state, cf, alu, " > ");
}

static void
RECIP(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "1 / ");
}

static void
RECIPSQRT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "inversesqrt");
}

static void
RNDNE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "roundEven");
}

static void
SETE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareSet(state, cf, alu, " == ");
}

static void
SETNE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareSet(state, cf, alu, " != ");
}

static void
SETGE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareSet(state, cf, alu, " >= ");
}

static void
SETGT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryCompareSet(state, cf, alu, " > ");
}

static void
SIN(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "sin");
}

static void
SQRT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "sqrt");
}

static void
SUB(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " - ");
}

static void
TRUNC(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "trunc");
}

static void
UINT_TO_FLT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   unaryFunction(state, cf, alu, "uint");
}

static void
XOR(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   binaryOperator(state, cf, alu, " ^ ");
}

void
registerOP2Functions()
{
   registerInstruction(latte::SQ_OP2_INST_ADD, ADD);
   registerInstruction(latte::SQ_OP2_INST_ADD_INT, ADD);
   registerInstruction(latte::SQ_OP2_INST_AND_INT, AND);
   registerInstruction(latte::SQ_OP2_INST_ASHR_INT, LSHR);
   registerInstruction(latte::SQ_OP2_INST_CEIL, CEIL);
   registerInstruction(latte::SQ_OP2_INST_COS, COS);
   registerInstruction(latte::SQ_OP2_INST_EXP_IEEE, EXP);
   registerInstruction(latte::SQ_OP2_INST_FLOOR, FLOOR);
   registerInstruction(latte::SQ_OP2_INST_FLT_TO_INT, FLT_TO_INT);
   registerInstruction(latte::SQ_OP2_INST_FLT_TO_UINT, FLT_TO_UINT);
   registerInstruction(latte::SQ_OP2_INST_FRACT, FRACT);
   registerInstruction(latte::SQ_OP2_INST_INT_TO_FLT, INT_TO_FLT);
   registerInstruction(latte::SQ_OP2_INST_LOG_CLAMPED, LOG);
   registerInstruction(latte::SQ_OP2_INST_LOG_IEEE, LOG);
   registerInstruction(latte::SQ_OP2_INST_LSHL_INT, LSHL);
   registerInstruction(latte::SQ_OP2_INST_LSHR_INT, LSHR);
   registerInstruction(latte::SQ_OP2_INST_KILLE, KILLE);
   registerInstruction(latte::SQ_OP2_INST_KILLE_INT, KILLE);
   registerInstruction(latte::SQ_OP2_INST_KILLGE, KILLGE);
   registerInstruction(latte::SQ_OP2_INST_KILLGE_INT, KILLGE);
   registerInstruction(latte::SQ_OP2_INST_KILLGE_UINT, KILLGE);
   registerInstruction(latte::SQ_OP2_INST_KILLGT, KILLGT);
   registerInstruction(latte::SQ_OP2_INST_KILLGT_INT, KILLGT);
   registerInstruction(latte::SQ_OP2_INST_KILLGT_UINT, KILLGT);
   registerInstruction(latte::SQ_OP2_INST_KILLNE, KILLNE);
   registerInstruction(latte::SQ_OP2_INST_KILLNE_INT, KILLNE);
   registerInstruction(latte::SQ_OP2_INST_MAX, MAX);
   registerInstruction(latte::SQ_OP2_INST_MAX_DX10, MAX);
   registerInstruction(latte::SQ_OP2_INST_MAX_INT, MAX);
   registerInstruction(latte::SQ_OP2_INST_MAX_UINT, MAX);
   registerInstruction(latte::SQ_OP2_INST_MIN, MIN);
   registerInstruction(latte::SQ_OP2_INST_MIN_DX10, MAX);
   registerInstruction(latte::SQ_OP2_INST_MIN_INT, MIN);
   registerInstruction(latte::SQ_OP2_INST_MIN_UINT, MIN);
   registerInstruction(latte::SQ_OP2_INST_MOV, MOV);
   registerInstruction(latte::SQ_OP2_INST_MOVA_FLOOR, MOVA_FLOOR);
   registerInstruction(latte::SQ_OP2_INST_MUL, MUL);
   registerInstruction(latte::SQ_OP2_INST_MUL_IEEE, MUL);
   registerInstruction(latte::SQ_OP2_INST_NOT_INT, NOT);
   registerInstruction(latte::SQ_OP2_INST_NOP, NOP);
   registerInstruction(latte::SQ_OP2_INST_OR_INT, OR);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETE, PRED_SETE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETE_INT, PRED_SETE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGE, PRED_SETGE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGE_INT, PRED_SETGE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGE_UINT, PRED_SETGE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGT, PRED_SETGT);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGT_INT, PRED_SETGT);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETGT_UINT, PRED_SETGT);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETNE, PRED_SETNE);
   registerInstruction(latte::SQ_OP2_INST_PRED_SETNE_INT, PRED_SETNE);
   registerInstruction(latte::SQ_OP2_INST_RECIP_CLAMPED, RECIP);
   registerInstruction(latte::SQ_OP2_INST_RECIP_IEEE, RECIP);
   registerInstruction(latte::SQ_OP2_INST_RECIP_FF, RECIP);
   registerInstruction(latte::SQ_OP2_INST_RECIP_INT, RECIP);
   registerInstruction(latte::SQ_OP2_INST_RECIP_UINT, RECIP);
   registerInstruction(latte::SQ_OP2_INST_RECIPSQRT_CLAMPED, RECIPSQRT);
   registerInstruction(latte::SQ_OP2_INST_RECIPSQRT_IEEE, RECIPSQRT);
   registerInstruction(latte::SQ_OP2_INST_RECIPSQRT_FF, RECIPSQRT);
   registerInstruction(latte::SQ_OP2_INST_RNDNE, RNDNE);
   registerInstruction(latte::SQ_OP2_INST_SETE, SETE);
   registerInstruction(latte::SQ_OP2_INST_SETE_INT, SETE);
   registerInstruction(latte::SQ_OP2_INST_SETE_DX10, SETE);
   registerInstruction(latte::SQ_OP2_INST_SETGE, SETGE);
   registerInstruction(latte::SQ_OP2_INST_SETGE_INT, SETGE);
   registerInstruction(latte::SQ_OP2_INST_SETGE_UINT, SETGE);
   registerInstruction(latte::SQ_OP2_INST_SETGE_DX10, SETGE);
   registerInstruction(latte::SQ_OP2_INST_SETGT, SETGT);
   registerInstruction(latte::SQ_OP2_INST_SETGT_INT, SETGT);
   registerInstruction(latte::SQ_OP2_INST_SETGT_UINT, SETGT);
   registerInstruction(latte::SQ_OP2_INST_SETGT_DX10, SETGT);
   registerInstruction(latte::SQ_OP2_INST_SETNE, SETNE);
   registerInstruction(latte::SQ_OP2_INST_SETNE_INT, SETNE);
   registerInstruction(latte::SQ_OP2_INST_SETNE_DX10, SETNE);
   registerInstruction(latte::SQ_OP2_INST_SIN, SIN);
   registerInstruction(latte::SQ_OP2_INST_SQRT_IEEE, SQRT);
   registerInstruction(latte::SQ_OP2_INST_SUB_INT, SUB);
   registerInstruction(latte::SQ_OP2_INST_TRUNC, TRUNC);
   registerInstruction(latte::SQ_OP2_INST_UINT_TO_FLT, UINT_TO_FLT);
   registerInstruction(latte::SQ_OP2_INST_XOR_INT, XOR);
}

} // namespace glsl2
