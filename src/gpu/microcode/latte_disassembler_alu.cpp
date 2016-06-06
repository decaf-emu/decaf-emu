#include "latte_disassembler.h"
#include "common/bit_cast.h"

namespace latte
{

namespace disassembler
{

static bool
isTranscendentalOnly(SQ_ALU_FLAGS flags)
{
   if (flags & SQ_ALU_FLAG_VECTOR) {
      return false;
   }

   if (flags & SQ_ALU_FLAG_TRANSCENDENTAL) {
      return true;
   }

   return false;
}

static bool
isVectorOnly(SQ_ALU_FLAGS flags)
{
   if (flags & SQ_ALU_FLAG_TRANSCENDENTAL) {
      return false;
   }

   if (flags & SQ_ALU_FLAG_VECTOR) {
      return true;
   }

   return false;
}

static void
disassembleKcache(State &state, SQ_CF_KCACHE_MODE mode, uint32_t bank, uint32_t addr)
{
   switch (mode) {
   case SQ_CF_KCACHE_NOP:
      break;
   case SQ_CF_KCACHE_LOCK_1:
      state.out
         << (16 * addr)
         << " to "
         << (16 * addr + 15);
      break;
   case SQ_CF_KCACHE_LOCK_2:
      state.out
         << (16 * addr)
         << " to "
         << (16 * addr + 31);
      break;
   case SQ_CF_KCACHE_LOCK_LOOP_INDEX:
      state.out
         << "AL+" << (16 * addr)
         << " to "
         << "AL+" << (16 * addr + 31);
      break;
   }
}

static void
disassembleAluSource(State &state, shadir::AluInstruction *aluIns, uint32_t sel, SQ_REL rel, SQ_CHAN chan, uint32_t literalValue, bool negate, bool absolute)
{
   bool useChannel = true;

   if (negate) {
      state.out << "-";
   }

   if (absolute) {
      state.out << "|";
   }

   if (sel >= SQ_ALU_KCACHE_BANK0_FIRST && sel <= SQ_ALU_KCACHE_BANK1_LAST) {
      auto kcache = aluIns->parent->kcache[0];

      if (sel >= SQ_ALU_KCACHE_BANK1_FIRST && sel <= SQ_ALU_KCACHE_BANK1_LAST) {
         kcache = aluIns->parent->kcache[1];
      }

      auto id = kcache.addr * 16 + (sel - SQ_ALU_KCACHE_BANK0_FIRST);

      switch (kcache.mode) {
      case SQ_CF_KCACHE_LOCK_1:
      case SQ_CF_KCACHE_LOCK_2:
         state.out << "KC[" << kcache.bank << "][" << id << "]";
         break;
      case SQ_CF_KCACHE_LOCK_LOOP_INDEX:
         state.out << "KC[" << kcache.bank << "][AL + " << id << "]";
         break;
      default:
         state.out << "KC_UNKNOWN_MODE";
      }
   } else if (sel >= SQ_ALU_REGISTER_FIRST && sel <= SQ_ALU_REGISTER_LAST) {
      state.out << "R" << (sel - SQ_ALU_REGISTER_FIRST);
   } else if (sel >= SQ_ALU_TMP_REGISTER_FIRST && sel <= SQ_ALU_TMP_REGISTER_LAST) {
      state.out << "T" << (SQ_ALU_TMP_REGISTER_LAST - sel);
   } else if (sel >= SQ_ALU_SRC_CONST_FILE_FIRST && sel <= SQ_ALU_SRC_CONST_FILE_LAST) {
      state.out << "C" << (sel - SQ_ALU_SRC_CONST_FILE_FIRST);
   } else {
      useChannel = false;

      switch (sel) {
      case SQ_ALU_SRC_LDS_OQ_A:
         state.out << "LDS_OQ_A";
         break;
      case SQ_ALU_SRC_LDS_OQ_B:
         state.out << "LDS_OQ_B";
         break;
      case SQ_ALU_SRC_LDS_OQ_A_POP:
         state.out << "LDS_OQ_A_POP";
         break;
      case SQ_ALU_SRC_LDS_OQ_B_POP:
         state.out << "LDS_OQ_B_POP";
         break;
      case SQ_ALU_SRC_LDS_DIRECT_A:
         state.out << "LDS_DIRECT_A";
         break;
      case SQ_ALU_SRC_LDS_DIRECT_B:
         state.out << "LDS_DIRECT_B";
         break;
      case SQ_ALU_SRC_TIME_HI:
         state.out << "TIME_HI";
         break;
      case SQ_ALU_SRC_TIME_LO:
         state.out << "TIME_LO";
         break;
      case SQ_ALU_SRC_MASK_HI:
         state.out << "MASK_HI";
         break;
      case SQ_ALU_SRC_MASK_LO:
         state.out << "MASK_LO";
         break;
      case SQ_ALU_SRC_HW_WAVE_ID:
         state.out << "HW_WAVE_ID";
         break;
      case SQ_ALU_SRC_SIMD_ID:
         state.out << "SIMD_ID";
         break;
      case SQ_ALU_SRC_SE_ID:
         state.out << "SE_ID";
         break;
      case SQ_ALU_SRC_HW_THREADGRP_ID:
         state.out << "HW_THREADGRP_ID";
         break;
      case SQ_ALU_SRC_WAVE_ID_IN_GRP:
         state.out << "WAVE_ID_IN_GRP";
         break;
      case SQ_ALU_SRC_NUM_THREADGRP_WAVES:
         state.out << "NUM_THREADGRP_WAVES";
         break;
      case SQ_ALU_SRC_HW_ALU_ODD:
         state.out << "HW_ALU_ODD";
         break;
      case SQ_ALU_SRC_LOOP_IDX:
         state.out << "AL";
         break;
      case SQ_ALU_SRC_PARAM_BASE_ADDR:
         state.out << "PARAM_BASE_ADDR";
         break;
      case SQ_ALU_SRC_NEW_PRIM_MASK:
         state.out << "NEW_PRIM_MASK";
         break;
      case SQ_ALU_SRC_PRIM_MASK_HI:
         state.out << "PRIM_MASK_HI";
         break;
      case SQ_ALU_SRC_PRIM_MASK_LO:
         state.out << "PRIM_MASK_LO";
         break;
      case SQ_ALU_SRC_1_DBL_L:
         state.out << "1.0_L";
         break;
      case SQ_ALU_SRC_1_DBL_M:
         state.out << "1.0_M";
         break;
      case SQ_ALU_SRC_0_5_DBL_L:
         state.out << "0.5_L";
         break;
      case SQ_ALU_SRC_0_5_DBL_M:
         state.out << "0.5_M";
         break;
      case SQ_ALU_SRC_0:
         state.out << "0.0f";
         break;
      case SQ_ALU_SRC_1:
         state.out << "1.0f";
         break;
      case SQ_ALU_SRC_1_INT:
         state.out << "1";
         break;
      case SQ_ALU_SRC_M_1_INT:
         state.out << "-1";
         break;
      case SQ_ALU_SRC_0_5:
         state.out << "0.5f";
         break;
      case SQ_ALU_SRC_LITERAL:
         state.out.write("(0x{:08X}, {})", literalValue, bit_cast<float>(literalValue));
         break;
      case SQ_ALU_SRC_PV:
         state.out << "PV" << (aluIns->groupPC - 1);
         useChannel = true;
         break;
      case SQ_ALU_SRC_PS:
         state.out << "PS" << (aluIns->groupPC - 1);
         break;
      default:
         state.out << "UNKNOWN";
      }
   }

   if (rel) {
      switch (aluIns->indexMode) {
      case SQ_INDEX_AR_X:
         state.out << "[AR.x]";
         break;
      case SQ_INDEX_AR_Y:
         state.out << "[AR.y]";
         break;
      case SQ_INDEX_AR_Z:
         state.out << "[AR.z]";
         break;
      case SQ_INDEX_AR_W:
         state.out << "[AR.w]";
         break;
      case SQ_INDEX_LOOP:
         state.out << "[AL]";
         break;
      default:
         state.out << "[UNKNOWN]";
      }
   }

   if (useChannel) {
      switch (chan) {
      case SQ_CHAN_X:
         state.out << ".x";
         break;
      case SQ_CHAN_Y:
         state.out << ".y";
         break;
      case SQ_CHAN_Z:
         state.out << ".z";
         break;
      case SQ_CHAN_W:
         state.out << ".w";
         break;
      default:
         state.out << ".UNKNOWN";
         break;
      }
   }

   if (absolute) {
      state.out << "|";
   }
}

bool
disassembleControlFlowALU(State &state, shadir::CfAluInstruction *inst)
{
   uint32_t lastGroup = -1;
   state.out.write("{}{:02} ", state.indent, inst->cfPC);

   state.out
      << inst->name
      << " ADDR(" << inst->addr << ")"
      << " CNT(" << inst->clause.size() << ")";


   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   if (inst->wholeQuadMode) {
      state.out << " WHOLE_QUAD";
   }

   for (auto i = 0u; i < inst->kcache.size(); ++i) {
      disassembleKcache(state, inst->kcache[i].mode, inst->kcache[i].bank, inst->kcache[i].addr);
   }

   increaseIndent(state);

   for (auto &child : inst->clause) {
      static char unitName[5] = { 'x', 'y', 'z', 'w', 't' };
      auto aluIns = reinterpret_cast<shadir::AluInstruction *>(child.get());

      if (lastGroup != aluIns->groupPC) {
         state.out << '\n';
         state.out << state.indent << fmt::pad(aluIns->groupPC, 3, ' ');
         lastGroup = aluIns->groupPC;
      } else {
         state.out << state.indent << "   ";
      }

      state.out
         << ' '
         << unitName[aluIns->unit]
         << ": "
         << fmt::pad(aluIns->name, 16, ' ');

      if (!aluIns->writeMask) {
         state.out << " ____";
      } else {
         disassembleAluSource(state,
                              aluIns,
                              aluIns->dst.sel,
                              aluIns->dst.rel,
                              aluIns->dst.chan,
                              0,
                              false,
                              false);
      }

      for (auto i = 0u; i < aluIns->srcCount; ++i) {
         state.out << ", ";
         disassembleAluSource(state,
                              aluIns,
                              aluIns->src[i].sel,
                              aluIns->src[i].rel,
                              aluIns->src[i].chan,
                              aluIns->src[i].literalUint,
                              aluIns->src[i].negate,
                              aluIns->src[i].absolute);
      }

      if (aluIns->dst.clamp) {
         state.out << " CLAMP";
      }

      if (isTranscendentalOnly(aluIns->flags)) {
         switch (aluIns->bankSwizzle) {
         case SQ_ALU_SCL_210:
            state.out << " SCL_210";
            break;
         case SQ_ALU_SCL_122:
            state.out << " SCL_122";
            break;
         case SQ_ALU_SCL_212:
            state.out << " SCL_212";
            break;
         case SQ_ALU_SCL_221:
            state.out << " SCL_221";
            break;
         default:
            state.out << " SCL_UNKNOWN";
         }
      } else {
         switch (aluIns->bankSwizzle) {
         case SQ_ALU_VEC_012:
            // This is default, no need to print
            break;
         case SQ_ALU_VEC_021:
            state.out << " VEC_021";
            break;
         case SQ_ALU_VEC_120:
            state.out << " VEC_120";
            break;
         case SQ_ALU_VEC_102:
            state.out << " VEC_102";
            break;
         case SQ_ALU_VEC_201:
            state.out << " VEC_201";
            break;
         case SQ_ALU_VEC_210:
            state.out << " VEC_210";
            break;
         default:
            state.out << " VEC_UNKNOWN";
         }
      }

      if (aluIns->updateExecuteMask) {
         state.out << " UPDATE_EXECUTE_MASK";

         switch (aluIns->executeMaskOP) {
         case SQ_ALU_EXECUTE_MASK_OP_DEACTIVATE:
            state.out << " DEACTIVATE";
            break;
         case SQ_ALU_EXECUTE_MASK_OP_BREAK:
            state.out << " BREAK";
            break;
         case SQ_ALU_EXECUTE_MASK_OP_CONTINUE:
            state.out << " CONTINUE";
            break;
         case SQ_ALU_EXECUTE_MASK_OP_KILL:
            state.out << " KILL";
            break;
         }
      } else {
         switch (aluIns->outputModifier) {
         case SQ_ALU_OMOD_OFF:
            break;
         case SQ_ALU_OMOD_D2:
            state.out << " OMOD_D2";
            break;
         case SQ_ALU_OMOD_M2:
            state.out << " OMOD_M2";
            break;
         case SQ_ALU_OMOD_M4:
            state.out << " OMOD_M4";
            break;
         default:
            state.out << " OMOD_UNKNOWN";
         }
      }

      if (aluIns->updatePredicate) {
         state.out << " UPDATE_PRED";
      }

      state.out << "\n";
   }

   decreaseIndent(state);
   return true;
}

} // namespace disassembler

} // namespace latte
