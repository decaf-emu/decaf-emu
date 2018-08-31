#include "glsl2_alu.h"
#include "glsl2_translate.h"

using namespace latte;

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

namespace glsl2
{

static void
conditionalMove(State &state,
                const ControlFlowInst &cf,
                const AluInst &alu,
                const char *op)
{
   // dst = (src0 op 0) ? src1 : src2
   insertLineStart(state);
   insertDestBegin(state, cf, alu, state.unit);

   fmt::format_to(state.out, "(");
   insertSource0(state, state.out, cf, alu);
   fmt::format_to(state.out, "{}0) ? ", op);
   insertSource1(state, state.out, cf, alu);
   fmt::format_to(state.out, " : ");
   insertSource2(state, state.out, cf, alu);

   insertDestEnd(state, cf, alu);
   fmt::format_to(state.out, ";");
   insertLineEnd(state);
}

static void
multiplyAdd(State &state,
            const ControlFlowInst &cf,
            const AluInst &alu,
            const char *modifier = nullptr)
{
   // dst = (src0 * src1 + src2) modifier
   insertLineStart(state);
   insertDestBegin(state, cf, alu, state.unit);

   if (modifier) {
      fmt::format_to(state.out, "(");
   }

   insertSource0(state, state.out, cf, alu);
   fmt::format_to(state.out, " * ");
   insertSource1(state, state.out, cf, alu);
   fmt::format_to(state.out, " + ");
   insertSource2(state, state.out, cf, alu);

   if (modifier) {
      fmt::format_to(state.out, "){}", modifier);
   }

   insertDestEnd(state, cf, alu);
   fmt::format_to(state.out, ";");
   insertLineEnd(state);
}

static void
CNDE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   conditionalMove(state, cf, alu, " == ");
}

static void
CNDGE(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   conditionalMove(state, cf, alu, " >= ");
}

static void
CNDGT(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   conditionalMove(state, cf, alu, " > ");
}

static void
MULADD(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   multiplyAdd(state, cf, alu);
}

static void
MULADD_M2(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   multiplyAdd(state, cf, alu, " * 2");
}

static void
MULADD_M4(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   multiplyAdd(state, cf, alu, " * 4");
}

static void
MULADD_D2(State &state, const ControlFlowInst &cf, const AluInst &alu)
{
   multiplyAdd(state, cf, alu, " / 2");
}

void
registerOP3Functions()
{
   registerInstruction(latte::SQ_OP3_INST_CNDE, CNDE);
   registerInstruction(latte::SQ_OP3_INST_CNDE_INT, CNDE);
   registerInstruction(latte::SQ_OP3_INST_CNDGE, CNDGE);
   registerInstruction(latte::SQ_OP3_INST_CNDGE_INT, CNDGE);
   registerInstruction(latte::SQ_OP3_INST_CNDGT, CNDGT);
   registerInstruction(latte::SQ_OP3_INST_CNDGT_INT, CNDGT);
   registerInstruction(latte::SQ_OP3_INST_MULADD, MULADD);
   registerInstruction(latte::SQ_OP3_INST_MULADD_IEEE, MULADD);
   registerInstruction(latte::SQ_OP3_INST_MULADD_M2, MULADD_M2);
   registerInstruction(latte::SQ_OP3_INST_MULADD_IEEE_M2, MULADD_M2);
   registerInstruction(latte::SQ_OP3_INST_MULADD_M4, MULADD_M4);
   registerInstruction(latte::SQ_OP3_INST_MULADD_IEEE_M4, MULADD_M4);
   registerInstruction(latte::SQ_OP3_INST_MULADD_D2, MULADD_D2);
   registerInstruction(latte::SQ_OP3_INST_MULADD_IEEE_D2, MULADD_D2);
}

} // namespace glsl2
