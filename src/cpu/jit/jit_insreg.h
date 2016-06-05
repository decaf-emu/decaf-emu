#pragma once
#include "../state.h"
#include "../espresso/espresso_instruction.h"
#include "../espresso/espresso_instructionid.h"
#include "jit_internal.h"

// TODO: Remove me
using espresso::Instruction;

namespace cpu
{

namespace jit
{

using jitinstrfptr_t = bool(*)(PPCEmuAssembler&, Instruction);

void registerInstruction(espresso::InstructionID id, jitinstrfptr_t fptr);
void registerBranchInstructions();
void registerConditionInstructions();
void registerFloatInstructions();
void registerIntegerInstructions();
void registerLoadStoreInstructions();
void registerPairedInstructions();
void registerSystemInstructions();

bool jit_fallback(PPCEmuAssembler& a, Instruction instr);

} // namespace jit

} // namespace cpu

#undef RegisterInstruction
#undef RegisterInstructionFn
#undef RegisterInstructionFallback

#define RegisterInstruction(x) \
   cpu::jit::registerInstruction(espresso::InstructionID::x, &x)
#define RegisterInstructionFn(x, fn) \
   cpu::jit::registerInstruction(espresso::InstructionID::x, &fn)
#define RegisterInstructionFallback(x) \
   cpu::jit::registerInstruction(espresso::InstructionID::x, &cpu::jit::jit_fallback)
