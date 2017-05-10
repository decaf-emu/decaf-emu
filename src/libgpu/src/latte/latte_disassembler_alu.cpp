#include "latte/latte_disassembler.h"
#include "latte_decoders.h"

#include <common/bit_cast.h>
#include <common/decaf_assert.h>
#include <common/log.h>

namespace latte
{

namespace disassembler
{

static void
disassembleKcache(fmt::MemoryWriter &out,
                  uint32_t id,
                  SQ_CF_KCACHE_MODE mode,
                  uint32_t bank,
                  uint32_t addr)
{
   switch (mode) {
   case SQ_CF_KCACHE_MODE::NOP:
      break;
   case SQ_CF_KCACHE_MODE::LOCK_1:
      out
         << " KCACHE" << id << "("
         << "CB" << bank << ":"
         << (16 * addr)
         << "-"
         << (16 * addr + 15)
         << ")";
      break;
   case SQ_CF_KCACHE_MODE::LOCK_2:
      out
         << " KCACHE" << id << "("
         << "CB" << bank << ":"
         << (16 * addr)
         << "-"
         << (16 * addr + 31)
         << ")";
      break;
   case SQ_CF_KCACHE_MODE::LOCK_LOOP_INDEX:
      out
         << " KCACHE" << id << "("
         << "CB" << bank << ":"
         << "AL+" << (16 * addr)
         << "-"
         << "AL+" << (16 * addr + 31)
         << ")";
      break;
   }
}

static void
disassembleAluSource(fmt::MemoryWriter &out,
                     const latte::ControlFlowInst &parent,
                     size_t groupPC,
                     SQ_INDEX_MODE indexMode,
                     uint32_t sel,
                     SQ_REL rel,
                     SQ_CHAN chan,
                     uint32_t literalValue,
                     bool negate,
                     bool absolute)
{
   bool useChannel = true;

   if (negate) {
      out << "-";
   }

   if (absolute) {
      out << "|";
   }

   if (sel >= SQ_ALU_SRC::KCACHE_BANK0_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK0_LAST) {
      auto id = sel - SQ_ALU_SRC::KCACHE_BANK0_FIRST;
      out << "KC0[" << id << "]";
   } else if (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK1_LAST) {
      auto id = sel - SQ_ALU_SRC::KCACHE_BANK1_FIRST;
      out << "KC1[" << id << "]";
   } else if (sel >= SQ_ALU_SRC::REGISTER_FIRST && sel <= SQ_ALU_SRC::REGISTER_LAST) {
      out << "R" << (sel - SQ_ALU_SRC::REGISTER_FIRST);
   } else if (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST) {
      out << "C" << (sel - SQ_ALU_SRC::CONST_FILE_FIRST);
   } else {
      useChannel = false;

      switch (sel) {
      case SQ_ALU_SRC::LDS_OQ_A:
         out << "LDS_OQ_A";
         break;
      case SQ_ALU_SRC::LDS_OQ_B:
         out << "LDS_OQ_B";
         break;
      case SQ_ALU_SRC::LDS_OQ_A_POP:
         out << "LDS_OQ_A_POP";
         break;
      case SQ_ALU_SRC::LDS_OQ_B_POP:
         out << "LDS_OQ_B_POP";
         break;
      case SQ_ALU_SRC::LDS_DIRECT_A:
         out << "LDS_DIRECT_A";
         break;
      case SQ_ALU_SRC::LDS_DIRECT_B:
         out << "LDS_DIRECT_B";
         break;
      case SQ_ALU_SRC::TIME_HI:
         out << "TIME_HI";
         break;
      case SQ_ALU_SRC::TIME_LO:
         out << "TIME_LO";
         break;
      case SQ_ALU_SRC::MASK_HI:
         out << "MASK_HI";
         break;
      case SQ_ALU_SRC::MASK_LO:
         out << "MASK_LO";
         break;
      case SQ_ALU_SRC::HW_WAVE_ID:
         out << "HW_WAVE_ID";
         break;
      case SQ_ALU_SRC::SIMD_ID:
         out << "SIMD_ID";
         break;
      case SQ_ALU_SRC::SE_ID:
         out << "SE_ID";
         break;
      case SQ_ALU_SRC::HW_THREADGRP_ID:
         out << "HW_THREADGRP_ID";
         break;
      case SQ_ALU_SRC::WAVE_ID_IN_GRP:
         out << "WAVE_ID_IN_GRP";
         break;
      case SQ_ALU_SRC::NUM_THREADGRP_WAVES:
         out << "NUM_THREADGRP_WAVES";
         break;
      case SQ_ALU_SRC::HW_ALU_ODD:
         out << "HW_ALU_ODD";
         break;
      case SQ_ALU_SRC::LOOP_IDX:
         out << "AL";
         break;
      case SQ_ALU_SRC::PARAM_BASE_ADDR:
         out << "PARAM_BASE_ADDR";
         break;
      case SQ_ALU_SRC::NEW_PRIM_MASK:
         out << "NEW_PRIM_MASK";
         break;
      case SQ_ALU_SRC::PRIM_MASK_HI:
         out << "PRIM_MASK_HI";
         break;
      case SQ_ALU_SRC::PRIM_MASK_LO:
         out << "PRIM_MASK_LO";
         break;
      case SQ_ALU_SRC::IMM_1_DBL_L:
         out << "1.0_L";
         break;
      case SQ_ALU_SRC::IMM_1_DBL_M:
         out << "1.0_M";
         break;
      case SQ_ALU_SRC::IMM_0_5_DBL_L:
         out << "0.5_L";
         break;
      case SQ_ALU_SRC::IMM_0_5_DBL_M:
         out << "0.5_M";
         break;
      case SQ_ALU_SRC::IMM_0:
         out << "0.0f";
         break;
      case SQ_ALU_SRC::IMM_1:
         out << "1.0f";
         break;
      case SQ_ALU_SRC::IMM_1_INT:
         out << "1";
         break;
      case SQ_ALU_SRC::IMM_M_1_INT:
         out << "-1";
         break;
      case SQ_ALU_SRC::IMM_0_5:
         out << "0.5f";
         break;
      case SQ_ALU_SRC::LITERAL:
         out.write("(0x{:08X}, {})", literalValue, bit_cast<float>(literalValue));
         break;
      case SQ_ALU_SRC::PV:
         out << "PV" << (groupPC - 1);
         useChannel = true;
         break;
      case SQ_ALU_SRC::PS:
         out << "PS" << (groupPC - 1);
         break;
      default:
         out << "UNKNOWN";
      }
   }

   if (rel) {
      switch (indexMode) {
      case SQ_INDEX_MODE::AR_X:
         out << "[AR.x]";
         break;
      case SQ_INDEX_MODE::AR_Y:
         out << "[AR.y]";
         break;
      case SQ_INDEX_MODE::AR_Z:
         out << "[AR.z]";
         break;
      case SQ_INDEX_MODE::AR_W:
         out << "[AR.w]";
         break;
      case SQ_INDEX_MODE::LOOP:
         out << "[AL]";
         break;
      default:
         out << "[UNKNOWN]";
      }
   }

   if (useChannel) {
      switch (chan) {
      case SQ_CHAN::X:
         out << ".x";
         break;
      case SQ_CHAN::Y:
         out << ".y";
         break;
      case SQ_CHAN::Z:
         out << ".z";
         break;
      case SQ_CHAN::W:
         out << ".w";
         break;
      default:
         out << ".UNKNOWN";
         break;
      }
   }

   if (absolute) {
      out << "|";
   }
}

void
disassembleAluInstruction(fmt::MemoryWriter &out,
                          const ControlFlowInst &parent,
                          const AluInst &inst,
                          size_t groupPC,
                          SQ_CHAN unit,
                          const gsl::span<const uint32_t> &literals,
                          int namePad)
{
   std::string name;
   SQ_ALU_FLAGS flags;
   auto srcCount = 0u;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      name = getInstructionName(inst.op2.ALU_INST());
      flags = getInstructionFlags(inst.op2.ALU_INST());
      srcCount = getInstructionNumSrcs(inst.op2.ALU_INST());
   } else {
      name = getInstructionName(inst.op3.ALU_INST());
      flags = getInstructionFlags(inst.op3.ALU_INST());
      srcCount = getInstructionNumSrcs(inst.op3.ALU_INST());
   }

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2 && !inst.op2.UPDATE_EXECUTE_MASK()) {
      switch (inst.op2.OMOD()) {
      case SQ_ALU_OMOD::OFF:
         break;
      case SQ_ALU_OMOD::D2:
         name += "/2";
         break;
      case SQ_ALU_OMOD::M2:
         name += "*2";
         break;
      case SQ_ALU_OMOD::M4:
         name += "*4";
         break;
      default:
         decaf_abort(fmt::format("Unexpected OMOD {}", inst.op2.OMOD()));
      }
   }

