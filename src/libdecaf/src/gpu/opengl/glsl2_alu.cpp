#include "glsl2_alu.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

namespace glsl2
{

union LiteralValue
{
   float asFloat;
   uint32_t asUint;
   int asInt;
};

void
insertIndexMode(fmt::MemoryWriter &out, SQ_INDEX_MODE index)
{
   switch (index) {
   case SQ_INDEX_AR_X:
      out << "AR.x";
      break;
   case SQ_INDEX_AR_Y:
      out << "AR.y";
      break;
   case SQ_INDEX_AR_Z:
      out << "AR.z";
      break;
   case SQ_INDEX_AR_W:
      out << "AR.w";
      break;
   case SQ_INDEX_LOOP:
      out << "AL";
      break;
   default:
      throw std::logic_error("Invalid SQ_INDEX_MODE");
   }
}

void
insertChannel(fmt::MemoryWriter &out, SQ_CHAN channel)
{
   switch (channel) {
   case SQ_CHAN_X:
      out << 'x';
      break;
   case SQ_CHAN_Y:
      out << 'y';
      break;
   case SQ_CHAN_Z:
      out << 'z';
      break;
   case SQ_CHAN_W:
      out << 'w';
      break;
   default:
      throw std::logic_error(fmt::format("Unexpected channel {}", channel));
   }
}

void
insertSource0(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   insertSource(out, literals, cf,
                inst,
                inst.word0.SRC0_SEL(),
                inst.word0.SRC0_REL(),
                inst.word0.SRC0_CHAN(),
                inst.op2.SRC0_ABS(),
                inst.word0.SRC0_NEG());
}

void
insertSource1(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   insertSource(out, literals, cf,
                inst,
                inst.word0.SRC1_SEL(),
                inst.word0.SRC1_REL(),
                inst.word0.SRC1_CHAN(),
                inst.op2.SRC1_ABS(),
                inst.word0.SRC1_NEG());
}

void
insertSource2(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   insertSource(out, literals, cf,
                inst,
                inst.op3.SRC2_SEL(),
                inst.op3.SRC2_REL(),
                inst.op3.SRC2_CHAN(),
                false,
                inst.op3.SRC2_NEG());
}

void
insertSource0Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource0(out, literals, cf, x);
   out << ", ";
   insertSource0(out, literals, cf, y);
   out << ", ";
   insertSource0(out, literals, cf, z);
   out << ", ";
   insertSource0(out, literals, cf, w);
   out << ")";
}

void
insertSource1Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource1(out, literals, cf, x);
   out << ", ";
   insertSource1(out, literals, cf, y);
   out << ", ";
   insertSource1(out, literals, cf, z);
   out << ", ";
   insertSource1(out, literals, cf, w);
   out << ")";
}

void
insertSource2Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource2(out, literals, cf, x);
   out << ", ";
   insertSource2(out, literals, cf, y);
   out << ", ";
   insertSource2(out, literals, cf, z);
   out << ", ";
   insertSource2(out, literals, cf, w);
   out << ")";
}

