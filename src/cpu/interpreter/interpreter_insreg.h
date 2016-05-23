#pragma once
#include "../state.h"
#include "../instruction.h"
#include "../instructiondata.h"
#include "../instructionid.h"

namespace cpu
{
namespace interpreter
{

using instrfptr_t = void(*)(ThreadState*, Instruction);

bool hasInstruction(InstructionID instrId);
instrfptr_t getInstructionHandler(InstructionID id);
void registerInstruction(InstructionID id, instrfptr_t fptr);
void registerBranchInstructions();
void registerConditionInstructions();
void registerFloatInstructions();
void registerIntegerInstructions();
void registerLoadStoreInstructions();
void registerPairedInstructions();
void registerSystemInstructions();

}
}

#undef RegisterInstruction
#undef RegisterInstructionFn

#define RegisterInstruction(x) \
   cpu::interpreter::registerInstruction(InstructionID::x, &x)
#define RegisterInstructionFn(x, fn) \
   cpu::interpreter::registerInstruction(InstructionID::x, &fn)
