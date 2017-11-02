#include <common/bitutils.h>
#include "cpu_internal.h"
#include "interpreter_insreg.h"

static void
b(cpu::Core *state, Instruction instr)
{
   uint32_t nia;
   nia = sign_extend<26>(instr.li << 2);

   if (!instr.aa) {
      nia += state->cia;
   }

   state->nia = nia;

   if (instr.lk) {
      state->lr = state->cia + 4;
   }

   if (cpu::gBranchTraceHandler) {
      cpu::gBranchTraceHandler(state, state->nia);
   }
}

// Branch Conditional
enum BoBits
{
   CtrValue    = 1,
   NoCheckCtr  = 2,
   CondValue   = 3,
   NoCheckCond = 4
};

enum BcFlags
{
   BcCheckCtr  = 1 << 0,
   BcCheckCond = 1 << 1,
   BcBranchLR  = 1 << 2,
   BcBranchCTR = 1 << 3
};

template<unsigned flags>
static void
bcGeneric(cpu::Core *state, Instruction instr)
{
   auto bo = instr.bo;
   auto ctr_ok = true;
   auto cond_ok = true;

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         state->ctr--;

         auto ctb = static_cast<uint32_t>(state->ctr != 0);
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
            nia += state->cia;
         }
      }

      state->nia = nia;

      if (instr.lk) {
         state->lr = state->cia + 4;
      }
   }

   if (cpu::gBranchTraceHandler) {
      cpu::gBranchTraceHandler(state, state->nia);
   }
}

// Branch Conditional
static void
bc(cpu::Core *state, Instruction instr)
{
   return bcGeneric<BcCheckCtr | BcCheckCond>(state, instr);
}

// Branch Conditional to CTR
static void
bcctr(cpu::Core *state, Instruction instr)
{
   return bcGeneric<BcBranchCTR | BcCheckCond>(state, instr);
}

// Branch Conditional to LR
static void
bclr(cpu::Core *state, Instruction instr)
{
   return bcGeneric<BcBranchLR | BcCheckCtr | BcCheckCond>(state, instr);
}

void
cpu::interpreter::registerBranchInstructions()
{
   RegisterInstruction(b);
   RegisterInstruction(bc);
   RegisterInstruction(bcctr);
   RegisterInstruction(bclr);
}
