#pragma once
#include "latte/latte_instructions.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <algorithm>
#include <fmt/format.h>
#include <gsl.h>
#include <stdexcept>

namespace latte
{

inline bool
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

inline bool
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


inline SQ_ALU_FLAGS
getInstructionFlags(const AluInst &inst)
{
   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      return getInstructionFlags(inst.op2.ALU_INST());
   } else {
      return getInstructionFlags(inst.op3.ALU_INST());
   }
}

inline uint32_t
getInstructionNumSrcs(const AluInst &inst)
{
   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      return getInstructionNumSrcs(inst.op2.ALU_INST());
   } else {
      return getInstructionNumSrcs(inst.op3.ALU_INST());
   }
}

inline bool
isTranscendentalOnlyInst(const AluInst &inst)
{
   auto flags = getInstructionFlags(inst);
   return isTranscendentalOnly(flags);
}

inline bool
isVectorOnlyInst(const AluInst &inst)
{
   auto flags = getInstructionFlags(inst);
   return isVectorOnly(flags);
}

inline bool
isReductionInst(const AluInst &inst)
{
   auto flags = getInstructionFlags(inst);
   return !!(flags & SQ_ALU_FLAG_REDUCTION);
}

struct AluGroup
{
   AluGroup(const latte::AluInst *group)
   {
      auto instructionCount = 0u;
      auto literalCount = 0u;

      for (instructionCount = 1u; instructionCount <= 5u; ++instructionCount) {
         auto &inst = group[instructionCount - 1];
         auto srcCount = 0u;

         if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
            srcCount = getInstructionNumSrcs(inst.op2.ALU_INST());
         } else {
            srcCount = getInstructionNumSrcs(inst.op3.ALU_INST());
         }

         if (srcCount > 0 && inst.word0.SRC0_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<unsigned>(literalCount, 1u + inst.word0.SRC0_CHAN());
         }

         if (srcCount > 1 && inst.word0.SRC1_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<unsigned>(literalCount, 1u + inst.word0.SRC1_CHAN());
         }

         if (srcCount > 2 && inst.op3.SRC2_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<unsigned>(literalCount, 1u + inst.op3.SRC2_CHAN());
         }

         if (inst.word0.LAST()) {
            break;
         }
      }

      instructions = gsl::make_span(group, instructionCount);
      literals = gsl::make_span(reinterpret_cast<const uint32_t *>(group + instructionCount), literalCount);
   }

   size_t getNextSlot(size_t slot)
   {
      slot += instructions.size();
      slot += (literals.size() + 1) / 2;
      return slot;
   }

   gsl::span<const latte::AluInst> instructions;
   gsl::span<const uint32_t> literals;
};

struct AluGroupUnits
{
   SQ_CHAN addInstructionUnit(const latte::AluInst &inst)
   {
      SQ_ALU_FLAGS flags;
      SQ_CHAN unit = inst.word1.DST_CHAN();

      if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         flags = getInstructionFlags(inst.op2.ALU_INST());
      } else {
         flags = getInstructionFlags(inst.op3.ALU_INST());
      }

      if (isTranscendentalOnly(flags)) {
         unit = SQ_CHAN::T;
      } else if (isVectorOnly(flags)) {
         unit = unit;
      } else if (units[unit]) {
         unit = SQ_CHAN::T;
      }

      decaf_assert(!units[unit], fmt::format("Clause instruction unit collision for unit {}", unit));
      units[unit] = true;
      return unit;
   }

   bool units[5] = { false, false, false, false, false };
};

struct AluInstructionGroup
{
   std::array<const AluInst*, 5> units;
   gsl::span<const uint32_t> literals;
};

class AluClauseParser
{
public:
   AluClauseParser(gsl::span<const AluInst> slots, bool aluInstPreferVector)
      : mSlots(slots), mAluInstPreferVector(aluInstPreferVector), mIndex(0)
   {
   }

