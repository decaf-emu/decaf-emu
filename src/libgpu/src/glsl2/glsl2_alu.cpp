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
insertIndexMode(fmt::MemoryWriter &out,
                SQ_INDEX_MODE index)
{
   switch (index) {
   case SQ_INDEX_MODE::AR_X:
      out << "AR.x";
      break;
   case SQ_INDEX_MODE::AR_Y:
      out << "AR.y";
      break;
   case SQ_INDEX_MODE::AR_Z:
      out << "AR.z";
      break;
   case SQ_INDEX_MODE::AR_W:
      out << "AR.w";
      break;
   /* TODO: We need to implement AL yet, let's throw an error for now.
   case SQ_INDEX_MODE::LOOP:
      out << "AL";
      break;*/
   default:
      throw translate_exception(fmt::format("Invalid SQ_INDEX_MODE {}", index));
   }
}

void
insertChannel(fmt::MemoryWriter &out,
              SQ_CHAN channel)
{
   switch (channel) {
   case SQ_CHAN::X:
      out << 'x';
      break;
   case SQ_CHAN::Y:
      out << 'y';
      break;
   case SQ_CHAN::Z:
      out << 'z';
      break;
   case SQ_CHAN::W:
      out << 'w';
      break;
   default:
      throw translate_exception(fmt::format("Unexpected SQ_CHAN {}", channel));
   }
}

void
insertSource0(State &state,
              fmt::MemoryWriter &out,
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
              fmt::MemoryWriter &out,
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
              fmt::MemoryWriter &out,
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
                    fmt::MemoryWriter &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource0(state, out, cf, x);
   out << ", ";
   insertSource0(state, out, cf, y);
   out << ", ";
   insertSource0(state, out, cf, z);
   out << ", ";
   insertSource0(state, out, cf, w);
   out << ")";
}

void
insertSource1Vector(State &state,
                    fmt::MemoryWriter &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource1(state, out, cf, x);
   out << ", ";
   insertSource1(state, out, cf, y);
   out << ", ";
   insertSource1(state, out, cf, z);
   out << ", ";
   insertSource1(state, out, cf, w);
   out << ")";
}

void
insertSource2Vector(State &state,
                    fmt::MemoryWriter &out,
                    const ControlFlowInst &cf,
                    const AluInst &x,
                    const AluInst &y,
                    const AluInst &z,
                    const AluInst &w)
{
   out << "vec4(";
   insertSource2(state, out, cf, x);
   out << ", ";
   insertSource2(state, out, cf, y);
   out << ", ";
   insertSource2(state, out, cf, z);
   out << ", ";
   insertSource2(state, out, cf, w);
   out << ")";
}

