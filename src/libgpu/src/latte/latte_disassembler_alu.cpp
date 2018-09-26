#include "latte/latte_disassembler.h"
#include "latte_decoders.h"

#include <common/bit_cast.h>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

namespace latte
{

namespace disassembler
{

static void
disassembleKcache(fmt::memory_buffer &out,
                  uint32_t id,
                  SQ_CF_KCACHE_MODE mode,
                  uint32_t bank,
                  uint32_t addr)
{
   switch (mode) {
   case SQ_CF_KCACHE_MODE::NOP:
      break;
   case SQ_CF_KCACHE_MODE::LOCK_1:
      fmt::format_to(out, " KCACHE{}(CB{}:{}-{})",
                     id, bank, 16 * addr, 16 * addr + 15);
      break;
   case SQ_CF_KCACHE_MODE::LOCK_2:
      fmt::format_to(out, " KCACHE{}(CB{}:{}-{})",
                     id, bank, 16 * addr, 16 * addr + 31);
      break;
   case SQ_CF_KCACHE_MODE::LOCK_LOOP_INDEX:
      fmt::format_to(out, " KCACHE{}(CB{}:AL+{}-AL+{})",
                     id, bank, 16 * addr, 16 * addr + 31);
      break;
   }
}

static void
disassembleAluSource(fmt::memory_buffer &out,
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
      fmt::format_to(out, "-");
   }

   if (absolute) {
      fmt::format_to(out, "|");
   }

   if (sel >= SQ_ALU_SRC::KCACHE_BANK0_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK0_LAST) {
      auto id = sel - SQ_ALU_SRC::KCACHE_BANK0_FIRST;
      fmt::format_to(out, "KC0[{}]", id);
   } else if (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK1_LAST) {
      auto id = sel - SQ_ALU_SRC::KCACHE_BANK1_FIRST;
      fmt::format_to(out, "KC1[{}]", id);
   } else if (sel >= SQ_ALU_SRC::REGISTER_FIRST && sel <= SQ_ALU_SRC::REGISTER_LAST) {
      fmt::format_to(out, "R{}", sel - SQ_ALU_SRC::REGISTER_FIRST);
   } else if (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST) {
      fmt::format_to(out, "C{}", sel - SQ_ALU_SRC::CONST_FILE_FIRST);
   } else {
      useChannel = false;

      switch (sel) {
      case SQ_ALU_SRC::LDS_OQ_A:
         fmt::format_to(out, "LDS_OQ_A");
         break;
      case SQ_ALU_SRC::LDS_OQ_B:
         fmt::format_to(out, "LDS_OQ_B");
         break;
      case SQ_ALU_SRC::LDS_OQ_A_POP:
         fmt::format_to(out, "LDS_OQ_A_POP");
         break;
      case SQ_ALU_SRC::LDS_OQ_B_POP:
         fmt::format_to(out, "LDS_OQ_B_POP");
         break;
      case SQ_ALU_SRC::LDS_DIRECT_A:
         fmt::format_to(out, "LDS_DIRECT_A");
         break;
      case SQ_ALU_SRC::LDS_DIRECT_B:
         fmt::format_to(out, "LDS_DIRECT_B");
         break;
      case SQ_ALU_SRC::TIME_HI:
         fmt::format_to(out, "TIME_HI");
         break;
      case SQ_ALU_SRC::TIME_LO:
         fmt::format_to(out, "TIME_LO");
         break;
      case SQ_ALU_SRC::MASK_HI:
         fmt::format_to(out, "MASK_HI");
         break;
      case SQ_ALU_SRC::MASK_LO:
         fmt::format_to(out, "MASK_LO");
         break;
      case SQ_ALU_SRC::HW_WAVE_ID:
         fmt::format_to(out, "HW_WAVE_ID");
         break;
      case SQ_ALU_SRC::SIMD_ID:
         fmt::format_to(out, "SIMD_ID");
         break;
      case SQ_ALU_SRC::SE_ID:
         fmt::format_to(out, "SE_ID");
         break;
      case SQ_ALU_SRC::HW_THREADGRP_ID:
         fmt::format_to(out, "HW_THREADGRP_ID");
         break;
      case SQ_ALU_SRC::WAVE_ID_IN_GRP:
         fmt::format_to(out, "WAVE_ID_IN_GRP");
         break;
      case SQ_ALU_SRC::NUM_THREADGRP_WAVES:
         fmt::format_to(out, "NUM_THREADGRP_WAVES");
         break;
      case SQ_ALU_SRC::HW_ALU_ODD:
         fmt::format_to(out, "HW_ALU_ODD");
         break;
      case SQ_ALU_SRC::LOOP_IDX:
         fmt::format_to(out, "AL");
         break;
      case SQ_ALU_SRC::PARAM_BASE_ADDR:
         fmt::format_to(out, "PARAM_BASE_ADDR");
         break;
      case SQ_ALU_SRC::NEW_PRIM_MASK:
         fmt::format_to(out, "NEW_PRIM_MASK");
         break;
      case SQ_ALU_SRC::PRIM_MASK_HI:
         fmt::format_to(out, "PRIM_MASK_HI");
         break;
      case SQ_ALU_SRC::PRIM_MASK_LO:
         fmt::format_to(out, "PRIM_MASK_LO");
         break;
      case SQ_ALU_SRC::IMM_1_DBL_L:
         fmt::format_to(out, "1.0_L");
         break;
      case SQ_ALU_SRC::IMM_1_DBL_M:
         fmt::format_to(out, "1.0_M");
         break;
      case SQ_ALU_SRC::IMM_0_5_DBL_L:
         fmt::format_to(out, "0.5_L");
         break;
      case SQ_ALU_SRC::IMM_0_5_DBL_M:
         fmt::format_to(out, "0.5_M");
         break;
      case SQ_ALU_SRC::IMM_0:
         fmt::format_to(out, "0.0f");
         break;
      case SQ_ALU_SRC::IMM_1:
         fmt::format_to(out, "1.0f");
         break;
      case SQ_ALU_SRC::IMM_1_INT:
         fmt::format_to(out, "1");
         break;
      case SQ_ALU_SRC::IMM_M_1_INT:
         fmt::format_to(out, "-1");
         break;
      case SQ_ALU_SRC::IMM_0_5:
         fmt::format_to(out, "0.5f");
         break;
      case SQ_ALU_SRC::LITERAL:
         fmt::format_to(out, "(0x{:08X}, {})", literalValue, bit_cast<float>(literalValue));
         break;
      case SQ_ALU_SRC::PV:
         fmt::format_to(out, "PV{}", groupPC - 1);
         useChannel = true;
         break;
      case SQ_ALU_SRC::PS:
         fmt::format_to(out, "PS{}", groupPC - 1);
         break;
      default:
         fmt::format_to(out, "UNKNOWN");
      }
   }

