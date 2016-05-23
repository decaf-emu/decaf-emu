#pragma once
#include "../state.h"
#include "../instruction.h"
#include "../instructionid.h"
#include "../instructiondata.h"
#include "jit_internal.h"

namespace cpu
{

namespace jit
{

using jitinstrfptr_t = bool(*)(PPCEmuAssembler&, Instruction);

bool hasInstruction(InstructionID instrId);
void registerInstruction(InstructionID id, jitinstrfptr_t fptr);
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
   cpu::jit::registerInstruction(InstructionID::x, &x)
#define RegisterInstructionFn(x, fn) \
   cpu::jit::registerInstruction(InstructionID::x, &fn)
#define RegisterInstructionFallback(x) \
   cpu::jit::registerInstruction(InstructionID::x, &cpu::jit::jit_fallback)
