#include "latte/latte_disassembler_state.h"

#include <iterator>

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
disassembleExpInstruction(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   auto id = inst.exp.word1.CF_INST();
   auto name = getInstructionName(id);
   fmt::format_to(std::back_inserter(out), "{}:", name);

   auto type = inst.exp.word0.TYPE();
   auto memExpType = static_cast<SQ_MEM_EXPORT_TYPE>(inst.exp.word0.TYPE());
   auto arrayBase = inst.exp.word0.ARRAY_BASE();

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      switch (type) {
      case SQ_EXPORT_TYPE::PIXEL:
         fmt::format_to(std::back_inserter(out), " PIX{}", arrayBase);
         break;
      case SQ_EXPORT_TYPE::POS:
         fmt::format_to(std::back_inserter(out), " POS{}", arrayBase - 60);
         break;
      case SQ_EXPORT_TYPE::PARAM:
         fmt::format_to(std::back_inserter(out), " PARAM{}", arrayBase);
         break;
      default:
         fmt::format_to(std::back_inserter(out), " INVALID{}", arrayBase);
         break;
      }
   } else if (id >= SQ_CF_INST_MEM_STREAM0 && id <= SQ_CF_INST_MEM_STREAM3
              && (memExpType == SQ_MEM_EXPORT_TYPE::READ || memExpType == SQ_MEM_EXPORT_TYPE::READ_IND)) {
      fmt::format_to(std::back_inserter(out), " INVALID_READ");
   } else {
      if (memExpType == SQ_MEM_EXPORT_TYPE::WRITE || memExpType == SQ_MEM_EXPORT_TYPE::WRITE_IND) {
         fmt::format_to(std::back_inserter(out), " WRITE(");
      } else {
         fmt::format_to(std::back_inserter(out), " READ(");
      }

      if (id == SQ_CF_INST_MEM_SCRATCH || id == SQ_CF_INST_MEM_REDUCTION) {
         fmt::format_to(std::back_inserter(out), "{}", arrayBase * 16);
      } else {
         fmt::format_to(std::back_inserter(out), "{}", arrayBase * 4);
      }

      if (memExpType == SQ_MEM_EXPORT_TYPE::WRITE_IND || memExpType == SQ_MEM_EXPORT_TYPE::READ_IND) {
         fmt::format_to(std::back_inserter(out), " + R{}", inst.exp.word0.INDEX_GPR());
      }

      fmt::format_to(std::back_inserter(out), ")");

   }

   fmt::format_to(std::back_inserter(out), ", ");

   if (inst.exp.word0.RW_REL() == SQ_REL::REL) {
      fmt::format_to(std::back_inserter(out), "R[AL + {}]", inst.exp.word0.RW_GPR());
   } else {
      fmt::format_to(std::back_inserter(out), "R{}", inst.exp.word0.RW_GPR());
   }

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      fmt::format_to(std::back_inserter(out), ".{}{}{}{}",
         disassembleDestMask(inst.exp.swiz.SEL_X()),
         disassembleDestMask(inst.exp.swiz.SEL_Y()),
         disassembleDestMask(inst.exp.swiz.SEL_Z()),
         disassembleDestMask(inst.exp.swiz.SEL_W()));
   } else {
      fmt::format_to(std::back_inserter(out), ".{}{}{}{}",
         (inst.exp.buf.COMP_MASK() & (1 << 0) ? 'x' : '_'),
         (inst.exp.buf.COMP_MASK() & (1 << 1) ? 'y' : '_'),
         (inst.exp.buf.COMP_MASK() & (1 << 2) ? 'z' : '_'),
         (inst.exp.buf.COMP_MASK() & (1 << 3) ? 'w' : '_'));

      fmt::format_to(std::back_inserter(out), " ARRAY_SIZE({})", inst.exp.buf.ARRAY_SIZE());
   }

   if (inst.exp.word0.ELEM_SIZE()) {
      fmt::format_to(std::back_inserter(out), " ELEM_SIZE({})", inst.exp.word0.ELEM_SIZE());
   }

   if (inst.exp.word1.BURST_COUNT()) {
      fmt::format_to(std::back_inserter(out), " BURSTCNT({})", inst.exp.word1.BURST_COUNT());
   }

   if (!inst.exp.word1.BARRIER()) {
      fmt::format_to(std::back_inserter(out), " NO_BARRIER");
   }

   if (inst.exp.word1.WHOLE_QUAD_MODE()) {
      fmt::format_to(std::back_inserter(out), " WHOLE_QUAD");
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      fmt::format_to(std::back_inserter(out), " VALID_PIX");
   }
}

void
disassembleExport(State &state, const ControlFlowInst &inst)
{
   fmt::format_to(std::back_inserter(state.out), "{}{:02} ", state.indent, state.cfPC);
   disassembleExpInstruction(state.out, inst);
   fmt::format_to(std::back_inserter(state.out), "\n");
}

} // namespace disassembler

} // namespace latte
