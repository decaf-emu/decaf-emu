#include "latte_disassembler.h"

namespace latte
{

namespace disassembler
{

char
disassembleDestMask(SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL_X:
      return 'x';
   case SQ_SEL_Y:
      return 'y';
   case SQ_SEL_Z:
      return 'z';
   case SQ_SEL_W:
      return 'w';
   case SQ_SEL_0:
      return '0';
   case SQ_SEL_1:
      return '1';
   case SQ_SEL_MASK:
      return '_';
   default:
      return '?';
   }
}

void
disassembleExpInstruction(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   auto id = inst.exp.word1.CF_INST();
   auto name = getInstructionName(id);
   out << name;

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      auto arrayBase = inst.exp.word0.ARRAY_BASE();

      switch (inst.exp.word0.TYPE()) {
      case SQ_EXPORT_PIXEL:
         out << " PIXEL" << arrayBase;
         break;
      case SQ_EXPORT_POS:
         out << " POS" << (arrayBase - 60);
         break;
      case SQ_EXPORT_PARAM:
         out << " PARAM" << arrayBase;
         break;
      }

      out << ", ";

      if (inst.exp.word0.RW_REL() == SQ_RELATIVE) {
         out << "R[AL + " << inst.exp.word0.RW_GPR() << "]";
      } else {
         out << "R" << inst.exp.word0.RW_GPR();
      }

      out
         << '.'
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_X())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_Y())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_Z())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_W());
   } else {
      // TODO: Disassemble export MEM_*
      out << " MEM_UNKNOWN_FORMAT";
   }

   if (inst.exp.word1.BURST_COUNT()) {
      out << " BURSTCNT(" << inst.exp.word1.BURST_COUNT() << ")";
   }

   if (!inst.exp.word1.BARRIER()) {
      out << " NO_BARRIER";
   }

   if (inst.exp.word1.WHOLE_QUAD_MODE()) {
      out << " WHOLE_QUAD";
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      out << " VALID_PIX";
   }
}

void
disassembleExport(State &state, const ControlFlowInst &inst)
{
   auto id = inst.exp.word1.CF_INST();
   auto name = getInstructionName(id);
   state.out.write("{}{:02} ", state.indent, state.cfPC);
   disassembleExpInstruction(state.out, inst);
   state.out << "\n";
}

} // namespace disassembler

} // namespace latte
