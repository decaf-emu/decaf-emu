#include "glsl2_alu.h"
#include "glsl2_translate.h"

using namespace latte;

namespace glsl2
{

static const AluInst *
findWriteInstruction(const std::array<AluInst, 4> &group)
{
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].op2.WRITE_MASK()) {
         return &group[i];
      }
   }

   return nullptr;
}

static void
DOT4(State &state, const ControlFlowInst &cf, const std::array<AluInst, 4> &group)
{
   auto hasWriteMask = false;
   auto writeUnit = 0u;
   insertLineStart(state);

   // Find which, if any, instruction has a write mask set
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].op2.WRITE_MASK()) {
         hasWriteMask = true;
         writeUnit = i;
         insertDestBegin(state.out, cf, group[i], static_cast<SQ_CHAN>(SQ_CHAN_X));
         break;
      }
   }

   // If no instruction in the group has a dest, then we must still write to PV.x
   if (!hasWriteMask) {
      insertPreviousValueUpdate(state.out, SQ_CHAN_X);
   }

   state.out << "dot(";
   insertSource0Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   state.out << ", ";
   insertSource1Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   state.out << ")";

   if (hasWriteMask) {
      insertDestEnd(state.out, cf, group[writeUnit]);
   }

   state.out << ';';
   insertLineEnd(state);
}

void
registerOP2ReductionFunctions()
{
   registerInstruction(latte::SQ_OP2_INST_DOT4, DOT4);
   registerInstruction(latte::SQ_OP2_INST_DOT4_IEEE, DOT4);
}

void
registerOP3ReductionFunctions()
{

}

} // namespace glsl2
