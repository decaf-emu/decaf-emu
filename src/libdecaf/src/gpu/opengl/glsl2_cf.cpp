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
   // It's fine for a JUMP to skip an ELSE block entirely (that's just a
   //  nesting if block) -- what we don't want to see is a JUMP that enters
   //  into an ELSE block.
   if (!state.jumpStack.empty() && state.jumpStack.top().toPC < cf.word0.ADDR) {
      throw translate_exception(fmt::format("JUMP target {} enters ELSE block at {}-{}", state.jumpStack.top().toPC, state.cfPC, cf.word0.ADDR));
   }

   insertElse(state);
}

static void
END_PROGRAM(State &state, const ControlFlowInst &cf)
{
}

static void
JUMP(State &state, const ControlFlowInst &cf)
{
   if (cf.word0.ADDR <= state.cfPC) {
      throw translate_exception(fmt::format("JUMP at {} jumps backward", state.cfPC));
   }

   if (!state.jumpStack.empty() && cf.word0.ADDR > state.jumpStack.top().toPC) {
      throw translate_exception(fmt::format("JUMP at {} breaks nesting", state.cfPC));
   }

   insertLineStart(state);
   state.out << "if (activeMask != Active) {";
   insertLineEnd(state);
   increaseIndent(state);

   insertPop(state, cf.word1.POP_COUNT());

   decreaseIndent(state);
   insertLineStart(state);
   state.out << "} else {";
   insertLineEnd(state);
   increaseIndent(state);

   state.jumpStack.emplace(JumpState { state.cfPC, cf.word0.ADDR });
}

static void
KILL(State &state, const ControlFlowInst &cf)
{
   insertLineStart(state);
   state.out << "discard;";
   insertLineEnd(state);
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

   if (!state.jumpStack.empty() && state.jumpStack.top().fromPC > loopState.startPC) {
      throw translate_exception(fmt::format("JUMP from {} to {} crosses LOOP_END at {}", state.jumpStack.top().fromPC, state.jumpStack.top().toPC, state.cfPC));
   }

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
