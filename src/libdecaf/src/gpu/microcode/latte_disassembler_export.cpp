#include "latte_disassembler.h"

namespace latte
{

namespace disassembler
{

char
disassembleDestMask(SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL::SEL_X:
      return 'x';
   case SQ_SEL::SEL_Y:
      return 'y';
   case SQ_SEL::SEL_Z:
      return 'z';
   case SQ_SEL::SEL_W:
      return 'w';
   case SQ_SEL::SEL_0:
      return '0';
   case SQ_SEL::SEL_1:
      return '1';
   case SQ_SEL::SEL_MASK:
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

   auto type = inst.exp.word0.TYPE();
   auto arrayBase = inst.exp.word0.ARRAY_BASE();

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      switch (type) {
      case SQ_EXPORT_TYPE::PIXEL:
         out << " PIXEL" << arrayBase;
         break;
      case SQ_EXPORT_TYPE::POS:
         out << " POS" << (arrayBase - 60);
         break;
      case SQ_EXPORT_TYPE::PARAM:
         out << " PARAM" << arrayBase;
         break;
      default:
         out << " INVALID" << arrayBase;
         break;
      }
   } else if (id >= SQ_CF_INST_MEM_STREAM0 && id <= SQ_CF_INST_MEM_STREAM3
              && (type == SQ_MEM_EXPORT_TYPE::READ || type == SQ_MEM_EXPORT_TYPE::READ_IND)) {
      out << " INVALID_READ";
   } else {
      if (type == SQ_MEM_EXPORT_TYPE::WRITE || type == SQ_MEM_EXPORT_TYPE::WRITE_IND) {
         out << " WRITE(";
      } else {
         out << " READ(";
      }

      if (id == SQ_CF_INST_MEM_SCRATCH || id == SQ_CF_INST_MEM_REDUCTION) {
         out << (arrayBase * 16);
      } else {
         out << (arrayBase * 4);
      }

      if (type == SQ_MEM_EXPORT_TYPE::WRITE_IND || type == SQ_MEM_EXPORT_TYPE::READ_IND) {
         out << " + R" << inst.exp.word0.INDEX_GPR();
      }

      out << ")";

   }

   out << ", ";

   if (inst.exp.word0.RW_REL() == SQ_REL::REL) {
      out << "R[AL + " << inst.exp.word0.RW_GPR() << "]";
   } else {
      out << "R" << inst.exp.word0.RW_GPR();
   }

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      out
         << '.'
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_X())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_Y())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_Z())
         << disassembleDestMask(inst.exp.swiz.SRC_SEL_W());
   } else {
      out
         << '.'
         << (inst.exp.buf.COMP_MASK() & (1 << 0) ? 'x' : '_')
         << (inst.exp.buf.COMP_MASK() & (1 << 1) ? 'y' : '_')
         << (inst.exp.buf.COMP_MASK() & (1 << 2) ? 'z' : '_')
         << (inst.exp.buf.COMP_MASK() & (1 << 3) ? 'w' : '_');

      out << " ARRAY_SIZE(" << inst.exp.buf.ARRAY_SIZE() << ")";
   }

   if (inst.exp.word0.ELEM_SIZE()) {
      out << " ELEM_SIZE(" << inst.exp.word0.ELEM_SIZE() << ")";
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
