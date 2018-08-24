#include "glsl2_alu.h"
#include "latte/latte_instructions.h"

#include <fmt/format.h>

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
insertIndexMode(fmt::memory_buffer &out,
                SQ_INDEX_MODE index)
{
   switch (index) {
   case SQ_INDEX_MODE::AR_X:
      fmt::format_to(out, "AR.x");
      break;
   case SQ_INDEX_MODE::AR_Y:
      fmt::format_to(out, "AR.y");
      break;
   case SQ_INDEX_MODE::AR_Z:
      fmt::format_to(out, "AR.z");
      break;
   case SQ_INDEX_MODE::AR_W:
      fmt::format_to(out, "AR.w");
      break;
   /* TODO: We need to implement AL yet, let's throw an error for now.
   case SQ_INDEX_MODE::LOOP:
      fmt::format_to(out, "AL");
      break;*/
   default:
      throw translate_exception(fmt::format("Invalid SQ_INDEX_MODE {}", index));
   }
}

void
insertChannel(fmt::memory_buffer &out,
              SQ_CHAN channel)
{
   switch (channel) {
   case SQ_CHAN::X:
      fmt::format_to(out, "x");
      break;
   case SQ_CHAN::Y:
      fmt::format_to(out, "y");
      break;
   case SQ_CHAN::Z:
      fmt::format_to(out, "z");
      break;
   case SQ_CHAN::W:
      fmt::format_to(out, "w");
      break;
   default:
      throw translate_exception(fmt::format("Unexpected SQ_CHAN {}", channel));
   }
}

void
insertSource0(State &state,
              fmt::memory_buffer &out,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   auto abs = false;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      abs = inst.op2.SRC0_ABS();
   }

   insertSource(state, out, cf,
                inst,
                inst.word0.SRC0_SEL(),
                inst.word0.SRC0_REL(),
                inst.word0.SRC0_CHAN(),
                abs,
                inst.word0.SRC0_NEG());
}

void
insertSource1(State &state,
              fmt::memory_buffer &out,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   auto abs = false;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      abs = inst.op2.SRC1_ABS();
   }

   insertSource(state, out, cf,
                inst,
                inst.word0.SRC1_SEL(),
                inst.word0.SRC1_REL(),
                inst.word0.SRC1_CHAN(),
                abs,
                inst.word0.SRC1_NEG());
}

void
insertSource2(State &state,
              fmt::memory_buffer &out,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   insertSource(state, out, cf,
                inst,
                inst.op3.SRC2_SEL(),
                inst.op3.SRC2_REL(),
                inst.op3.SRC2_CHAN(),
                false,
                inst.op3.SRC2_NEG());
}

void
insertSource0Vector(State &state,
                    fmt::memory_buffer &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   fmt::format_to(out, "vec4(");
   insertSource0(state, out, cf, x);
   fmt::format_to(out, ", ");
   insertSource0(state, out, cf, y);
   fmt::format_to(out, ", ");
   insertSource0(state, out, cf, z);
   fmt::format_to(out, ", ");
   insertSource0(state, out, cf, w);
   fmt::format_to(out, ")");
}

void
insertSource1Vector(State &state,
                    fmt::memory_buffer &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   fmt::format_to(out, "vec4(");
   insertSource1(state, out, cf, x);
   fmt::format_to(out, ", ");
   insertSource1(state, out, cf, y);
   fmt::format_to(out, ", ");
   insertSource1(state, out, cf, z);
   fmt::format_to(out, ", ");
   insertSource1(state, out, cf, w);
   fmt::format_to(out, ")");
}

void
insertSource2Vector(State &state,
                    fmt::memory_buffer &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   fmt::format_to(out, "vec4(");
   insertSource2(state, out, cf, x);
   fmt::format_to(out, ", ");
   insertSource2(state, out, cf, y);
   fmt::format_to(out, ", ");
   insertSource2(state, out, cf, z);
   fmt::format_to(out, ", ");
   insertSource2(state, out, cf, w);
   fmt::format_to(out, ")");
}

