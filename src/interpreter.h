#pragma once
#include <cstdint>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "instruction.h"
#include "instructionid.h"
#include "ppc.h"
#include "jit.h"
#include "wfunc_ptr.h"

#define RegisterInstruction(x) \
   registerInstruction(InstructionID::x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::x, &fn)

using instrfptr_t = void(*)(ThreadState*, Instruction);

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
   void executeSub(ThreadState *state);

   InterpJitMode getJitMode() const {
      return mJitMode;
   }

private:
   void execute(ThreadState *state);
   InterpJitMode mJitMode;

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

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   ThreadState *state = GetCurrentFiberState();

   // Push args
   ppctypes::applyArguments(state, args...);

   // Save state
   auto nia = state->nia;

   // Set state
   state->cia = 0;
   state->nia = address;
   gInterpreter.executeSub(state);

   // Restore state
   state->nia = nia;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}