void
insertSource(fmt::MemoryWriter &out,
             const gsl::span<const uint32_t> &literals,
             const ControlFlowInst &cf,
             const AluInst &inst,
             const SQ_ALU_SRC sel,
             const SQ_REL rel,
             const SQ_CHAN chan,
             const bool abs,
             const bool neg)
{
   auto didTypeConversion = false;
   auto needsChannelSelect = false;
   auto flags = SQ_ALU_FLAG_NONE;
   LiteralValue value;

   if (inst.word1.ENCODING() == SQ_ALU_OP2) {
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   if (abs) {
      out << "abs(";
   }

   if (neg) {
      out << "-(";
   }

   if ((sel >= SQ_ALU_REGISTER_FIRST && sel <= SQ_ALU_REGISTER_LAST)
    || (sel >= SQ_ALU_TMP_REGISTER_FIRST && sel <= SQ_ALU_TMP_REGISTER_LAST)
    || (sel >= SQ_ALU_KCACHE_BANK0_FIRST && sel <= SQ_ALU_KCACHE_BANK0_LAST)
    || (sel >= SQ_ALU_KCACHE_BANK1_FIRST && sel <= SQ_ALU_KCACHE_BANK1_LAST)
    || (sel >= SQ_ALU_SRC_CONST_FILE_FIRST && sel <= SQ_ALU_SRC_CONST_FILE_LAST)) {
      // Register, Uniform Register, Uniform Block
      if (flags & SQ_ALU_FLAG_INT_IN) {
         out << "floatBitsToInt(";
         didTypeConversion = true;
      } else if (flags & SQ_ALU_FLAG_UINT_IN) {
         out << "floatBitsToUint(";
         didTypeConversion = true;
      }
   } else {
      switch (sel) {
      case SQ_ALU_SRC_PV:
      case SQ_ALU_SRC_PS:
         // PreviousVector, PreviousScalar
         if (flags & SQ_ALU_FLAG_INT_IN) {
            out << "floatBitsToInt(";
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << "floatBitsToUint(";
            didTypeConversion = true;
         }
         break;
      case SQ_ALU_SRC_1_DBL_L:
      case SQ_ALU_SRC_1_DBL_M:
      case SQ_ALU_SRC_0_5_DBL_L:
      case SQ_ALU_SRC_0_5_DBL_M:
      case SQ_ALU_SRC_0:
      case SQ_ALU_SRC_1:
      case SQ_ALU_SRC_0_5:
         // Constant values
         if (flags & SQ_ALU_FLAG_INT_IN) {
            out << "int(";
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << "uint(";
            didTypeConversion = true;
         }
         break;
      case SQ_ALU_SRC_1_INT:
      case SQ_ALU_SRC_M_1_INT:
      case SQ_ALU_SRC_LITERAL:
         // No need for type conversion
         break;
      default:
         throw std::logic_error("Unexpected ALU source sel");
      }
   }

   // Now for actual value!
   if ((sel >= SQ_ALU_REGISTER_FIRST && sel <= SQ_ALU_REGISTER_LAST)
       || (sel >= SQ_ALU_TMP_REGISTER_FIRST && sel <= SQ_ALU_TMP_REGISTER_LAST)) {
      // Registers
      if (sel >= SQ_ALU_TMP_REGISTER_FIRST && sel <= SQ_ALU_TMP_REGISTER_LAST) {
         out << "T[" << (SQ_ALU_TMP_REGISTER_LAST - sel);
      } else {
         out << "R[" << (sel - SQ_ALU_REGISTER_FIRST);
      }

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
      needsChannelSelect = true;
   } else if ((sel >= SQ_ALU_KCACHE_BANK0_FIRST && sel <= SQ_ALU_KCACHE_BANK0_LAST)
              || (sel >= SQ_ALU_KCACHE_BANK1_FIRST && sel <= SQ_ALU_KCACHE_BANK1_LAST)) {
      auto index = (sel >= SQ_ALU_KCACHE_BANK1_FIRST) ? 1 : 0;
      auto addr = 0u;
      auto bank = 0u;
      auto mode = SQ_CF_KCACHE_NOP;

      if (sel < SQ_ALU_KCACHE_BANK1_FIRST) {
         addr = cf.alu.word1.KCACHE_ADDR0();
         bank = cf.alu.word0.KCACHE_BANK0();
         mode = cf.alu.word0.KCACHE_MODE0();
      } else {
         addr = cf.alu.word1.KCACHE_ADDR1();
         bank = cf.alu.word0.KCACHE_BANK1();
         mode = cf.alu.word1.KCACHE_MODE1();
      }

      auto id = (addr * 16);

      if (sel >= SQ_ALU_KCACHE_BANK1_FIRST) {
         id += sel - SQ_ALU_KCACHE_BANK1_FIRST;
      } else {
         id += sel - SQ_ALU_KCACHE_BANK0_FIRST;
      }

      if (mode == SQ_CF_KCACHE_LOCK_LOOP_INDEX) {
         throw std::logic_error("Unimplemented kcache lock mode SQ_CF_KCACHE_LOCK_LOOP_INDEX");
      } else if (mode == SQ_CF_KCACHE_NOP) {
         throw std::logic_error("Invalid kcache lock mode SQ_CF_KCACHE_NOP");
      }

      out << "UB[" << bank << "].values[" << id;

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
      needsChannelSelect = true;
   } else if (sel >= SQ_ALU_SRC_CONST_FILE_FIRST && sel <= SQ_ALU_SRC_CONST_FILE_LAST) {
      out << "UR[" << (sel - SQ_ALU_SRC_CONST_FILE_FIRST);

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
      needsChannelSelect = true;
   } else {
      switch (sel) {
      case SQ_ALU_SRC_PV:
         out << "PV";
         needsChannelSelect = true;
         break;
      case SQ_ALU_SRC_PS:
         out << "PS";
         break;
      case SQ_ALU_SRC_0:
         out << "0.0f";
         break;
      case SQ_ALU_SRC_1:
         out << "1.0f";
         break;
      case SQ_ALU_SRC_0_5:
         out << "0.5f";
         break;
      case SQ_ALU_SRC_1_INT:
         out << "1";
         break;
      case SQ_ALU_SRC_M_1_INT:
         out << "-1";
         break;
      case SQ_ALU_SRC_LITERAL:
         value.asUint = literals[chan];

         if (flags & SQ_ALU_FLAG_INT_IN) {
            out << value.asInt;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << value.asUint;
         } else {
            out.write("{:.6f}f", value.asFloat);
         }
         break;
      case SQ_ALU_SRC_1_DBL_L:
      case SQ_ALU_SRC_1_DBL_M:
      case SQ_ALU_SRC_0_5_DBL_L:
      case SQ_ALU_SRC_0_5_DBL_M:
      case SQ_ALU_SRC_LDS_OQ_A:
      case SQ_ALU_SRC_LDS_OQ_B:
      case SQ_ALU_SRC_LDS_OQ_A_POP:
      case SQ_ALU_SRC_LDS_OQ_B_POP:
      case SQ_ALU_SRC_LDS_DIRECT_A:
      case SQ_ALU_SRC_LDS_DIRECT_B:
      case SQ_ALU_SRC_TIME_HI:
      case SQ_ALU_SRC_TIME_LO:
      case SQ_ALU_SRC_MASK_HI:
      case SQ_ALU_SRC_MASK_LO:
      case SQ_ALU_SRC_HW_WAVE_ID:
      case SQ_ALU_SRC_SIMD_ID:
      case SQ_ALU_SRC_SE_ID:
      case SQ_ALU_SRC_HW_THREADGRP_ID:
      case SQ_ALU_SRC_WAVE_ID_IN_GRP:
      case SQ_ALU_SRC_NUM_THREADGRP_WAVES:
      case SQ_ALU_SRC_HW_ALU_ODD:
      case SQ_ALU_SRC_LOOP_IDX:
      case SQ_ALU_SRC_PARAM_BASE_ADDR:
      case SQ_ALU_SRC_NEW_PRIM_MASK:
      case SQ_ALU_SRC_PRIM_MASK_HI:
      case SQ_ALU_SRC_PRIM_MASK_LO:
      default:
         throw std::logic_error(fmt::format("Unsupported ALU source sel {}", sel));
         break;
      }
   }

   // Do a santiy check, ensure that we only use relative indexing with registers
   if (rel) {
      if ((sel < SQ_ALU_REGISTER_FIRST || sel > SQ_ALU_REGISTER_LAST)
          && (sel < SQ_ALU_TMP_REGISTER_FIRST || sel > SQ_ALU_TMP_REGISTER_LAST)
          && (sel < SQ_ALU_SRC_CONST_FILE_FIRST || sel > SQ_ALU_SRC_CONST_FILE_LAST)
          && (sel < SQ_ALU_KCACHE_BANK0_FIRST || sel > SQ_ALU_KCACHE_BANK0_LAST)
          && (sel < SQ_ALU_KCACHE_BANK1_FIRST || sel > SQ_ALU_KCACHE_BANK1_LAST)) {
         throw std::logic_error(fmt::format("Relative AluSource is only supported for registers and uniforms, sel: {}", sel));
      }
   }

   if (needsChannelSelect) {
      out << '.';
      insertChannel(out, chan);
   }

   if (didTypeConversion) {
      out << ")";
   }

   if (abs) {
      out << ")";
   }

   if (neg) {
      out << ")";
   }
}

void
insertPreviousValueUpdate(fmt::MemoryWriter &out,
                          SQ_CHAN unit)
{
   switch (unit) {
   case SQ_CHAN_X:
      out << "PVo.x = ";
      break;
   case SQ_CHAN_Y:
      out << "PVo.y = ";
      break;
   case SQ_CHAN_Z:
      out << "PVo.z = ";
      break;
   case SQ_CHAN_W:
      out << "PVo.w = ";
      break;
   case SQ_CHAN_T:
      out << "PSo   = ";
      break;
   }
}

void
insertDestBegin(fmt::MemoryWriter &out,
                const ControlFlowInst &cf,
                const AluInst &inst,
                SQ_CHAN unit)
{
   auto flags = SQ_ALU_FLAG_NONE;
   auto omod = SQ_ALU_OMOD_OFF;
   auto writeMask = true;

   insertPreviousValueUpdate(out, unit);

   if (inst.word1.ENCODING() == SQ_ALU_OP2) {
      writeMask = inst.op2.WRITE_MASK();
      omod = inst.op2.OMOD();
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   if (writeMask) {
      auto gpr = inst.word1.DST_GPR().get();

      if (gpr >= SQ_ALU_TMP_REGISTER_FIRST) {
         out << 'T';
         gpr = SQ_ALU_TMP_REGISTER_LAST - gpr;
      } else {
         out << 'R';
      }

      out << "[" << gpr << "].";
      insertChannel(out, inst.word1.DST_CHAN());
      out << " = ";
   }

   if (flags & SQ_ALU_FLAG_INT_OUT) {
      out << "intBitsToFloat(";
   } else if (flags & SQ_ALU_FLAG_UINT_OUT) {
      out << "uintBitsToFloat(";
   }

   if (omod != SQ_ALU_OMOD_OFF) {
      out << '(';
   }

   if (inst.word1.CLAMP()) {
      out << "clamp(";
   }
}

void
insertDestEnd(fmt::MemoryWriter &out,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   auto omod = SQ_ALU_OMOD_OFF;
   auto flags = SQ_ALU_FLAG_NONE;

   if (inst.word1.ENCODING() == SQ_ALU_OP2) {
      omod = inst.op2.OMOD();
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   if (inst.word1.CLAMP()) {
      out << ", 0, 1)";
   }

   switch (omod) {
   case SQ_ALU_OMOD_OFF:
      break;
   case SQ_ALU_OMOD_M2:
      out << ") * 2";
      break;
   case SQ_ALU_OMOD_M4:
      out << ") * 4";
      break;
   case SQ_ALU_OMOD_D2:
      out << ") / 2";
      break;
   default:
      throw std::logic_error(fmt::format("Unexpected output modifier {}", omod));
   }

   if ((flags & SQ_ALU_FLAG_INT_OUT) || (flags & SQ_ALU_FLAG_UINT_OUT)) {
      out << ")";
   }
}

void
updatePredicate(State &state, const ControlFlowInst &cf, const AluInst &inst, const std::string &condition)
{
   auto updateExecuteMask = inst.op2.UPDATE_EXECUTE_MASK();
   auto updatePredicate = inst.op2.UPDATE_PRED();
   auto flags = getInstructionFlags(inst.op2.ALU_INST());

   if (updatePredicate) {
      insertLineStart(state);
      state.out << "predicateRegister = (" << condition << ");";
      insertLineEnd(state);
   }

   if (updateExecuteMask) {
      insertLineStart(state);
      state.out << "activeMask = ";

      if (updatePredicate) {
         state.out << "predicateRegister;";
      } else {
         state.out << "(" << condition << ");";
      }
      insertLineEnd(state);
   }

   insertLineStart(state);
   insertDestBegin(state.out, cf, inst, state.unit);

   if (updatePredicate) {
      state.out << "predicateRegister";
   } else if (updateExecuteMask) {
      state.out << "activeMask";
   } else {
      state.out << "(" << condition << ")";
   }

   if ((flags & latte::SQ_ALU_FLAG_INT_OUT) || (flags & latte::SQ_ALU_FLAG_UINT_OUT)) {
      state.out << " ? 1 : 0";
   } else {
      state.out << " ? 1.0f : 0.0f";
   }

   insertDestEnd(state.out, cf, inst);
   state.out << ';';
   insertLineEnd(state);
}

} // namespace glsl2