   AluInstructionGroup
      readOneGroup()
   {
      AluInstructionGroup group = { 0 };
      auto literalCount = 0;

      while (true) {
         auto &inst = mSlots[mIndex++];

         // Read some internal data about the instruction
         uint32_t srcCount = getInstructionNumSrcs(inst);

         // Calculate the number of literals used by this instruction
         if (srcCount > 0 && inst.word0.SRC0_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<uint32_t>(literalCount, 1u + inst.word0.SRC0_CHAN());
         }

         if (srcCount > 1 && inst.word0.SRC1_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<uint32_t>(literalCount, 1u + inst.word0.SRC1_CHAN());
         }

         if (srcCount > 2 && inst.op3.SRC2_SEL() == SQ_ALU_SRC::LITERAL) {
            literalCount = std::max<uint32_t>(literalCount, 1u + inst.op3.SRC2_CHAN());
         }

         // This logic comes straight from the reference pages.
         auto elem = inst.word1.DST_CHAN();
         bool isLast = inst.word0.LAST();
         bool isTrans;

         if (isTranscendentalOnlyInst(inst)) {
            isTrans = true;
         } else if (isVectorOnlyInst(inst)) {
            isTrans = false;
         } else if (group.units[elem] || (!mAluInstPreferVector && isLast)) {
            isTrans = true;
         } else {
            isTrans = false;
         }

         if (isTrans) {
            decaf_check(!group.units[SQ_CHAN::T]);
            group.units[SQ_CHAN::T] = &inst;
         } else {
            decaf_check(!group.units[elem]);
            group.units[elem] = &inst;
         }

         // If this is marked as the last instruction, we are done
         if (isLast) {
            break;
         }
      }

      // We decode the literals using subspan to allow GSL to perform proper
      //  span bounds checking for us!
      if (literalCount > 0) {
         auto literalSlotCount = align_up(literalCount, 2) / 2;
         auto literalSpan = mSlots.subspan(mIndex, literalSlotCount);
         group.literals = gsl::make_span(reinterpret_cast<const uint32_t*>(&literalSpan[0]), literalCount);
         mIndex += literalSlotCount;
      }

      // We place NOP data into all the ALU units that are not used.  This
      //  permits us to simplify the logical handling to avoid NULL checks.
      static const uint32_t NopInstData[] = { 0x00000000, 0x00000d00 };
      static const auto NopInstPtr = reinterpret_cast<const AluInst *>(NopInstData);

      for (auto i = 0; i < 5; ++i) {
         if (!group.units[i]) {
            group.units[i] = NopInstPtr;
         }
      }

      return group;
   }

   bool
      isEndOfClause()
   {
      decaf_check(mIndex <= mSlots.size());
      return mIndex >= mSlots.size();
   }

private:
   bool mAluInstPreferVector;
   size_t mIndex;
   gsl::span<const AluInst> mSlots;

};

enum class GprIndexMode : uint32_t
{
   None,
   AR_X,
   AL
};

enum class CfileIndexMode : uint32_t
{
   None,
   AR_X,
   AR_Y,
   AR_Z,
   AR_W,
   AL
};

enum class CbufferIndexMode : uint32_t
{
   None,
   AL
};

enum class VarRefType : uint32_t
{
   UNKNOWN,
   FLOAT,
   INT,
   UINT
};

struct GprRef
{
   uint32_t number;
   GprIndexMode indexMode;

   inline void next()
   {
      number++;
   }
};

struct CfileRef
{
   uint32_t index;
   CfileIndexMode indexMode;
};

struct CbufferRef
{
   uint32_t bufferId;
   uint32_t index;
   CbufferIndexMode indexMode;
};

struct GprMaskRef
{
   GprRef gpr;
   std::array<SQ_SEL, 4> mask;
};

struct GprSelRef
{
   GprRef gpr;
   SQ_SEL sel;
};

struct GprChanRef
{
   GprRef gpr;
   SQ_CHAN chan;
};

struct CfileChanRef
{
   CfileRef cfile;
   SQ_CHAN chan;
};

struct CbufferChanRef
{
   CbufferRef cbuffer;
   SQ_CHAN chan;
};

struct PrevValRef
{
   SQ_CHAN unit;
};

struct ValueRef
{
   union
   {
      int32_t intValue;
      uint32_t uintValue;
      float floatValue;
   };
};

