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
   using fptr_t = void(*)(ThreadState*, Instruction);

public:
   void initialise();
   void execute(ThreadState *state);

   virtual void addBreakpoint(uint32_t addr);
   virtual void removeBreakpoint(uint32_t addr);
   virtual void pause();
   virtual void resume();
   virtual void step();
   virtual ThreadState *getThreadState(uint32_t tid);

private:
   void registerInstruction(InstructionID id, fptr_t fptr);
   void registerBranchInstructions();
   void registerConditionInstructions();
   void registerFloatInstructions();
   void registerIntegerInstructions();
   void registerLoadStoreInstructions();
   void registerPairedInstructions();
   void registerSystemInstructions();

   std::vector<fptr_t> mInstructionMap;

   ThreadState *mActiveThread;
   volatile bool mPaused;
   volatile bool mStep;
   std::vector<uint32_t> mBreakpoints;
   std::mutex mDebugMutex;
   std::condition_variable mDebugCV;
   void enterBreakpoint();
};