void
insertSource(State &state,
             fmt::memory_buffer &out,
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

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   if (abs) {
      fmt::format_to(out, "abs(");
   }

   if (neg) {
      fmt::format_to(out, "-(");
   }

   if ((sel >= SQ_ALU_SRC::REGISTER_FIRST && sel <= SQ_ALU_SRC::REGISTER_LAST)
    || (sel >= SQ_ALU_SRC::KCACHE_BANK0_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK0_LAST)
    || (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK1_LAST)
    || (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST)) {
      // Register, Uniform Register, Uniform Block
      if (flags & SQ_ALU_FLAG_INT_IN) {
         fmt::format_to(out, "floatBitsToInt(");
         didTypeConversion = true;
      } else if (flags & SQ_ALU_FLAG_UINT_IN) {
         fmt::format_to(out, "floatBitsToUint(");
         didTypeConversion = true;
      }
   } else {
      switch (sel) {
      case SQ_ALU_SRC::PV:
      case SQ_ALU_SRC::PS:
         // PreviousVector, PreviousScalar
         if (flags & SQ_ALU_FLAG_INT_IN) {
            fmt::format_to(out, "floatBitsToInt(");
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            fmt::format_to(out, "floatBitsToUint(");
            didTypeConversion = true;
         }
         break;
      case SQ_ALU_SRC::IMM_1_DBL_L:
      case SQ_ALU_SRC::IMM_1_DBL_M:
      case SQ_ALU_SRC::IMM_0_5_DBL_L:
      case SQ_ALU_SRC::IMM_0_5_DBL_M:
      case SQ_ALU_SRC::IMM_0:
      case SQ_ALU_SRC::IMM_1:
      case SQ_ALU_SRC::IMM_0_5:
         // Constant values
         if (flags & SQ_ALU_FLAG_INT_IN) {
            fmt::format_to(out, "int(");
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            fmt::format_to(out, "uint(");
            didTypeConversion = true;
         }
         break;
      case SQ_ALU_SRC::IMM_1_INT:
      case SQ_ALU_SRC::IMM_M_1_INT:
      case SQ_ALU_SRC::LITERAL:
         // No need for type conversion
         break;
      default:
         throw translate_exception(fmt::format("Unexpected ALU source sel {}", sel));
      }
   }

   // Now for actual value!
   if (sel >= SQ_ALU_SRC::REGISTER_FIRST && sel <= SQ_ALU_SRC::REGISTER_LAST) {
      // Registers
      fmt::format_to(out, "R[{}", sel - SQ_ALU_SRC::REGISTER_FIRST);

      if (rel) {
         fmt::format_to(out, " + ");
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      fmt::format_to(out, "]");
      needsChannelSelect = true;
   } else if ((sel >= SQ_ALU_SRC::KCACHE_BANK0_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK0_LAST)
              || (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK1_LAST)) {
      auto index = (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST) ? 1 : 0;
      auto addr = 0u;
      auto bank = 0u;
      auto mode = SQ_CF_KCACHE_MODE::NOP;

      if (sel < SQ_ALU_SRC::KCACHE_BANK1_FIRST) {
         addr = cf.alu.word1.KCACHE_ADDR0();
         bank = cf.alu.word0.KCACHE_BANK0();
         mode = cf.alu.word0.KCACHE_MODE0();
      } else {
         addr = cf.alu.word1.KCACHE_ADDR1();
         bank = cf.alu.word0.KCACHE_BANK1();
         mode = cf.alu.word1.KCACHE_MODE1();
      }

      auto id = (addr * 16);

      if (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST) {
         id += sel - SQ_ALU_SRC::KCACHE_BANK1_FIRST;
      } else {
         id += sel - SQ_ALU_SRC::KCACHE_BANK0_FIRST;
      }

      if (mode == SQ_CF_KCACHE_MODE::LOCK_LOOP_INDEX) {
         throw translate_exception("Unimplemented kcache lock mode SQ_CF_KCACHE_LOCK_LOOP_INDEX");
      } else if (mode == SQ_CF_KCACHE_MODE::NOP) {
         throw translate_exception("Invalid kcache lock mode SQ_CF_KCACHE_NOP");
      }

      fmt::format_to(out, "UB_{}.values[{}", bank, id);

      if (state.shader) {
         state.shader->usedUniformBlocks[bank] = true;
      }

      if (rel) {
         fmt::format_to(out, " + ");
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      fmt::format_to(out, "]");
      needsChannelSelect = true;
   } else if (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST) {
      if (!state.shader) {
         fmt::format_to(out, "UR[");
      } else if (state.shader->type == Shader::PixelShader) {
         fmt::format_to(out, "PR[");
      } else if (state.shader->type == Shader::VertexShader) {
         fmt::format_to(out, "VR[");
      } else if (state.shader->type == Shader::GeometryShader) {
         fmt::format_to(out, "GR[");
      }

      fmt::format_to(out, "{}", sel - SQ_ALU_SRC::CONST_FILE_FIRST);

      if (rel) {
         fmt::format_to(out, " + ");
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      fmt::format_to(out, "]");
      needsChannelSelect = true;
   } else {
      switch (sel) {
      case SQ_ALU_SRC::PV:
         fmt::format_to(out, "PV");
         needsChannelSelect = true;
         break;
      case SQ_ALU_SRC::PS:
         fmt::format_to(out, "PS");
         break;
      case SQ_ALU_SRC::IMM_0:
         fmt::format_to(out, "0.0f");
         break;
      case SQ_ALU_SRC::IMM_1:
         fmt::format_to(out, "1.0f");
         break;
      case SQ_ALU_SRC::IMM_0_5:
         fmt::format_to(out, "0.5f");
         break;
      case SQ_ALU_SRC::IMM_1_INT:
         fmt::format_to(out, "1");
         break;
      case SQ_ALU_SRC::IMM_M_1_INT:
         fmt::format_to(out, "-1");
         break;
      case SQ_ALU_SRC::LITERAL:
         value.asUint = state.literals[chan];

         if (flags & SQ_ALU_FLAG_INT_IN) {
            fmt::format_to(out, "{}", value.asInt);
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            fmt::format_to(out, "{}", value.asUint);
         } else {
            fmt::format_to(out, "{:.6f}f", value.asFloat);
         }
         break;
      case SQ_ALU_SRC::IMM_1_DBL_L:
      case SQ_ALU_SRC::IMM_1_DBL_M:
      case SQ_ALU_SRC::IMM_0_5_DBL_L:
      case SQ_ALU_SRC::IMM_0_5_DBL_M:
      case SQ_ALU_SRC::LDS_OQ_A:
      case SQ_ALU_SRC::LDS_OQ_B:
      case SQ_ALU_SRC::LDS_OQ_A_POP:
      case SQ_ALU_SRC::LDS_OQ_B_POP:
      case SQ_ALU_SRC::LDS_DIRECT_A:
      case SQ_ALU_SRC::LDS_DIRECT_B:
      case SQ_ALU_SRC::TIME_HI:
      case SQ_ALU_SRC::TIME_LO:
      case SQ_ALU_SRC::MASK_HI:
      case SQ_ALU_SRC::MASK_LO:
      case SQ_ALU_SRC::HW_WAVE_ID:
      case SQ_ALU_SRC::SIMD_ID:
      case SQ_ALU_SRC::SE_ID:
      case SQ_ALU_SRC::HW_THREADGRP_ID:
      case SQ_ALU_SRC::WAVE_ID_IN_GRP:
      case SQ_ALU_SRC::NUM_THREADGRP_WAVES:
      case SQ_ALU_SRC::HW_ALU_ODD:
      case SQ_ALU_SRC::LOOP_IDX:
      case SQ_ALU_SRC::PARAM_BASE_ADDR:
      case SQ_ALU_SRC::NEW_PRIM_MASK:
      case SQ_ALU_SRC::PRIM_MASK_HI:
      case SQ_ALU_SRC::PRIM_MASK_LO:
      default:
         throw translate_exception(fmt::format("Unsupported ALU source sel {}", sel));
      }
   }

   // Do a sanity check, ensure that we only use relative indexing with registers
   if (rel) {
      if ((sel < SQ_ALU_SRC::REGISTER_FIRST || sel > SQ_ALU_SRC::REGISTER_LAST)
       && (sel < SQ_ALU_SRC::CONST_FILE_FIRST || sel > SQ_ALU_SRC::CONST_FILE_LAST)
       && (sel < SQ_ALU_SRC::KCACHE_BANK0_FIRST || sel > SQ_ALU_SRC::KCACHE_BANK0_LAST)
       && (sel < SQ_ALU_SRC::KCACHE_BANK1_FIRST || sel > SQ_ALU_SRC::KCACHE_BANK1_LAST)) {
         throw translate_exception(fmt::format("Relative AluSource is only supported for registers and uniforms, sel: {}", sel));
      }
   }

   if (needsChannelSelect) {
      fmt::format_to(out, ".");
      insertChannel(out, chan);
   }

   if (didTypeConversion) {
      fmt::format_to(out, ")");
   }

   if (abs) {
      fmt::format_to(out, ")");
   }

   if (neg) {
      fmt::format_to(out, ")");
   }
}

void
insertPreviousValueUpdate(fmt::memory_buffer &out,
                          SQ_CHAN unit)
{
   switch (unit) {
   case SQ_CHAN::X:
      fmt::format_to(out, "PVo.x = ");
      break;
   case SQ_CHAN::Y:
      fmt::format_to(out, "PVo.y = ");
      break;
   case SQ_CHAN::Z:
      fmt::format_to(out, "PVo.z = ");
      break;
   case SQ_CHAN::W:
      fmt::format_to(out, "PVo.w = ");
      break;
   case SQ_CHAN::T:
      fmt::format_to(out, "PSo   = ");
      break;
   }
}

void
insertDestBegin(State &state,
                const ControlFlowInst &cf,
                const AluInst &inst,
                SQ_CHAN unit)
{
   auto flags = SQ_ALU_FLAG_NONE;
   auto omod = SQ_ALU_OMOD::OFF;
   auto writeMask = true;

   insertPreviousValueUpdate(state.out, unit);

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      writeMask = inst.op2.WRITE_MASK();
      omod = inst.op2.OMOD();
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   if (writeMask) {
      fmt::memory_buffer postWrite;

      auto gpr = inst.word1.DST_GPR();
      fmt::format_to(postWrite, "R[{}].", gpr);
      insertChannel(postWrite, inst.word1.DST_CHAN());

      fmt::format_to(postWrite, " = ");

      switch (unit) {
      case SQ_CHAN::X:
         fmt::format_to(postWrite, "PVo.x");
         break;
      case SQ_CHAN::Y:
         fmt::format_to(postWrite, "PVo.y");
         break;
      case SQ_CHAN::Z:
         fmt::format_to(postWrite, "PVo.z");
         break;
      case SQ_CHAN::W:
         fmt::format_to(postWrite, "PVo.w");
         break;
      case SQ_CHAN::T:
         fmt::format_to(postWrite, "PSo");
         break;
      }

      fmt::format_to(postWrite, ";");

      state.postGroupWrites.push_back(to_string(postWrite));
   }

   if (flags & SQ_ALU_FLAG_INT_OUT) {
      fmt::format_to(state.out, "intBitsToFloat(");
   } else if (flags & SQ_ALU_FLAG_UINT_OUT) {
      fmt::format_to(state.out, "uintBitsToFloat(");
   }

   if (inst.word1.CLAMP()) {
      fmt::format_to(state.out, "clamp(");
   }

   if (omod != SQ_ALU_OMOD::OFF) {
      fmt::format_to(state.out, "(");
   }
}

void
insertDestEnd(State &state,
              const ControlFlowInst &cf,
              const AluInst &inst)
{
   auto omod = SQ_ALU_OMOD::OFF;
   auto flags = SQ_ALU_FLAG_NONE;

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      omod = inst.op2.OMOD();
      flags = getInstructionFlags(inst.op2.ALU_INST());
   } else {
      flags = getInstructionFlags(inst.op3.ALU_INST());
   }

   switch (omod) {
   case SQ_ALU_OMOD::OFF:
      break;
   case SQ_ALU_OMOD::M2:
      fmt::format_to(state.out, ") * 2");
      break;
   case SQ_ALU_OMOD::M4:
      fmt::format_to(state.out, ") * 4");
      break;
   case SQ_ALU_OMOD::D2:
      fmt::format_to(state.out, ") / 2");
      break;
   default:
      throw translate_exception(fmt::format("Unexpected output modifier {}", omod));
   }

   if (inst.word1.CLAMP()) {
      fmt::format_to(state.out, ", 0, 1)");
   }

   if ((flags & SQ_ALU_FLAG_INT_OUT) || (flags & SQ_ALU_FLAG_UINT_OUT)) {
      fmt::format_to(state.out, ")");
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
      fmt::format_to(state.out, "predicateRegister = ({});", condition);
      insertLineEnd(state);
   }

   if (updateExecuteMask) {
      insertLineStart(state);
      fmt::format_to(state.out, "activeMask = ");

      if (updatePredicate) {
         fmt::format_to(state.out, "predicateRegister");
      } else {
         fmt::format_to(state.out, "({})", condition);
      }

      fmt::format_to(state.out, " ? Active : InactiveBranch;");
      insertLineEnd(state);
   }

   insertLineStart(state);
   insertDestBegin(state, cf, inst, state.unit);

   if (updatePredicate) {
      fmt::format_to(state.out, "predicateRegister");
   } else if (updateExecuteMask) {
      fmt::format_to(state.out, "(activeMask == Active)");
   } else {
      fmt::format_to(state.out, "({})");
   }

   if ((flags & latte::SQ_ALU_FLAG_INT_OUT) || (flags & latte::SQ_ALU_FLAG_UINT_OUT)) {
      fmt::format_to(state.out, " ? 1 : 0");
   } else {
      fmt::format_to(state.out, " ? 1.0f : 0.0f");
   }

   insertDestEnd(state, cf, inst);
   fmt::format_to(state.out, ";");
   insertLineEnd(state);
}

} // namespace glsl2
