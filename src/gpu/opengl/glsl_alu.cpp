#include "glsl_generator.h"
#include "utils/bit_cast.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;
using latte::shadir::AluDest;
using latte::shadir::ValueType;

namespace gpu
{

namespace opengl
{

namespace glsl
{

void translateChannel(GenerateState &state, latte::SQ_CHAN channel)
{
   switch (channel) {
   case latte::SQ_CHAN_X:
      state.out << 'x';
      break;
   case latte::SQ_CHAN_Y:
      state.out << 'y';
      break;
   case latte::SQ_CHAN_Z:
      state.out << 'z';
      break;
   case latte::SQ_CHAN_W:
      state.out << 'w';
      break;
   default:
      state.out << '?';
      break;
   }
}

void translateAluDestStart(GenerateState &state, AluInstruction *ins)
{
   if (ins->unit != latte::SQ_CHAN_T) {
      if (state.shader->pvUsed.find(ins->groupPC) != state.shader->pvUsed.end()) {
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            // If we read and write PV in this group we must use a temp to prevent clash
            state.out << "PVo.";
         } else {
            state.out << "PV.";
         }

         if (ins->isReduction) {
            // Reduction targets PV.x always
            state.out << 'x';
         } else {
            translateChannel(state, ins->unit);
         }

         state.out << " = ";
      }
   } else {
      if (state.shader->psUsed.find(ins->groupPC) != state.shader->psUsed.end()) {
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            state.out << "PSo = ";
         } else {
            state.out << "PS = ";
         }
      }
   }

   if (ins->writeMask) {
      state.out << 'R' << ins->dst.sel << '.';
      translateChannel(state, ins->dst.chan);
      state.out << " = ";
   }

   if (ins->dst.type == ValueType::Uint) {
      state.out << "uintBitsToFloat(";
   } else if (ins->dst.type == ValueType::Int) {
      state.out << "intBitsToFloat(";
   }

   switch (ins->outputModifier) {
   case latte::SQ_ALU_OMOD_M2:
   case latte::SQ_ALU_OMOD_M4:
   case latte::SQ_ALU_OMOD_D2:
      state.out << '(';
      break;
   }

