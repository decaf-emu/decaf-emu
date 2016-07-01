#include "glsl2_translate.h"
#include "glsl2_cf.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

/*
Unimplemented:
NOP
LOOP_START
LOOP_END
LOOP_START_DX10
LOOP_START_NO_AL
LOOP_CONTINUE
LOOP_BREAK
PUSH
PUSH_ELSE
POP_JUMP
POP_PUSH
POP_PUSH_ELSE
CALL
RETURN
EMIT_VERTEX
EMIT_CUT_VERTEX
CUT_VERTEX
WAIT_ACK
TEX_ACK
VTX_ACK
VTX_TC_ACK
*/

namespace glsl2
{

static void
CALL_FS(State &state, const ControlFlowInst &cf)
{
}

static void
ELSE(State &state, const ControlFlowInst &cf)
{
   insertLineStart(state);
   state.out << "activeMask = !activeMask;";
   insertLineEnd(state);

   if (cf.word1.POP_COUNT()) {
      insertLineStart(state);
      state.out << "if (!activeMask) {";
      insertLineEnd(state);

      increaseIndent(state);
      insertPop(state, cf.word1.POP_COUNT());
      decreaseIndent(state);

      insertLineStart(state);
      state.out << "}";
      insertLineEnd(state);
   }
}

static void
END_PROGRAM(State &state, const ControlFlowInst &cf)
{
}

static void
JUMP(State &state, const ControlFlowInst &cf)
{
   if (cf.word1.POP_COUNT()) {
      insertLineStart(state);
      state.out << "if (!activeMask) {";
      insertLineEnd(state);

      increaseIndent(state);
      insertPop(state, cf.word1.POP_COUNT());
      decreaseIndent(state);

      insertLineStart(state);
      state.out << "}";
      insertLineEnd(state);
   }
}

static void
KILL(State &state, const ControlFlowInst &cf)
{
   insertLineStart(state);
   state.out << "discard;";
   insertLineEnd(state);
}

static void
NOP(State &state, const ControlFlowInst &cf)
{
}

static void
POP(State &state, const ControlFlowInst &cf)
{
   if (cf.word1.POP_COUNT()) {
      insertPop(state, cf.word1.POP_COUNT());
   }
}

void
registerCfFunctions()
{
   registerInstruction(SQ_CF_INST_CALL_FS, CALL_FS);
   registerInstruction(SQ_CF_INST_ELSE, ELSE);
   registerInstruction(SQ_CF_INST_END_PROGRAM, END_PROGRAM);
   registerInstruction(SQ_CF_INST_JUMP, JUMP);
   registerInstruction(SQ_CF_INST_KILL, KILL);
   registerInstruction(SQ_CF_INST_NOP, NOP);
   registerInstruction(SQ_CF_INST_POP, POP);
}

} // namespace glsl2