struct ExportRef
{
   enum class Type : uint32_t
   {
      Position,
      Param,
      Pixel,
      PixelWithFog,
      ComputedZ,
      Stream0Write,
      Stream1Write,
      Stream2Write,
      Stream3Write,
      VsGsRingWrite,
      GsDcRingWrite
   };

   Type type;
   uint32_t dataStride;
   uint32_t elemCount;
   uint32_t arrayBase;
   uint32_t arraySize;
   uint32_t indexGpr;

   inline void next()
   {
      // Normal exports have a specific behaviour that they increment
      // by one and have a elemCount of 1, even though they write vec4's
      if (type < Type::Stream0Write) {
         decaf_check(elemCount == 1);
      }

      arrayBase += elemCount;
   }
};

struct ExportMaskRef
{
   ExportRef output;
   std::array<SQ_SEL, 4> mask;
};

struct SrcVarRef
{
   enum class Type : uint32_t
   {
      GPR,
      CBUFFER,
      CFILE,
      PREVRES,
      VALUE
   };

   Type type;
   union
   {
      GprChanRef gprChan;
      CbufferChanRef cbufferChan;
      CfileChanRef cfileChan;
      PrevValRef prevres;
      ValueRef value;
   };
   VarRefType valueType;
   bool isAbsolute;
   bool isNegated;
};

inline GprRef
makeGprRef(uint32_t gprNum, SQ_REL rel = SQ_REL::ABS, SQ_INDEX_MODE indexMode = SQ_INDEX_MODE::AR_X)
{
   GprRef gpr;

   gpr.number = gprNum;

   if (rel == SQ_REL::REL) {
      switch (indexMode) {
      case SQ_INDEX_MODE::AR_X:
      case SQ_INDEX_MODE::AR_Y:
      case SQ_INDEX_MODE::AR_Z:
      case SQ_INDEX_MODE::AR_W:
         gpr.indexMode = GprIndexMode::AR_X;
         break;
      case SQ_INDEX_MODE::LOOP:
         gpr.indexMode = GprIndexMode::AL;
         break;
      default:
         decaf_abort("Unexpected GPR index mode");
      }
   } else {
      gpr.indexMode = GprIndexMode::None;
   }

   return gpr;
}

inline CfileRef
makeCfileRef(uint32_t offset, SQ_REL rel, SQ_INDEX_MODE indexMode)
{
   CfileRef cfile;

   cfile.index = offset;

   if (rel == SQ_REL::REL) {
      switch (indexMode) {
      case SQ_INDEX_MODE::AR_X:
         cfile.indexMode = CfileIndexMode::AR_X;
         break;
      case SQ_INDEX_MODE::AR_Y:
         cfile.indexMode = CfileIndexMode::AR_Y;
         break;
      case SQ_INDEX_MODE::AR_Z:
         cfile.indexMode = CfileIndexMode::AR_Z;
         break;
      case SQ_INDEX_MODE::AR_W:
         cfile.indexMode = CfileIndexMode::AR_W;
         break;
      case SQ_INDEX_MODE::LOOP:
         cfile.indexMode = CfileIndexMode::AL;
         break;
      default:
         decaf_abort("Unexpected GPR index mode");
      }
   } else {
      cfile.indexMode = CfileIndexMode::None;
   }

   return cfile;
}

inline CbufferRef
makeCbufferRef(uint32_t offset, SQ_CF_KCACHE_MODE mode, uint32_t bank, uint32_t addr)
{
   CbufferRef cbuffer;

   uint32_t lockedCount;
   CbufferIndexMode cbufferIndexMode;
   switch (mode) {
   case SQ_CF_KCACHE_MODE::NOP:
      lockedCount = 0;
      cbufferIndexMode = CbufferIndexMode::None;
      break;
   case SQ_CF_KCACHE_MODE::LOCK_1:
      lockedCount = 16;
      cbufferIndexMode = CbufferIndexMode::None;
      break;
   case SQ_CF_KCACHE_MODE::LOCK_2:
      lockedCount = 32;
      cbufferIndexMode = CbufferIndexMode::None;
      break;
   case SQ_CF_KCACHE_MODE::LOCK_LOOP_INDEX:
      lockedCount = 32;
      cbufferIndexMode = CbufferIndexMode::AL;
      break;
   default:
      decaf_abort("Unexpected KCACHE_MODE");
   }

   decaf_check(offset < lockedCount);
   cbuffer.bufferId = bank;
   cbuffer.index = addr * 16 + offset;
   cbuffer.indexMode = cbufferIndexMode;

   return cbuffer;
}

