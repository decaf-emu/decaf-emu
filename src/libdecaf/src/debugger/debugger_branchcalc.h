#pragma once
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "libcpu/espresso/espresso_instructionset.h"

#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace debugger
{

namespace analysis
{

struct BranchMetaInfo
{
   bool isVariable;
   uint32_t target;
   bool isConditional;
   bool conditionSatisfied;
   bool isCall;
};

static BranchMetaInfo
getBranchMetaB(uint32_t address, espresso::Instruction instr)
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
getBranchMetaBX(uint32_t address, espresso::Instruction instr, uint32_t ctr, uint32_t cr, uint32_t lr)
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

         auto ctb = static_cast<uint32_t>(ctr - 1 != 0);
         auto ctv = get_bit<CtrValue>(bo);
         if (!(ctb ^ ctv)) {
            meta.conditionSatisfied = false;
         }
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         meta.isConditional = true;

         auto crb = get_bit(cr, 31 - instr.bi);
         auto crv = get_bit<CondValue>(bo);
         if (crb != crv) {
            meta.conditionSatisfied = false;
         }
      }
   }

   if (flags & BcBranchCTR) {
      meta.isVariable = true;
      if (ctr) {
         meta.target = ctr & ~0x3;
      }
   } else if (flags & BcBranchLR) {
      meta.isVariable = true;
      if (lr) {
         meta.target = lr & ~0x3;
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
getBranchMeta(uint32_t address, espresso::Instruction instr, espresso::InstructionInfo *data, uint32_t ctr, uint32_t cr, uint32_t lr)
{
   if (data->id == espresso::InstructionID::b) {
      return getBranchMetaB(address, instr);
   } else if (data->id == espresso::InstructionID::bc) {
      return getBranchMetaBX<BcCheckCtr | BcCheckCond>(address, instr, ctr, cr, lr);
   } else if (data->id == espresso::InstructionID::bcctr) {
      return getBranchMetaBX<BcBranchCTR | BcCheckCond>(address, instr, ctr, cr, lr);
   } else if (data->id == espresso::InstructionID::bclr) {
      return getBranchMetaBX<BcBranchLR | BcCheckCtr | BcCheckCond>(address, instr, ctr, cr, lr);
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

} // namespace analysis

} // namespace debugger
