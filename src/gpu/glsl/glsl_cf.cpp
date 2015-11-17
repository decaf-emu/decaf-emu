#include "glsl_generator.h"

using latte::shadir::CfInstruction;

/*
Unimplemented:
NOP
VTX
VTX_TC
JUMP
PUSH
PUSH_ELSE
ELSE
POP
POP_JUMP
POP_PUSH
POP_PUSH_ELSE
CALL
CALL_FS
RETURN
EMIT_VERTEX
EMIT_CUT_VERTEX
CUT_VERTEX
KILL
END_PROGRAM
WAIT_ACK
TEX_ACK
VTX_ACK
VTX_TC_ACK
*/

namespace glsl
{

static bool LOOP_BREAK(GenerateState &state, CfInstruction *ins)
{
   state.out << "break;";
   return true;
}

static bool LOOP_CONTINUE(GenerateState &state, CfInstruction *ins)
{
   state.out << "continue;";
   return true;
}

void registerCf()
{
   registerGenerator(latte::cf::inst::LOOP_BREAK, LOOP_BREAK);
   registerGenerator(latte::cf::inst::LOOP_CONTINUE, LOOP_CONTINUE);
}

} // namespace hlsl
