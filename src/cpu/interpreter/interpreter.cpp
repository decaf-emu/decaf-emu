#include "cpu/instructiondata.h"
#include "cpu/interpreter/interpreter.h"
#include "cpu/interpreter/interpreter_insreg.h"
#include "debugcontrol.h"
#include "mem/mem.h"
#include "processor.h"
#include "trace.h"
#include "utils/log.h"
#include "../cpu_internal.h"

namespace cpu
{
namespace interpreter
{

   static std::vector<instrfptr_t>
   sInstructionMap;

   void initialise()
   {
      sInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);

      // Register instruction handlers
      registerBranchInstructions();
      registerConditionInstructions();
      registerFloatInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerPairedInstructions();
      registerSystemInstructions();
   }

   instrfptr_t getInstructionHandler(InstructionID id)
   {
      auto instrId = static_cast<size_t>(id);
      if (instrId >= sInstructionMap.size()) {
         return nullptr;
      }
      return sInstructionMap[instrId];
   }

   void registerInstruction(InstructionID id, instrfptr_t fptr)
   {
      sInstructionMap[static_cast<size_t>(id)] = fptr;
   }

   bool hasInstruction(InstructionID instrId)
   {
      return getInstructionHandler(instrId) != nullptr;
   }

   void execute(ThreadState *state)
   {
      while (state->nia != cpu::CALLBACK_ADDR) {
         // TankTankTank decryptor fn
         //forceJit = state->nia >= 0x0250B648 && state->nia < 0x0250B8B8;

         if (state->core->interrupt.load()) {
            cpu::gInterruptHandler(state->core, state);
         }

         // Interpreter Loop!
         state->cia = state->nia;
         state->nia = state->cia + 4;

         gDebugControl.maybeBreak(state->cia, state, gProcessor.getCoreID());

         auto instr = mem::read<Instruction>(state->cia);
         auto data = gInstructionTable.decode(instr);

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

}
}