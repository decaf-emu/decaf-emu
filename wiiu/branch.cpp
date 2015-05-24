#include "bitutils.h"
#include "interpreter.h"

/*
// Branch
INS(b, (), (li), (aa, lk), (opcd == 18), "")
INS(bc, (), (bo, bi, bd), (aa, lk), (opcd == 16), "")
INS(bcctr, (), (bo, bi), (lk), (opcd == 19, xo1 == 528), "")
INS(bclr, (), (bo, bi), (lk), (opcd == 19, xo1 == 16), "")
*/

static void
b(ThreadState *state, Instruction instr)
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
bcGeneric(ThreadState *state, Instruction instr)
{
   uint32_t bo;
   bool ctr_ok, cond_ok;

   bo = instr.bo;
   ctr_ok = true;
   cond_ok = true;

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         state->ctr--;
         ctr_ok = (state->ctr != 0) ^ (get_bit<CtrValue>(bo));
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         auto crb = get_bit(state->cr.value, 31 - instr.bi);
         auto crv = get_bit<CondValue>(bo);
         cond_ok = crb ^ crv;
      }
   }

   if (ctr_ok && cond_ok) {
      uint32_t nia;

      if (flags & BcBranchCTR) {
         nia = state->ctr << 2;
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
}

// Branch Conditional
static void
bc(ThreadState *state, Instruction instr)
{
   return bcGeneric<BcCheckCtr | BcCheckCond>(state, instr);
}

// Branch Conditional to CTR
static void
bcctr(ThreadState *state, Instruction instr)
{
   return bcGeneric<BcBranchCTR | BcCheckCond>(state, instr);
}

// Branch Conditional to LR
static void
bclr(ThreadState *state, Instruction instr)
{
   return bcGeneric<BcBranchLR | BcCheckCtr | BcCheckCond>(state, instr);
}

void
Interpreter::registerBranchInstructions()
{
   RegisterInstruction(b);
   RegisterInstruction(bc);
   RegisterInstruction(bcctr);
   RegisterInstruction(bclr);
}
