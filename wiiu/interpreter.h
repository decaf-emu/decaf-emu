#pragma once
#include <cstdint>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "instruction.h"
#include "instructionid.h"
#include "ppc.h"
#include "gdbstub.h"

#define RegisterInstruction(x) \
   registerInstruction(InstructionID::##x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::##x, &fn)

class Interpreter : public IDebugInterface
{
   using instrfptr_t = void(*)(ThreadState*, Instruction);

public:
   void execute(ThreadState *state, uint32_t addr);

   virtual void addBreakpoint(uint32_t addr);
   virtual void removeBreakpoint(uint32_t addr);
   virtual void pause();
   virtual void resume();
   virtual void step();
   virtual ThreadState *getThreadState(uint32_t tid);

private:
   ThreadState *mActiveThread;
   volatile bool mPaused;
   volatile bool mStep;
   std::vector<uint32_t> mBreakpoints;
   std::mutex mDebugMutex;
   std::condition_variable mDebugCV;
   void enterBreakpoint();

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

   static std::vector<instrfptr_t> sInstructionMap;
};

extern Interpreter gInterpreter;
