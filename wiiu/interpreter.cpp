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
   for (unsigned i = 0; i < 32; ++i) {
      Disassembly dis;
      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);
      auto fptr = mInstructionMap[static_cast<size_t>(data->id)];

      dis.address = state->cia;
      gDisassembler.disassemble(instr, dis);
      xLog() << Log::hex(state->cia) << " " << dis.text;

      if (!fptr) {
         xLog() << "Unimplemented interpreter instruction!";
      } else {
         fptr(state, instr);
      }

      state->cia = state->nia;
      state->nia = state->cia + 4;
   }
}