inline SrcVarRef
makeSrcVar(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_ALU_SRC selId, SQ_CHAN chan, SQ_REL rel, bool abs, bool neg, SQ_INDEX_MODE indexMode, VarRefType type)
{
   SrcVarRef out;

   out.isAbsolute = abs;
   out.isNegated = neg;
   out.valueType = type;

   if (selId >= 0 && selId < 128) {
      out.type = SrcVarRef::Type::GPR;
      out.gprChan.gpr = makeGprRef(selId, rel, indexMode);
      out.gprChan.chan = chan;
   } else if (selId >= 128 && selId < 160) {
      // Cannot use relative indexing to a Cbuffer, instead you have to do it
      //  at the lock level, as below.
      decaf_check(rel == SQ_REL::ABS);

      out.type = SrcVarRef::Type::CBUFFER;
      out.cbufferChan.cbuffer = makeCbufferRef(selId - 128,
                                               cf.alu.word0.KCACHE_MODE0(), cf.alu.word0.KCACHE_BANK0(), cf.alu.word1.KCACHE_ADDR0());
      out.cbufferChan.chan = chan;
   } else if (selId >= 160 && selId < 192) {
      // Cannot use relative indexing to a Cbuffer, instead you have to do it
      //  at the lock level, as below.
      decaf_check(rel == SQ_REL::ABS);

      out.type = SrcVarRef::Type::CBUFFER;
      out.cbufferChan.cbuffer = makeCbufferRef(selId - 160,
                                               cf.alu.word1.KCACHE_MODE1(), cf.alu.word0.KCACHE_BANK1(), cf.alu.word1.KCACHE_ADDR1());
      out.cbufferChan.chan = chan;
   } else if (selId >= 256 && selId < 512) {
      auto cfileIndex = selId - 256;
      out.type = SrcVarRef::Type::CFILE;
      out.cfileChan.cfile = makeCfileRef(selId - 256, rel, indexMode);
      out.cfileChan.chan = chan;
   } else {
      switch (selId) {
      case latte::SQ_ALU_SRC::LDS_OQ_A:
         decaf_abort("Unsupported ALU SRC: LDS_OQ_A");
      case latte::SQ_ALU_SRC::LDS_OQ_B:
         decaf_abort("Unsupported ALU SRC: LDS_OQ_B");
      case latte::SQ_ALU_SRC::LDS_OQ_A_POP:
         decaf_abort("Unsupported ALU SRC: LDS_OQ_A_POP");
      case latte::SQ_ALU_SRC::LDS_OQ_B_POP:
         decaf_abort("Unsupported ALU SRC: LDS_OQ_B_POP");
      case latte::SQ_ALU_SRC::LDS_DIRECT_A:
         decaf_abort("Unsupported ALU SRC: LDS_DIRECT_A");
      case latte::SQ_ALU_SRC::LDS_DIRECT_B:
         decaf_abort("Unsupported ALU SRC: LDS_DIRECT_B");
      case latte::SQ_ALU_SRC::TIME_HI:
         decaf_abort("Unsupported ALU SRC: TIME_HI");
      case latte::SQ_ALU_SRC::TIME_LO:
         decaf_abort("Unsupported ALU SRC: TIME_LO");
      case latte::SQ_ALU_SRC::MASK_HI:
         decaf_abort("Unsupported ALU SRC: MASK_HI");
      case latte::SQ_ALU_SRC::MASK_LO:
         decaf_abort("Unsupported ALU SRC: MASK_LO");
      case latte::SQ_ALU_SRC::HW_WAVE_ID:
         decaf_abort("Unsupported ALU SRC: HW_WAVE_ID");
      case latte::SQ_ALU_SRC::SIMD_ID:
         decaf_abort("Unsupported ALU SRC: SIMD_ID");
      case latte::SQ_ALU_SRC::SE_ID:
         decaf_abort("Unsupported ALU SRC: SE_ID");
      case latte::SQ_ALU_SRC::HW_THREADGRP_ID:
         decaf_abort("Unsupported ALU SRC: HW_THREADGRP_ID");
      case latte::SQ_ALU_SRC::WAVE_ID_IN_GRP:
         decaf_abort("Unsupported ALU SRC: WAVE_ID_IN_GRP");
      case latte::SQ_ALU_SRC::NUM_THREADGRP_WAVES:
         decaf_abort("Unsupported ALU SRC: NUM_THREADGRP_WAVES");
      case latte::SQ_ALU_SRC::HW_ALU_ODD:
         decaf_abort("Unsupported ALU SRC: HW_ALU_ODD");
      case latte::SQ_ALU_SRC::LOOP_IDX:
         decaf_abort("Unsupported ALU SRC: LOOP_IDX");
      case latte::SQ_ALU_SRC::PARAM_BASE_ADDR:
         decaf_abort("Unsupported ALU SRC: PARAM_BASE_ADDR");
      case latte::SQ_ALU_SRC::NEW_PRIM_MASK:
         decaf_abort("Unsupported ALU SRC: NEW_PRIM_MASK");
      case latte::SQ_ALU_SRC::PRIM_MASK_HI:
         decaf_abort("Unsupported ALU SRC: PRIM_MASK_HI");
      case latte::SQ_ALU_SRC::PRIM_MASK_LO:
         decaf_abort("Unsupported ALU SRC: PRIM_MASK_LO");
      case latte::SQ_ALU_SRC::IMM_1_DBL_L:
         decaf_abort("Unsupported ALU SRC: IMM_1_DBL_L");
      case latte::SQ_ALU_SRC::IMM_1_DBL_M:
         decaf_abort("Unsupported ALU SRC: IMM_1_DBL_M");
      case latte::SQ_ALU_SRC::IMM_0_5_DBL_L:
         decaf_abort("Unsupported ALU SRC: IMM_0_5_DBL_L");
      case latte::SQ_ALU_SRC::IMM_0_5_DBL_M:
         decaf_abort("Unsupported ALU SRC: IMM_0_5_DBL_M");
      case latte::SQ_ALU_SRC::IMM_0:
         out.type = SrcVarRef::Type::VALUE;
         out.value.floatValue = 0.0f;
         break;
      case latte::SQ_ALU_SRC::IMM_1:
         out.type = SrcVarRef::Type::VALUE;
         out.value.floatValue = 1.0f;
         break;
      case latte::SQ_ALU_SRC::IMM_1_INT:
         out.type = SrcVarRef::Type::VALUE;
         out.value.uintValue = 1;
         break;
      case latte::SQ_ALU_SRC::IMM_M_1_INT:
         out.type = SrcVarRef::Type::VALUE;
         out.value.uintValue = -1;
         break;
      case latte::SQ_ALU_SRC::IMM_0_5:
         out.type = SrcVarRef::Type::VALUE;
         out.value.floatValue = 0.5f;
         break;
      case latte::SQ_ALU_SRC::LITERAL:
         out.type = SrcVarRef::Type::VALUE;
         out.value.uintValue = group.literals[chan];
         break;
      case latte::SQ_ALU_SRC::PV:
         out.type = SrcVarRef::Type::PREVRES;
         out.prevres.unit = chan;
         break;
      case latte::SQ_ALU_SRC::PS:
         out.type = SrcVarRef::Type::PREVRES;
         out.prevres.unit = SQ_CHAN::T;
         break;
      default:
         decaf_abort(fmt::format("Unexpected ALU SRC encountered: {}", selId));
      }
   }

   return out;
}

