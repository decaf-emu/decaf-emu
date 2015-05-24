#pragma once
#include <cstdint>
#include <vector>
#include "instruction.h"
#include "instructionid.h"
#include "ppc.h"

#define RegisterInstruction(x) \
   registerInstruction(InstructionID::##x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::##x, &fn)

class Interpreter
{
   using fptr_t = void(*)(ThreadState*, Instruction);

public:
   void initialise();
   void execute(ThreadState *state);

private:
   void registerInstruction(InstructionID id, fptr_t fptr);
   void registerBranchInstructions();
   void registerConditionInstructions();
   void registerIntegerInstructions();
   void registerLoadStoreInstructions();
   void registerSystemInstructions();

   std::vector<fptr_t> mInstructionMap;
};
