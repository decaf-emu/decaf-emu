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
   registerInstruction(InstructionID::x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::x, &fn)

using instrfptr_t = void(*)(ThreadState*, Instruction);

// Address used to signify a return to emulator-land.
const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

enum class InterpJitMode {
   Enabled,
   Disabled,
   Debug
};

class Interpreter
{
public:
   Interpreter()
      : mJitMode(InterpJitMode::Disabled) {}

   void setJitMode(InterpJitMode val);
   void execute(ThreadState *state);
   void addBreakpoint(uint32_t addr);

   InterpJitMode getJitMode() const {
      return mJitMode;
   }

private:
   InterpJitMode mJitMode;
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