inline SrcVarRef
makeSrcVar(const ControlFlowInst &cf, const AluInstructionGroup &group, const AluInst &inst, uint32_t srcIndex, VarRefType valueType = VarRefType::UNKNOWN)
{
   auto instFlags = getInstructionFlags(inst);
   if (valueType == VarRefType::UNKNOWN) {
      // TODO: Rewrite GLSL generator to input the expected type information.
      // This would allow us to remove the UNKNOWN type and just use that inputs.
      valueType = VarRefType::FLOAT;
      if (instFlags & SQ_ALU_FLAG_INT_IN) {
         valueType = VarRefType::INT;
      } else if (instFlags & SQ_ALU_FLAG_UINT_IN) {
         valueType = VarRefType::UINT;
      }
   } else {
      if (instFlags & (SQ_ALU_FLAG_INT_IN | SQ_ALU_FLAG_UINT_IN)) {
         decaf_check(valueType == VarRefType::INT || valueType == VarRefType::UINT);
      } else {
         decaf_check(valueType == VarRefType::FLOAT);
      }
   }

   if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
      switch (srcIndex) {
      case 0:
         return makeSrcVar(cf, group,
                           inst.word0.SRC0_SEL(),
                           inst.word0.SRC0_CHAN(),
                           inst.word0.SRC0_REL(),
                           inst.op2.SRC0_ABS(),
                           inst.word0.SRC0_NEG(),
                           inst.word0.INDEX_MODE(),
                           valueType);
      case 1:
         return makeSrcVar(cf, group,
                           inst.word0.SRC1_SEL(),
                           inst.word0.SRC1_CHAN(),
                           inst.word0.SRC1_REL(),
                           inst.op2.SRC1_ABS(),
                           inst.word0.SRC1_NEG(),
                           inst.word0.INDEX_MODE(),
                           valueType);
      default:
         decaf_abort("Invalid source var index");
      }
   } else {
      switch (srcIndex) {
      case 0:
         return makeSrcVar(cf, group,
                           inst.word0.SRC0_SEL(),
                           inst.word0.SRC0_CHAN(),
                           inst.word0.SRC0_REL(),
                           false,
                           inst.word0.SRC0_NEG(),
                           inst.word0.INDEX_MODE(),
                           valueType);
      case 1:
         return makeSrcVar(cf, group,
                           inst.word0.SRC1_SEL(),
                           inst.word0.SRC1_CHAN(),
                           inst.word0.SRC1_REL(),
                           false,
                           inst.word0.SRC1_NEG(),
                           inst.word0.INDEX_MODE(),
                           valueType);
      case 2:
         return makeSrcVar(cf, group,
                           inst.op3.SRC2_SEL(),
                           inst.op3.SRC2_CHAN(),
                           inst.op3.SRC2_REL(),
                           false,
                           inst.op3.SRC2_NEG(),
                           inst.word0.INDEX_MODE(),
                           valueType);
      default:
         decaf_abort("Invalid source var index");
      }
   }
}

