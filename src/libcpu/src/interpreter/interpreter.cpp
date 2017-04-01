#include <common/decaf_assert.h>
#include <common/log.h>
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter.h"
#include "interpreter_insreg.h"
#include "mem.h"
#include "trace.h"
#include <cfenv>

namespace cpu
{

namespace interpreter
{

static std::vector<instrfptr_t>
sInstructionMap;

void
initialise()
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

instrfptr_t
getInstructionHandler(espresso::InstructionID id)
{
   auto instrId = static_cast<size_t>(id);

   if (instrId >= sInstructionMap.size()) {
      return nullptr;
   }

   return sInstructionMap[instrId];
}

void
registerInstruction(espresso::InstructionID id, instrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool
hasInstruction(espresso::InstructionID id)
{
   return getInstructionHandler(id) != nullptr;
}

Core *
step_one(Core *core)
{
   // This is volatile because otherwise we appear to encounter
   //  some kind of compiler optimization error, where the value
   //  in cia is not correctly persisted.
   uint32_t cia = core->nia;
   core->nia = cia + 4;

   // For debugging purposes.
   core->cia = cia;

   auto instr = mem::read<espresso::Instruction>(cia);
   auto data = espresso::decodeInstruction(instr);

   if (!data) {
      gLog->error("Could not decode instruction at {:08x} = {:08x}", cia, instr.value);
   }
   decaf_check(data);

   auto trace = traceInstructionStart(instr, data, core);
   auto fptr = sInstructionMap[static_cast<size_t>(data->id)];

   if (!fptr) {
      gLog->error("Unimplemented interpreter instruction {}", data->name);
   }
   decaf_check(fptr);

   fptr(core, instr);

   if (data->id == InstructionID::kc) {
      // If this is a KC, there is the potential that we are running on a
      //  different core now.  Lets make sure that we are using the right one.
      core = this_core::state();
   }

   decaf_check(core->cia == cia);
   traceInstructionEnd(trace, instr, data, core);

   return core;
}

void
resume()
{
   // Before we resume, we need to update our states!
   this_core::updateRoundingMode();
   std::feclearexcept(FE_ALL_EXCEPT);

   auto core = cpu::this_core::state();
   while (core->nia != cpu::CALLBACK_ADDR) {
      this_core::checkInterrupts();
      core = step_one(this_core::state());
   }
}

} // namespace interpreter

} // namespace cpu
