#include "latte_disassembler.h"

namespace latte
{

namespace disassembler
{

bool
disassembleVTX(State &state, shadir::CfInstruction *inst)
{
   state.out
      << " ADDR(" << inst->addr << ")"
      << " CNT(" << inst->clause.size() << ")";

   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   disassembleCondition(state, inst);

   state.out << '\n';
   increaseIndent(state);

   for (auto &child : inst->clause) {
      // TODO: Disassemble VTX clauses
   }

   decreaseIndent(state);
   return true;
}

} // namespace disassembler

} // namespace latte