inline uint32_t
condenseSwizzleMask(std::array<SQ_SEL, 4> &source, std::array<SQ_CHAN, 4> &dest, uint32_t numSwizzle)
{
   uint32_t numSwizzleOut = 0;

   for (auto i = 0u; i < numSwizzle; ++i) {
      if (source[i] != SQ_SEL::SEL_MASK) {
         source[numSwizzleOut] = source[i];
         dest[numSwizzleOut] = static_cast<SQ_CHAN>(i);
         numSwizzleOut++;
      }
   }

   return numSwizzleOut;
}

inline bool
simplifySwizzle(std::array<SQ_CHAN, 4> &out, const std::array<SQ_SEL, 4> &source, uint32_t numSwizzle)
{
   for (auto i = 0u; i < numSwizzle; ++i) {
      switch (source[i]) {
      case SQ_SEL::SEL_X:
         out[i] = SQ_CHAN::X;
         break;
      case SQ_SEL::SEL_Y:
         out[i] = SQ_CHAN::Y;
         break;
      case SQ_SEL::SEL_Z:
         out[i] = SQ_CHAN::Z;
         break;
      case SQ_SEL::SEL_W:
         out[i] = SQ_CHAN::W;
         break;
      default:
         return false;
      }
   }

   return true;
}

inline bool
isSwizzleFullyMasked(const std::array<SQ_SEL, 4> &dest)
{
   for (auto i = 0u; i < 4; ++i) {
      if (dest[i] != SQ_SEL::SEL_MASK) {
         return false;
      }
   }

   return true;
}

