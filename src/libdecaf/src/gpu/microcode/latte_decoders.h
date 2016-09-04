#pragma once
#include "common/decaf_assert.h"
#include "latte_instructions.h"
#include <algorithm>
#include <gsl.h>
#include <spdlog/fmt/fmt.h>
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

      instructions = gsl::as_span(group, instructionCount);
      literals = gsl::as_span(reinterpret_cast<const uint32_t *>(group + instructionCount), literalCount);
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

} // namespace latte