   out << fmt::pad(name.c_str(), namePad, ' ') << ' ';

   auto writeMask = true;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      writeMask = inst.op2.WRITE_MASK();
   }

   if (!writeMask) {
      out << "____";
   } else {
      disassembleAluSource(out,
                           parent,
                           groupPC,
                           inst.word0.INDEX_MODE(),
                           inst.word1.DST_GPR(),
                           inst.word1.DST_REL(),
                           inst.word1.DST_CHAN(),
                           0,
                           false,
                           false);
   }

   if (srcCount > 0) {
      auto literal = 0u;
      auto abs = false;

      if (inst.word0.SRC0_SEL() == SQ_ALU_SRC::LITERAL) {
         literal = literals[inst.word0.SRC0_CHAN()];
      }

      if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         abs = !!inst.op2.SRC0_ABS();
      }

      out << ", ";
      disassembleAluSource(out,
                           parent,
                           groupPC,
                           inst.word0.INDEX_MODE(),
                           inst.word0.SRC0_SEL(),
                           inst.word0.SRC0_REL(),
                           inst.word0.SRC0_CHAN(),
                           literal,
                           inst.word0.SRC0_NEG(),
                           abs);
   }

   if (srcCount > 1) {
      auto literal = 0u;
      auto abs = false;

      if (inst.word0.SRC1_SEL() == SQ_ALU_SRC::LITERAL) {
         literal = literals[inst.word0.SRC1_CHAN()];
      }

      if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         abs = !!inst.op2.SRC1_ABS();
      }

      out << ", ";
      disassembleAluSource(out,
                           parent,
                           groupPC,
                           inst.word0.INDEX_MODE(),
                           inst.word0.SRC1_SEL(),
                           inst.word0.SRC1_REL(),
                           inst.word0.SRC1_CHAN(),
                           literal,
                           inst.word0.SRC1_NEG(),
                           abs);
   }

   if (srcCount > 2) {
      auto literal = 0u;

      if (inst.op3.SRC2_SEL() == SQ_ALU_SRC::LITERAL) {
         literal = literals[inst.op3.SRC2_CHAN()];
      }

      out << ", ";
      disassembleAluSource(out,
                           parent,
                           groupPC,
                           inst.word0.INDEX_MODE(),
                           inst.op3.SRC2_SEL(),
                           inst.op3.SRC2_REL(),
                           inst.op3.SRC2_CHAN(),
                           literal,
                           inst.op3.SRC2_NEG(),
                           false);
   }

   if (inst.word1.CLAMP()) {
      out << " CLAMP";
   }

   if (isTranscendentalOnly(flags)) {
      switch (static_cast<SQ_ALU_SCL_BANK_SWIZZLE>(inst.word1.BANK_SWIZZLE())) {
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_210:
         out << " SCL_210";
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_122:
         out << " SCL_122";
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_212:
         out << " SCL_212";
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_221:
         out << " SCL_221";
         break;
      default:
         decaf_abort(fmt::format("Unexpected BANK_SWIZZLE {}", inst.word1.BANK_SWIZZLE()));
      }
   } else {
      switch (inst.word1.BANK_SWIZZLE()) {
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_012:
         // This is default, no need to print
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_021:
         out << " VEC_021";
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_120:
         out << " VEC_120";
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_102:
         out << " VEC_102";
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_201:
         out << " VEC_201";
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_210:
         out << " VEC_210";
         break;
      default:
         decaf_abort(fmt::format("Unexpected BANK_SWIZZLE {}", inst.word1.BANK_SWIZZLE()));
      }
   }

   switch (inst.word0.PRED_SEL()) {
   case SQ_PRED_SEL::OFF:
      break;
   case SQ_PRED_SEL::ZERO:
      out << " PRED_SEL_ZERO";
      break;
   case SQ_PRED_SEL::ONE:
      out << " PRED_SEL_ONE";
      break;
   default:
      decaf_abort(fmt::format("Unexpected PRED_SEL {}", inst.word0.PRED_SEL()));
   }

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      if (inst.op2.UPDATE_EXECUTE_MASK()) {
         out << " UPDATE_EXEC_MASK";

         switch (inst.op2.EXECUTE_MASK_OP()) {
         case SQ_ALU_EXECUTE_MASK_OP::DEACTIVATE:
            out << "(DEACTIVATE)";
            break;
         case SQ_ALU_EXECUTE_MASK_OP::BREAK:
            out << "(BREAK)";
            break;
         case SQ_ALU_EXECUTE_MASK_OP::CONTINUE:
            out << "(CONTINUE)";
            break;
         case SQ_ALU_EXECUTE_MASK_OP::KILL:
            out << "(KILL)";
            break;
         default:
            decaf_abort(fmt::format("Unexpected EXECUTE_MASK_OP {}", inst.op2.EXECUTE_MASK_OP()));
         }
      }

      if (inst.op2.UPDATE_PRED()) {
         out << " UPDATE_PRED";
      }
   }
}