   if (rel) {
      switch (indexMode) {
      case SQ_INDEX_MODE::AR_X:
         fmt::format_to(out, "[AR.x]");
         break;
      case SQ_INDEX_MODE::AR_Y:
         fmt::format_to(out, "[AR.y]");
         break;
      case SQ_INDEX_MODE::AR_Z:
         fmt::format_to(out, "[AR.z]");
         break;
      case SQ_INDEX_MODE::AR_W:
         fmt::format_to(out, "[AR.w]");
         break;
      case SQ_INDEX_MODE::LOOP:
         fmt::format_to(out, "[AL]");
         break;
      default:
         fmt::format_to(out, "[UNKNOWN]");
      }
   }

   if (useChannel) {
      switch (chan) {
      case SQ_CHAN::X:
         fmt::format_to(out, ".x");
         break;
      case SQ_CHAN::Y:
         fmt::format_to(out, ".y");
         break;
      case SQ_CHAN::Z:
         fmt::format_to(out, ".z");
         break;
      case SQ_CHAN::W:
         fmt::format_to(out, ".w");
         break;
      default:
         fmt::format_to(out, ".UNKNOWN");
         break;
      }
   }

   if (absolute) {
      fmt::format_to(out, "|");
   }
}

void
disassembleAluInstruction(fmt::memory_buffer &out,
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

   fmt::format_to(out, "{: <{}} ", name, namePad);

   auto writeMask = true;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      writeMask = inst.op2.WRITE_MASK();
   }

