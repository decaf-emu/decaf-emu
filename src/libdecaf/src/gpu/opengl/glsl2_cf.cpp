#include "glsl2_translate.h"
#include "glsl2_cf.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

/*
Unimplemented:
LOOP_START
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
   insertElse(state);
}

static void
END_PROGRAM(State &state, const ControlFlowInst &cf)
{
}

static void
JUMP(State &state, const ControlFlowInst &cf)
{
}

static void
KILL(State &state, const ControlFlowInst &cf)
{
   insertLineStart(state);
   state.out << "discard;";
   insertLineEnd(state);

   state.shader->usesDiscard = true;
}

static void
LOOP_END(State &state, const ControlFlowInst &cf)
{
   // TODO: LOOP_END has different behaviour depending on which LOOP_START
   // instruction started the loop, currently we only handle LOOP_START_DX10
   auto &loopState = state.loopStack.top();
   auto loopIndex = state.loopStack.size() - 1;

   // Sanity check to ensure we are at the cfPC
   decaf_check(state.cfPC == loopState.endPC);
   decaf_check((cf.word0.ADDR - 1) == loopState.startPC);

   state.loopStack.pop();

   // If breakMask is set, lets break from the while
   insertLineStart(state);
   state.out << "if (activeMask == InactiveBreak) {";
   insertLineEnd(state);

   increaseIndent(state);
   insertLineStart(state);
   state.out << "break;";
   insertLineEnd(state);
   decreaseIndent(state);

   insertLineStart(state);
   state.out << "}";
   insertLineEnd(state);

   // If ContinueMask is set, lets break from the while
   insertLineStart(state);
   state.out << "if (activeMask == InactiveContinue) {";
   insertLineEnd(state);

   increaseIndent(state);
   insertLineStart(state);
   state.out << "activeMask = Active;";
   insertLineEnd(state);
   decreaseIndent(state);

   insertLineStart(state);
   state.out << "}";
   insertLineEnd(state);

   // Check the while condition but without checking loop masks
   decreaseIndent(state);
   insertLineStart(state);
   state.out << "} while (";
   insertCond(state, cf.word1.COND());
   state.out << ");";
   insertLineEnd(state);

   insertPop(state);
   condEnd(state);
}

static void
LOOP_START_DX10(State &state, const ControlFlowInst &cf)
{
   LoopState loop;
   loop.startPC = state.cfPC;
   loop.endPC = cf.word0.ADDR - 1;
   state.loopStack.emplace(loop);

   condStart(state, cf.word1.COND());
   insertPush(state);

   insertLineStart(state);
   state.out << "do {";
   insertLineEnd(state);
   increaseIndent(state);
}

static void
NOP(State &state, const ControlFlowInst &cf)
{
}

static void
POP(State &state, const ControlFlowInst &cf)
{
   insertPop(state, cf.word1.POP_COUNT());
}

void
registerCfFunctions()
{
   registerInstruction(SQ_CF_INST_CALL_FS, CALL_FS);
   registerInstruction(SQ_CF_INST_ELSE, ELSE);
   registerInstruction(SQ_CF_INST_END_PROGRAM, END_PROGRAM);
   registerInstruction(SQ_CF_INST_JUMP, JUMP);
   registerInstruction(SQ_CF_INST_KILL, KILL);
   registerInstruction(SQ_CF_INST_LOOP_END, LOOP_END);
   registerInstruction(SQ_CF_INST_LOOP_START_DX10, LOOP_START_DX10);
   registerInstruction(SQ_CF_INST_NOP, NOP);
   registerInstruction(SQ_CF_INST_POP, POP);
}

} // namespace glsl2
