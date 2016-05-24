#include "interpreter.h"
#include "interpreter_insreg.h"
#include "cpu/espresso/espresso_instructionset.h"
#include "../trace.h"
#include "../cpu_internal.h"
#include "debugcontrol.h"
#include "mem/mem.h"
#include "utils/log.h"
#include "processor.h"

namespace cpu
{

namespace interpreter
{

static std::vector<instrfptr_t>
sInstructionMap;

void initialise()
{
   sInstructionMap.resize(static_cast<size_t>(espresso::InstructionID::InstructionCount), nullptr);

   // Register instruction handlers
   registerBranchInstructions();
   registerConditionInstructions();
   registerFloatInstructions();
   registerIntegerInstructions();
   registerLoadStoreInstructions();
   registerPairedInstructions();
   registerSystemInstructions();
}

instrfptr_t getInstructionHandler(espresso::InstructionID id)
{
   auto instrId = static_cast<size_t>(id);

   if (instrId >= sInstructionMap.size()) {
      return nullptr;
   }

   return sInstructionMap[instrId];
}

void registerInstruction(espresso::InstructionID id, instrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool hasInstruction(espresso::InstructionID id)
{
   return getInstructionHandler(id) != nullptr;
}

void execute(ThreadState *state)
{
   while (state->nia != cpu::CALLBACK_ADDR) {
      if (state->core->interrupt.load()) {
         cpu::gInterruptHandler(state->core, state);
      }

      // Interpreter Loop!
      state->cia = state->nia;
      state->nia = state->cia + 4;

      gDebugControl.maybeBreak(state->cia, state, gProcessor.getCoreID());

      auto instr = mem::read<espresso::Instruction>(state->cia);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         gLog->error("Could not decode instruction at {:08x} = {:08x}", state->cia, instr.value);
      }
      assert(data);

      auto trace = traceInstructionStart(instr, data, state);
      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];

      if (!fptr) {
         gLog->error("Unimplemented interpreter instruction {}", data->name);
      }
      assert(fptr);

      fptr(state, instr);
      traceInstructionEnd(trace, instr, data, state);
   }
}


void executeSub(ThreadState *state)
{
   auto lr = state->lr;
   state->lr = CALLBACK_ADDR;

   execute(state);

   state->lr = lr;
}

} // namespace interpreter

} // namespace cpu
