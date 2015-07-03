#pragma once
#include <cstdint>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "instruction.h"
#include "instructionid.h"
#include "ppc.h"
#include "jit.h"

#define RegisterInstruction(x) \
   registerInstruction(InstructionID::##x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::##x, &fn)

using instrfptr_t = void(*)(ThreadState*, Instruction);

class Interpreter
{
public:
   Interpreter()
      : mJitEnabled(false) {}

   void setJitEnabled(bool val);
   void execute(ThreadState *state, uint32_t addr);
   void addBreakpoint(uint32_t addr);

private:
   void execute(ThreadState *state);
   bool mJitEnabled;
   std::vector<uint32_t> mBreakpoints;

public:
   static void RegisterFunctions();

private:
   static void registerInstruction(InstructionID id, instrfptr_t fptr);
   static void registerBranchInstructions();
   static void registerConditionInstructions();
   static void registerFloatInstructions();
   static void registerIntegerInstructions();
   static void registerLoadStoreInstructions();
   static void registerPairedInstructions();
   static void registerSystemInstructions();
};

extern Interpreter gInterpreter;