static void
disassembleAluClause(State &state, const latte::ControlFlowInst &parent, uint32_t addr, uint32_t slots)
{
   static char unitName[5] = { 'x', 'y', 'z', 'w', 't' };
   auto result = true;
   auto clause = reinterpret_cast<const AluInst *>(state.binary.data() + 8 * addr);

   for (size_t slot = 0u; slot < slots; ) {
      auto units = AluGroupUnits { };
      auto group = AluGroup { clause + slot };

      state.out
         << '\n'
         << state.indent
         << fmt::pad(state.groupPC, 3, ' ');

      for (auto j = 0u; j < group.instructions.size(); ++j) {
         auto &inst = group.instructions[j];
         auto unit = units.addInstructionUnit(inst);
         const char *name = nullptr;
         auto srcCount = 0u;

         if (j > 0) {
            state.out << state.indent << "   ";
         }

         state.out
            << ' '
            << unitName[unit]
            << ": ";

         disassembleAluInstruction(state.out, parent, inst, state.groupPC, unit, group.literals, 15);
         state.out << "\n";
      }

      slot = group.getNextSlot(slot);
      state.groupPC++;
   }
}

void
disassembleCfALUInstruction(fmt::MemoryWriter &out,
                            const ControlFlowInst &inst)
{
   auto name = getInstructionName(inst.alu.word1.CF_INST());
   auto addr = inst.alu.word0.ADDR();
   auto count = inst.alu.word1.COUNT() + 1;

   out
      << name
      << ": ADDR(" << addr << ")"
      << " CNT(" << count << ")";


   if (!inst.word1.BARRIER()) {
      out << " NO_BARRIER";
   }

   if (inst.word1.WHOLE_QUAD_MODE()) {
      out << " WHOLE_QUAD";
   }

   disassembleKcache(out, 0, inst.alu.word0.KCACHE_MODE0(), inst.alu.word0.KCACHE_BANK0(), inst.alu.word1.KCACHE_ADDR0());
   disassembleKcache(out, 1, inst.alu.word1.KCACHE_MODE1(), inst.alu.word0.KCACHE_BANK1(), inst.alu.word1.KCACHE_ADDR1());
}

void
disassembleControlFlowALU(State &state, const ControlFlowInst &inst)
{
   auto addr = inst.alu.word0.ADDR();
   auto count = inst.alu.word1.COUNT() + 1;

   state.out.write("{}{:02} ", state.indent, state.cfPC);
   disassembleCfALUInstruction(state.out, inst);

   increaseIndent(state);
   disassembleAluClause(state, inst, addr, count);
   decreaseIndent(state);
}

} // namespace disassembler

} // namespace latte