void
insertSource(State &state,
             fmt::MemoryWriter &out,
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
      out << "abs(";
   }

   if (neg) {
      out << "-(";
   }

   if ((sel >= SQ_ALU_SRC::REGISTER_FIRST && sel <= SQ_ALU_SRC::REGISTER_LAST)
    || (sel >= SQ_ALU_SRC::KCACHE_BANK0_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK0_LAST)
    || (sel >= SQ_ALU_SRC::KCACHE_BANK1_FIRST && sel <= SQ_ALU_SRC::KCACHE_BANK1_LAST)
    || (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST)) {
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
      case SQ_ALU_SRC::PV:
      case SQ_ALU_SRC::PS:
         // PreviousVector, PreviousScalar
         if (flags & SQ_ALU_FLAG_INT_IN) {
            out << "floatBitsToInt(";
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << "floatBitsToUint(";
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
            out << "int(";
            didTypeConversion = true;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << "uint(";
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
      out << "R[" << (sel - SQ_ALU_SRC::REGISTER_FIRST);

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
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

      out << "UB_" << bank << ".values[" << id;

      if (state.shader) {
         state.shader->usedUniformBlocks[bank] = true;
      }

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
      needsChannelSelect = true;
   } else if (sel >= SQ_ALU_SRC::CONST_FILE_FIRST && sel <= SQ_ALU_SRC::CONST_FILE_LAST) {
      if (!state.shader) {
         out << "UR[";
      } else if (state.shader->type == Shader::PixelShader) {
         out << "PR[";
      } else if (state.shader->type == Shader::VertexShader) {
         out << "VR[";
      } else if (state.shader->type == Shader::GeometryShader) {
         out << "GR[";
      }

      out << (sel - SQ_ALU_SRC::CONST_FILE_FIRST);

      if (rel) {
         out << " + ";
         insertIndexMode(out, inst.word0.INDEX_MODE());
      }

      out << ']';
      needsChannelSelect = true;
   } else {
      switch (sel) {
      case SQ_ALU_SRC::PV:
         out << "PV";
         needsChannelSelect = true;
         break;
      case SQ_ALU_SRC::PS:
         out << "PS";
         break;
      case SQ_ALU_SRC::IMM_0:
         out << "0.0f";
         break;
      case SQ_ALU_SRC::IMM_1:
         out << "1.0f";
         break;
      case SQ_ALU_SRC::IMM_0_5:
         out << "0.5f";
         break;
      case SQ_ALU_SRC::IMM_1_INT:
         out << "1";
         break;
      case SQ_ALU_SRC::IMM_M_1_INT:
         out << "-1";
         break;
      case SQ_ALU_SRC::LITERAL:
         value.asUint = state.literals[chan];

         if (flags & SQ_ALU_FLAG_INT_IN) {
            out << value.asInt;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            out << value.asUint;
         } else {
            out.write("{:.6f}f", value.asFloat);
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
   case SQ_CHAN::X:
      out << "PVo.x = ";
      break;
   case SQ_CHAN::Y:
      out << "PVo.y = ";
      break;
   case SQ_CHAN::Z:
      out << "PVo.z = ";
      break;
   case SQ_CHAN::W:
      out << "PVo.w = ";
      break;
   case SQ_CHAN::T:
      out << "PSo   = ";
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
      fmt::MemoryWriter postWrite;

      auto gpr = inst.word1.DST_GPR();
      postWrite << "R[" << gpr << "].";
      insertChannel(postWrite, inst.word1.DST_CHAN());

      postWrite << " = ";

      switch (unit) {
      case SQ_CHAN::X:
         postWrite << "PVo.x";
         break;
      case SQ_CHAN::Y:
         postWrite << "PVo.y";
         break;
      case SQ_CHAN::Z:
         postWrite << "PVo.z";
         break;
      case SQ_CHAN::W:
         postWrite << "PVo.w";
         break;
      case SQ_CHAN::T:
         postWrite << "PSo";
         break;
      }

      postWrite << ";";

      state.postGroupWrites.push_back(postWrite.str());
   }

   if (flags & SQ_ALU_FLAG_INT_OUT) {
      state.out << "intBitsToFloat(";
   } else if (flags & SQ_ALU_FLAG_UINT_OUT) {
      state.out << "uintBitsToFloat(";
   }

   if (inst.word1.CLAMP()) {
      state.out << "clamp(";
   }

   if (omod != SQ_ALU_OMOD::OFF) {
      state.out << '(';
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
      state.out << ") * 2";
      break;
   case SQ_ALU_OMOD::M4:
      state.out << ") * 4";
      break;
   case SQ_ALU_OMOD::D2:
      state.out << ") / 2";
      break;
   default:
      throw translate_exception(fmt::format("Unexpected output modifier {}", omod));
   }

   if (inst.word1.CLAMP()) {
      state.out << ", 0, 1)";
   }

   if ((flags & SQ_ALU_FLAG_INT_OUT) || (flags & SQ_ALU_FLAG_UINT_OUT)) {
      state.out << ")";
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
         state.out << "predicateRegister";
      } else {
         state.out << "(" << condition << ")";
      }

      state.out << " ? Active : InactiveBranch;";
      insertLineEnd(state);
   }

   insertLineStart(state);
   insertDestBegin(state, cf, inst, state.unit);

   if (updatePredicate) {
      state.out << "predicateRegister";
   } else if (updateExecuteMask) {
      state.out << "(activeMask == Active)";
   } else {
      state.out << "(" << condition << ")";
   }

   if ((flags & latte::SQ_ALU_FLAG_INT_OUT) || (flags & latte::SQ_ALU_FLAG_UINT_OUT)) {
      state.out << " ? 1 : 0";
   } else {
      state.out << " ? 1.0f : 0.0f";
   }

   insertDestEnd(state, cf, inst);
   state.out << ';';
   insertLineEnd(state);
}

} // namespace glsl2