inline bool
isSwizzleFullyUnmasked(const std::array<SQ_SEL, 4> &dest)
{
   for (auto i = 0u; i < 4; ++i) {
      if (dest[i] == SQ_SEL::SEL_MASK) {
         return false;
      }
   }

   return true;
}

inline ExportRef
makeExportRef(SQ_EXPORT_TYPE type, uint32_t arrayBase)
{
   ExportRef out;
   out.dataStride = 0;
   out.arraySize = 1;
   out.elemCount = 1;
   out.indexGpr = -1;

   if (type == SQ_EXPORT_TYPE::POS) {
      if (60 <= arrayBase && arrayBase <= 63) {
         out.type = ExportRef::Type::Position;
         out.arrayBase = arrayBase - 60;
      } else {
         decaf_abort("Unexpected POS EXPORT index");
      }
   } else if (type == SQ_EXPORT_TYPE::PARAM) {
      if (0 <= arrayBase && arrayBase <= 31) {
         out.type = ExportRef::Type::Param;
         out.arrayBase = arrayBase;
      } else {
         decaf_abort("Unexpected PARAM EXPORT index");
      }
   } else if (type == SQ_EXPORT_TYPE::PIXEL) {
      if (0 <= arrayBase && arrayBase <= 7) {
         out.type = ExportRef::Type::Pixel;
         out.arrayBase = arrayBase;
      } else if (16 <= arrayBase && arrayBase <= 23) {
         out.type = ExportRef::Type::PixelWithFog;
         out.arrayBase = arrayBase - 16;
      } else if (arrayBase == 61) {
         out.type = ExportRef::Type::ComputedZ;
         out.arrayBase = 0;
      } else {
         decaf_abort("Unexpected PIXEL EXPORT index");
      }
   } else {
      decaf_abort("Unexpected shader EXPORT type");
   }

   return out;
}

inline ExportRef
_makeGenericMemExportRef(ExportRef::Type refType, SQ_EXPORT_TYPE type,
                         uint32_t indexGpr, uint32_t dataStride,
                         uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
{
   ExportRef out;
   out.type = refType;
   out.dataStride = dataStride;
   out.elemCount = elemCount;
   out.arrayBase = arrayBase;
   out.arraySize = arraySize;
   out.indexGpr = -1;

   switch(static_cast<SQ_MEM_EXPORT_TYPE>(type)) {
   case SQ_MEM_EXPORT_TYPE::WRITE:
      // This is handled implicitly
      break;
   case SQ_MEM_EXPORT_TYPE::WRITE_IND:
      out.indexGpr = indexGpr;
      break;
   default:
      decaf_abort("Unexpected shader MEM EXPORT type");
   }

   return out;
}

inline ExportRef
makeStreamExportRef(SQ_EXPORT_TYPE type, uint32_t indexGpr, uint32_t streamIdx, uint32_t streamStride, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
{
   ExportRef::Type writeType;
   if (streamIdx == 0) {
      writeType = ExportRef::Type::Stream0Write;
   } else if (streamIdx == 1) {
      writeType = ExportRef::Type::Stream1Write;
   } else if (streamIdx == 2) {
      writeType = ExportRef::Type::Stream2Write;
   } else if (streamIdx == 3) {
      writeType = ExportRef::Type::Stream3Write;
   } else {
      decaf_abort("Unexpected stream index for stream memory export");
   }

   return _makeGenericMemExportRef(writeType, type, indexGpr, streamStride, arrayBase, arraySize, elemCount);
}

inline ExportRef
makeVsGsRingExportRef(SQ_EXPORT_TYPE type, uint32_t indexGpr, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
{
   return _makeGenericMemExportRef(ExportRef::Type::VsGsRingWrite, type, indexGpr, 0, arrayBase, arraySize, elemCount);
}

inline ExportRef
makeGsDcRingExportRef(SQ_EXPORT_TYPE type, uint32_t indexGpr, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
{
   return _makeGenericMemExportRef(ExportRef::Type::GsDcRingWrite, type, indexGpr, 0, arrayBase, arraySize, elemCount);
}

} // namespace latte
