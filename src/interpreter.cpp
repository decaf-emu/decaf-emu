#include "disassembler.h"
#include "interpreter.h"
#include "instructiondata.h"
#include "memory.h"
#include "log.h"

Interpreter
gInterpreter;

static std::vector<instrfptr_t>
sInstructionMap;

void Interpreter::RegisterFunctions()
{
   static bool didInit = false;

   if (!didInit) {
      // Reserve instruction map
      sInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);

      // Register instruction handlers
      registerBranchInstructions();
      registerConditionInstructions();
      registerFloatInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerPairedInstructions();
      registerSystemInstructions();

      didInit = true;
   }
}

void Interpreter::registerInstruction(InstructionID id, instrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

void Interpreter::setJitEnabled(bool val) {
   mJitEnabled = val;
}

// Address used to signify a return to emulator-land.
const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

void
Interpreter::execute(ThreadState *state)
{
   bool hasJumped = false;

   while (state->nia != CALLBACK_ADDR) {
      // JIT Attempt!
      if (mJitEnabled) {
         if (state->nia != state->cia + 4) {
            // We jumped, try to enter JIT
            JitCode jitFn = mJitManager.get(state->nia);
            if (jitFn) {
               auto newNia = mJitManager.execute(state, jitFn);
               state->cia = 0;
               state->nia = newNia;
               continue;
            }
         }
      }

      // Interpreter Loop!
      state->cia = state->nia;
      state->nia = state->cia + 4;

      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);

      if (!data) {
         xLog() << Log::hex(state->cia) << " " << Log::hex(instr.value);
      }

      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
      auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), state->cia);

      if (bpitr != mBreakpoints.end()) {
         xLog() << "Hit breakpoint!";
      }

      if (!fptr) {
         xLog() << "Unimplemented interpreter instruction!";
      } else {
         fptr(state, instr);
      }
   }
}

void
Interpreter::execute(ThreadState *state, uint32_t addr)
{
   auto saveLR = state->lr;
   auto saveCIA = state->cia;
   auto saveNIA = state->nia;

   state->lr = CALLBACK_ADDR;
   state->cia = 0;
   state->nia = addr;

   execute(state);

   state->lr = saveLR;
   state->cia = saveCIA;
   state->nia = saveNIA;
}

void Interpreter::addBreakpoint(uint32_t addr)
{
   mBreakpoints.push_back(addr);
}