   if (!writeMask) {
      fmt::format_to(out, "____");
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

      fmt::format_to(out, ", ");
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

      fmt::format_to(out, ", ");
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

      fmt::format_to(out, ", ");
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
      fmt::format_to(out, " CLAMP");
   }

   if (isTranscendentalOnly(flags)) {
      switch (static_cast<SQ_ALU_SCL_BANK_SWIZZLE>(inst.word1.BANK_SWIZZLE())) {
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_210:
         fmt::format_to(out, " SCL_210");
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_122:
         fmt::format_to(out, " SCL_122");
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_212:
         fmt::format_to(out, " SCL_212");
         break;
      case SQ_ALU_SCL_BANK_SWIZZLE::SCL_221:
         fmt::format_to(out, " SCL_221");
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
         fmt::format_to(out, " VEC_021");
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_120:
         fmt::format_to(out, " VEC_120");
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_102:
         fmt::format_to(out, " VEC_102");
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_201:
         fmt::format_to(out, " VEC_201");
         break;
      case SQ_ALU_VEC_BANK_SWIZZLE::VEC_210:
         fmt::format_to(out, " VEC_210");
         break;
      default:
         decaf_abort(fmt::format("Unexpected BANK_SWIZZLE {}", inst.word1.BANK_SWIZZLE()));
      }
   }

   switch (inst.word0.PRED_SEL()) {
   case SQ_PRED_SEL::OFF:
      break;
   case SQ_PRED_SEL::ZERO:
      fmt::format_to(out, " PRED_SEL_ZERO");
      break;
   case SQ_PRED_SEL::ONE:
      fmt::format_to(out, " PRED_SEL_ONE");
      break;
   default:
      decaf_abort(fmt::format("Unexpected PRED_SEL {}", inst.word0.PRED_SEL()));
   }

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      if (inst.op2.UPDATE_EXECUTE_MASK()) {
         fmt::format_to(out, " UPDATE_EXEC_MASK");
      }

      if (inst.op2.UPDATE_PRED()) {
         fmt::format_to(out, " UPDATE_PRED");
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

      fmt::format_to(state.out, "\n{}{: <3}", state.indent, state.groupPC);

      for (auto j = 0u; j < group.instructions.size(); ++j) {
         auto &inst = group.instructions[j];
         auto unit = units.addInstructionUnit(inst);
         const char *name = nullptr;
         auto srcCount = 0u;

         if (j > 0) {
            fmt::format_to(state.out, "{}   ", state.indent);
         }

         fmt::format_to(state.out, " {}: ", unitName[unit]);

         disassembleAluInstruction(state.out, parent, inst, state.groupPC, unit, group.literals, 15);
         fmt::format_to(state.out, "\n");
      }

      slot = group.getNextSlot(slot);
      state.groupPC++;
   }
}

void
disassembleCfALUInstruction(fmt::memory_buffer &out,
                            const ControlFlowInst &inst)
{
   auto name = getInstructionName(inst.alu.word1.CF_INST());
   auto addr = inst.alu.word0.ADDR();
   auto count = inst.alu.word1.COUNT() + 1;

   fmt::format_to(out, "{}: ADDR({}) CNT({})", name, addr, count);

   if (!inst.word1.BARRIER()) {
      fmt::format_to(out, " NO_BARRIER");
   }

   if (inst.word1.WHOLE_QUAD_MODE()) {
      fmt::format_to(out, " WHOLE_QUAD");
   }

   disassembleKcache(out, 0, inst.alu.word0.KCACHE_MODE0(), inst.alu.word0.KCACHE_BANK0(), inst.alu.word1.KCACHE_ADDR0());
   disassembleKcache(out, 1, inst.alu.word1.KCACHE_MODE1(), inst.alu.word0.KCACHE_BANK1(), inst.alu.word1.KCACHE_ADDR1());
}

void
disassembleControlFlowALU(State &state, const ControlFlowInst &inst)
{
   auto addr = inst.alu.word0.ADDR();
   auto count = inst.alu.word1.COUNT() + 1;

   fmt::format_to(state.out, "{}{:02} ", state.indent, state.cfPC);
   disassembleCfALUInstruction(state.out, inst);

   increaseIndent(state);
   disassembleAluClause(state, inst, addr, count);
   decreaseIndent(state);
}

} // namespace disassembler

} // namespace latte
