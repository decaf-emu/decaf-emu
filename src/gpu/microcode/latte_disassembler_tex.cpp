#include "latte_disassembler.h"

namespace latte
{

namespace disassembler
{

bool
disassembleTEX(State &state, shadir::CfInstruction *inst)
{
   state.out
      << " ADDR(" << inst->addr << ")"
      << " CNT(" << inst->clause.size() << ")";

   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   disassembleCondition(state, inst);

   if (inst->wholeQuadMode) {
      state.out << " WHOLE_QUAD";
   }

   if (inst->validPixelMode) {
      state.out << " VALID_PIX";
   }

   state.out << '\n';
   increaseIndent(state);

   for (auto &child : inst->clause) {
      // TODO: Disassemble TEX clauses
   }

   decreaseIndent(state);
   return true;
}

} // namespace disassembler

} // namespace latte
