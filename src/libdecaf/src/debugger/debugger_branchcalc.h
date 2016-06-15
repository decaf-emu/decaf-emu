#pragma once

#include "common/bitutils.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "libcpu/espresso/espresso_instructionset.h"

static uint32_t
calculateNextInstrB(const cpu::CoreRegs *state, espresso::Instruction instr)
{
   uint32_t nia;
   nia = sign_extend<26>(instr.li << 2);
   if (!instr.aa) {
      nia += state->nia;
   }
   return nia;
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
static uint32_t
calculateNextInstrBc(const cpu::CoreRegs *state, espresso::Instruction instr)
{
   auto bo = instr.bo;
   auto ctr_ok = true;
   auto cond_ok = true;

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         auto ctb = static_cast<uint32_t>(state->ctr - 1 != 0);
         auto ctv = get_bit<CtrValue>(bo);
         ctr_ok = !!(ctb ^ ctv);
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         auto crb = get_bit(state->cr.value, 31 - instr.bi);
         auto crv = get_bit<CondValue>(bo);
         cond_ok = (crb == crv);
      }
   }

   if (ctr_ok && cond_ok) {
      uint32_t nia;

      if (flags & BcBranchCTR) {
         nia = state->ctr & ~0x3;
      } else if (flags & BcBranchLR) {
         nia = state->lr & ~0x3;
      } else {
         nia = sign_extend<16>(instr.bd << 2);

         if (!instr.aa) {
            nia += state->nia;
         }
      }

      return nia;
   }

   return state->nia + 4;
}

static uint32_t calculateNextInstr(const cpu::CoreRegs *state, bool stepOver)
{
   auto instr = mem::read<espresso::Instruction>(state->nia);
   auto data = espresso::decodeInstruction(instr);
   if (stepOver) {
      if (data->id == espresso::InstructionID::b
         || data->id == espresso::InstructionID::bc
         || data->id == espresso::InstructionID::bcctr
         || data->id == espresso::InstructionID::bclr) {
         if (instr.lk) {
            return state->nia + 4;
         }
      }
   }
   if (data->id == espresso::InstructionID::b) {
      return calculateNextInstrB(state, instr);
   } else if (data->id == espresso::InstructionID::bc) {
      return calculateNextInstrBc<BcCheckCtr | BcCheckCond>(state, instr);
   } else if (data->id == espresso::InstructionID::bcctr) {
      return calculateNextInstrBc<BcBranchCTR | BcCheckCond>(state, instr);
   } else if (data->id == espresso::InstructionID::bclr) {
      return calculateNextInstrBc<BcBranchLR | BcCheckCtr | BcCheckCond>(state, instr);
   } else {
      return state->nia + 4;
   }
}