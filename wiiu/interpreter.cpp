#include "disassembler.h"
#include "interpreter.h"
#include "instructiondata.h"
#include "memory.h"

void Interpreter::initialise()
{
   // Setup instruction map
   mInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);
   registerBranchInstructions();
   registerConditionInstructions();
   registerFloatInstructions();
   registerIntegerInstructions();
   registerLoadStoreInstructions();
   registerSystemInstructions();
}

void Interpreter::registerInstruction(InstructionID id, fptr_t fptr)
{
   mInstructionMap[static_cast<size_t>(id)] = fptr;
}

void Interpreter::execute(ThreadState *state)
{
   mActiveThread = state;
   mStep = false;
   mPaused = false;

   for (unsigned i = 0; i < 1024 * 1024; ++i) {
      Disassembly dis;
      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);
      auto fptr = mInstructionMap[static_cast<size_t>(data->id)];

      dis.address = state->cia;
      gDisassembler.disassemble(instr, dis);
      xLog() << Log::hex(state->cia) << " " << dis.text;

      auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), state->cia);

      if (mStep || bpitr != mBreakpoints.end()) {
         enterBreakpoint();
      }

      if (!fptr) {
         xLog() << "Unimplemented interpreter instruction!";
      } else {
         fptr(state, instr);
      }

      state->cia = state->nia;
      state->nia = state->cia + 4;
   }
}

ThreadState *Interpreter::getThreadState(uint32_t tid)
{
   return mActiveThread;
}

void Interpreter::resume()
{
   mStep = false;
   mDebugCV.notify_one();
}

void Interpreter::pause()
{
   mStep = true;
}

void Interpreter::step()
{
   mStep = true;
   resume();
}

void Interpreter::removeBreakpoint(uint32_t addr)
{
   auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), addr);

   if (bpitr != mBreakpoints.end()) {
      mBreakpoints.erase(bpitr);
   }
}

void Interpreter::addBreakpoint(uint32_t addr)
{
   mBreakpoints.push_back(addr);
}

void Interpreter::enterBreakpoint()
{
   std::unique_lock<std::mutex> lock(mDebugMutex);
   mPaused = true;
   mDebugCV.wait(lock);
   mPaused = false;
}
