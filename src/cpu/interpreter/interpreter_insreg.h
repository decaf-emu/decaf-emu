#pragma once
#include "../state.h"
#include "../espresso/espresso_instruction.h"
#include "../espresso/espresso_instructionid.h"

// TODO: Remove me
using espresso::Instruction;
using espresso::InstructionID;

namespace cpu
{

namespace interpreter
{

using instrfptr_t = void(*)(Core*, Instruction);

bool
hasInstruction(espresso::InstructionID instrId);

instrfptr_t
getInstructionHandler(espresso::InstructionID id);

void
registerInstruction(espresso::InstructionID id, instrfptr_t fptr);

void
registerBranchInstructions();

void
registerConditionInstructions();

void
registerFloatInstructions();

void
registerIntegerInstructions();

void
registerLoadStoreInstructions();

void
registerPairedInstructions();

void
registerSystemInstructions();

} // namespace interpreter

} // namespace cpu

#undef RegisterInstruction
#undef RegisterInstructionFn

#define RegisterInstruction(x) \
   cpu::interpreter::registerInstruction(espresso::InstructionID::x, &x)
#define RegisterInstructionFn(x, fn) \
   cpu::interpreter::registerInstruction(espresso::InstructionID::x, &fn)