   if (ins->dst.clamp) {
      state.out << "clamp(";
   }
}

void translateAluDestEnd(GenerateState &state, AluInstruction *ins)
{
   if (ins->dst.clamp) {
      state.out << ", 0, 1)";
   }

   switch (ins->outputModifier) {
   case latte::SQ_ALU_OMOD_M2:
      state.out << ") * 2";
      break;
   case latte::SQ_ALU_OMOD_M4:
      state.out << ") * 4";
      break;
   case latte::SQ_ALU_OMOD_D2:
      state.out << ") / 2";
      break;
   }

   if (ins->dst.type == ValueType::Int || ins->dst.type == ValueType::Uint) {
      state.out << ")";
   }
}

void translateIndex(GenerateState &state, latte::SQ_INDEX_MODE index)
{
   switch (index) {
   case latte::SQ_INDEX_AR_X:
      state.out << "AR.x";
      break;
   case latte::SQ_INDEX_AR_Y:
      state.out << "AR.y";
      break;
   case latte::SQ_INDEX_AR_Z:
      state.out << "AR.z";
      break;
   case latte::SQ_INDEX_AR_W:
      state.out << "AR.w";
      break;
   case latte::SQ_INDEX_LOOP:
      state.out << "AL";
      break;
   default:
      throw std::logic_error("Invalid SQ_INDEX_MODE");
   }
}

void translateAluSource(GenerateState &state, const AluInstruction *ins, const AluSource &src)
{
   bool didTypeConversion = false;
   bool doChannelSelect = false;

   if (src.absolute) {
      state.out << "abs(";
   }

   if (src.negate) {
      state.out << '-';
   }

   if ((src.sel >= latte::SQ_ALU_REGISTER_0 && src.sel <= latte::SQ_ALU_REGISTER_127)
    || (src.sel >= latte::SQ_ALU_KCACHE_BANK0_0 && src.sel <= latte::SQ_ALU_KCACHE_BANK0_31)
    || (src.sel >= latte::SQ_ALU_KCACHE_BANK1_0 && src.sel <= latte::SQ_ALU_KCACHE_BANK1_31)
    || (src.sel >= latte::SQ_ALU_SRC_CONST_FILE_0 && src.sel <= latte::SQ_ALU_SRC_CONST_FILE_255)) {
      // Register, uniform register, uniform block must bit cast
      if (src.type == ValueType::Int) {
         state.out << "floatBitsToInt(";
         didTypeConversion = true;
      } else if (src.type == ValueType::Uint) {
         state.out << "floatBitsToUint(";
         didTypeConversion = true;
      }
   } else {
      switch (src.sel) {
      case latte::SQ_ALU_SRC_PV:
      case latte::SQ_ALU_SRC_PS:
         // PV, PS is bit cast
         if (src.type == ValueType::Int) {
            state.out << "floatBitsToInt(";
            didTypeConversion = true;
         } else if (src.type == ValueType::Uint) {
            state.out << "floatBitsToUint(";
            didTypeConversion = true;
         }
         break;
      case latte::SQ_ALU_SRC_LDS_OQ_A:
      case latte::SQ_ALU_SRC_LDS_OQ_B:
      case latte::SQ_ALU_SRC_LDS_OQ_A_POP:
      case latte::SQ_ALU_SRC_LDS_OQ_B_POP:
      case latte::SQ_ALU_SRC_LDS_DIRECT_A:
      case latte::SQ_ALU_SRC_LDS_DIRECT_B:
      case latte::SQ_ALU_SRC_TIME_HI:
      case latte::SQ_ALU_SRC_TIME_LO:
      case latte::SQ_ALU_SRC_MASK_HI:
      case latte::SQ_ALU_SRC_MASK_LO:
      case latte::SQ_ALU_SRC_HW_WAVE_ID:
      case latte::SQ_ALU_SRC_SIMD_ID:
      case latte::SQ_ALU_SRC_SE_ID:
      case latte::SQ_ALU_SRC_HW_THREADGRP_ID:
      case latte::SQ_ALU_SRC_WAVE_ID_IN_GRP:
      case latte::SQ_ALU_SRC_NUM_THREADGRP_WAVES:
      case latte::SQ_ALU_SRC_HW_ALU_ODD:
      case latte::SQ_ALU_SRC_LOOP_IDX:
      case latte::SQ_ALU_SRC_PARAM_BASE_ADDR:
      case latte::SQ_ALU_SRC_NEW_PRIM_MASK:
      case latte::SQ_ALU_SRC_PRIM_MASK_HI:
      case latte::SQ_ALU_SRC_PRIM_MASK_LO:
      case latte::SQ_ALU_SRC_1_DBL_L:
      case latte::SQ_ALU_SRC_1_DBL_M:
      case latte::SQ_ALU_SRC_0_5_DBL_L:
      case latte::SQ_ALU_SRC_0_5_DBL_M:
      case latte::SQ_ALU_SRC_0:
      case latte::SQ_ALU_SRC_1:
      case latte::SQ_ALU_SRC_0_5:
         // Other is value cast
         if (src.type == ValueType::Int) {
            state.out << "int(";
            didTypeConversion = true;
         } else if (src.type == ValueType::Uint) {
            state.out << "uint(";
            didTypeConversion = true;
         }
         break;
      case latte::SQ_ALU_SRC_1_INT:
      case latte::SQ_ALU_SRC_M_1_INT:
      case latte::SQ_ALU_SRC_LITERAL:
         // No need for type conversion
         break;
      }
   }

   // Output register name
   if (src.sel >= latte::SQ_ALU_REGISTER_0 && src.sel <= latte::SQ_ALU_REGISTER_127) {
      state.out << 'R' << (src.sel - latte::SQ_ALU_REGISTER_0);
      doChannelSelect = true;
   } else if (src.sel >= latte::SQ_ALU_KCACHE_BANK0_0 && src.sel <= latte::SQ_ALU_KCACHE_BANK0_31) {
      auto addr = ins->parent->kcache[0].addr;
      auto bank = ins->parent->kcache[0].bank;
      auto mode = ins->parent->kcache[0].mode;
      auto id = (addr * 16) + (src.sel - latte::SQ_ALU_KCACHE_BANK0_0);

      if (mode == latte::SQ_CF_KCACHE_LOCK_LOOP_INDEX) {
         throw std::logic_error("Unimplemented kcache lock mode SQ_CF_KCACHE_LOCK_LOOP_INDEX");
      } else if (mode == latte::SQ_CF_KCACHE_NOP) {
         throw std::logic_error("Invalid kcache lock mode SQ_CF_KCACHE_NOP");
      }

      if (state.shader->type == latte::Shader::Vertex) {
         state.out << "VB[" << bank << "].values[" << id;
      } else if (state.shader->type == latte::Shader::Pixel) {
         state.out << "PB[" << bank << "].values[" << id;
      } else {
         throw std::logic_error("Unexpected shader type");
      }

      if (src.rel) {
         state.out << " + ";
         translateIndex(state, ins->indexMode);
      }

      state.out << ']';
      doChannelSelect = true;
   } else if (src.sel >= latte::SQ_ALU_KCACHE_BANK1_0 && src.sel <= latte::SQ_ALU_KCACHE_BANK1_31) {
      doChannelSelect = true;
   } else if (src.sel >= latte::SQ_ALU_SRC_CONST_FILE_0 && src.sel <= latte::SQ_ALU_SRC_CONST_FILE_255) {
      if (state.shader->type == latte::Shader::Vertex) {
         state.out << "VR[" << (src.sel - latte::SQ_ALU_SRC_CONST_FILE_0);
      } else if (state.shader->type == latte::Shader::Pixel) {
         state.out << "PR[" << (src.sel - latte::SQ_ALU_SRC_CONST_FILE_0);
      } else {
         throw std::runtime_error("Unexpected shader type for ConstantFile");
      }

      if (src.rel) {
         state.out << " + ";
         translateIndex(state, ins->indexMode);
      }

      state.out << ']';
      doChannelSelect = true;
   } else {
      switch (src.sel) {
      case latte::SQ_ALU_SRC_PV:
         state.out << "PV";
         doChannelSelect = true;
         break;
      case latte::SQ_ALU_SRC_PS:
         state.out << "PS";
         break;
      case latte::SQ_ALU_SRC_0:
         state.out << "0.0f";
         break;
      case latte::SQ_ALU_SRC_1:
         state.out << "1.0f";
         break;
      case latte::SQ_ALU_SRC_0_5:
         state.out << "0.5f";
         break;
      case latte::SQ_ALU_SRC_1_INT:
         state.out << "1";
         break;
      case latte::SQ_ALU_SRC_M_1_INT:
         state.out << "-1";
         break;
      case latte::SQ_ALU_SRC_LITERAL:
         if (src.type == ValueType::Float) {
            state.out.write("{:.6f}f", src.literalFloat);
         } else if (src.type == ValueType::Int) {
            state.out << src.literalInt;
         } else if (src.type == ValueType::Uint) {
            state.out << src.literalUint;
         }
         break;
      case latte::SQ_ALU_SRC_1_DBL_L:
      case latte::SQ_ALU_SRC_1_DBL_M:
      case latte::SQ_ALU_SRC_0_5_DBL_L:
      case latte::SQ_ALU_SRC_0_5_DBL_M:
      case latte::SQ_ALU_SRC_LDS_OQ_A:
      case latte::SQ_ALU_SRC_LDS_OQ_B:
      case latte::SQ_ALU_SRC_LDS_OQ_A_POP:
      case latte::SQ_ALU_SRC_LDS_OQ_B_POP:
      case latte::SQ_ALU_SRC_LDS_DIRECT_A:
      case latte::SQ_ALU_SRC_LDS_DIRECT_B:
      case latte::SQ_ALU_SRC_TIME_HI:
      case latte::SQ_ALU_SRC_TIME_LO:
      case latte::SQ_ALU_SRC_MASK_HI:
      case latte::SQ_ALU_SRC_MASK_LO:
      case latte::SQ_ALU_SRC_HW_WAVE_ID:
      case latte::SQ_ALU_SRC_SIMD_ID:
      case latte::SQ_ALU_SRC_SE_ID:
      case latte::SQ_ALU_SRC_HW_THREADGRP_ID:
      case latte::SQ_ALU_SRC_WAVE_ID_IN_GRP:
      case latte::SQ_ALU_SRC_NUM_THREADGRP_WAVES:
      case latte::SQ_ALU_SRC_HW_ALU_ODD:
      case latte::SQ_ALU_SRC_LOOP_IDX:
      case latte::SQ_ALU_SRC_PARAM_BASE_ADDR:
      case latte::SQ_ALU_SRC_NEW_PRIM_MASK:
      case latte::SQ_ALU_SRC_PRIM_MASK_HI:
      case latte::SQ_ALU_SRC_PRIM_MASK_LO:
      default:
         throw std::logic_error(fmt::format("Unsupported register used {}", src.sel));
         break;
      }
   }

   if (src.rel) {
      if (src.sel < latte::SQ_ALU_SRC_CONST_FILE_0 || src.sel > latte::SQ_ALU_SRC_CONST_FILE_255) {
         throw std::logic_error("Relative AluSource is only supported with uniform registers");
      }
   }

   if (doChannelSelect) {
      state.out << '.';
      translateChannel(state, src.chan);
   }

   if (didTypeConversion) {
      state.out << ')';
   }

   if (src.absolute) {
      state.out << ')';
   }
}

void translateAluSourceVector(GenerateState &state,
                              const AluInstruction *ins,
                              const AluSource &srcX,
                              const AluSource &srcY,
                              const AluSource &srcZ,
                              const AluSource &srcW)
{
   state.out << "vec4(";
   translateAluSource(state, ins, srcX);
   state.out << ", ";
   translateAluSource(state, ins, srcY);
   state.out << ", ";
   translateAluSource(state, ins, srcZ);
   state.out << ", ";
   translateAluSource(state, ins, srcW);
   state.out << ')';
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
