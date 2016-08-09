#pragma once
#include "common/bitutils.h"
#include "common/decaf_assert.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include <spdlog/fmt/fmt.h>

struct BranchMetaInfo
{
   bool isVariable;
   uint32_t target;
   bool isConditional;
   bool conditionSatisfied;
   bool isCall;
};

static BranchMetaInfo
getBranchMetaB(uint32_t address, espresso::Instruction instr, const cpu::CoreRegs *state)
{
   BranchMetaInfo meta;
   meta.isVariable = false;
   meta.isCall = instr.lk;
   meta.isConditional = false;
   meta.conditionSatisfied = true;

   meta.target = sign_extend<26>(instr.li << 2);
   if (!instr.aa) {
      meta.target += address;
   }

   return meta;
}

// Branch Conditional
enum BoBits
{
   CtrValue = 1,
   NoCheckCtr = 2,
   CondValue = 3,
   NoCheckCond = 4
};

enum BcFlags
{
   BcCheckCtr = 1 << 0,
   BcCheckCond = 1 << 1,
   BcBranchLR = 1 << 2,
   BcBranchCTR = 1 << 3
};

template<unsigned flags>
static BranchMetaInfo
getBranchMetaBX(uint32_t address, espresso::Instruction instr, const cpu::CoreRegs *state)
{
   BranchMetaInfo meta;
   meta.isVariable = false;
   meta.isCall = instr.lk;
   meta.isConditional = false;
   meta.conditionSatisfied = true;
   meta.target = 0xFFFFFFFF;

   auto bo = instr.bo;

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         meta.isConditional = true;
         if (state) {
            auto ctb = static_cast<uint32_t>(state->ctr - 1 != 0);
            auto ctv = get_bit<CtrValue>(bo);
            if (!(ctb ^ ctv)) {
               meta.conditionSatisfied = false;
            }
         }
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         meta.isConditional = true;
         if (state) {
            auto crb = get_bit(state->cr.value, 31 - instr.bi);
            auto crv = get_bit<CondValue>(bo);
            if (crb != crv) {
               meta.conditionSatisfied = false;
            }
         }
      }
   }

   if (flags & BcBranchCTR) {
      meta.isVariable = true;
      if (state) {
         meta.target = state->ctr & ~0x3;
      }
   } else if (flags & BcBranchLR) {
      meta.isVariable = true;
      if (state) {
         meta.target = state->lr & ~0x3;
      }
   } else {
      meta.target = sign_extend<16>(instr.bd << 2);

      if (!instr.aa) {
         meta.target += address;
      }
   }

   return meta;
}

static BranchMetaInfo
getBranchMeta(uint32_t address, espresso::Instruction instr, espresso::InstructionInfo *data, const cpu::CoreRegs *state)
{
   if (data->id == espresso::InstructionID::b) {
      return getBranchMetaB(address, instr, state);
   } else if (data->id == espresso::InstructionID::bc) {
      return getBranchMetaBX<BcCheckCtr | BcCheckCond>(address, instr, state);
   } else if (data->id == espresso::InstructionID::bcctr) {
      return getBranchMetaBX<BcBranchCTR | BcCheckCond>(address, instr, state);
   } else if (data->id == espresso::InstructionID::bclr) {
      return getBranchMetaBX<BcBranchLR | BcCheckCtr | BcCheckCond>(address, instr, state);
   } else {
      decaf_abort(fmt::format("Instruction {} was not a branch", static_cast<int>(data->id)));
   }
}

static bool
isBranchInstr(espresso::InstructionInfo *data)
{
   return data->id == espresso::InstructionID::b
      || data->id == espresso::InstructionID::bc
      || data->id == espresso::InstructionID::bcctr
      || data->id == espresso::InstructionID::bclr;
}

static uint32_t calculateNextInstr(const cpu::CoreRegs *state, bool stepOver)
{
   auto instr = mem::read<espresso::Instruction>(state->nia);
   auto data = espresso::decodeInstruction(instr);

   if (isBranchInstr(data)) {
      auto meta = getBranchMeta(state->nia, instr, data, state);

      if (meta.isCall && stepOver) {
         // This is a call and we are stepping over...
         return state->nia + 4;
      }

      if (meta.conditionSatisfied) {
         return meta.target;
      } else {
         return state->nia + 4;
      }
   } else {
      // This is not a branch instruction
      return state->nia + 4;
   }
}