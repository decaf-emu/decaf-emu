#include "jit_insreg.h"
#include "../cpu_internal.h"
#include "utils/bitutils.h"

namespace cpu
{

namespace jit
{

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

void
jit_interrupt_stub(ThreadState *state)
{
   auto core = cpu::this_core::state();

   uint32_t interrupt_flags = core->interrupt.exchange(0);
   if (interrupt_flags != 0) {
      cpu::gInterruptHandler(interrupt_flags);
   }
}

void
jit_b_check_interrupt(PPCEmuAssembler& a, uint32_t cia)
{
   auto noInterrupt = a.newLabel();
   a.cmp(asmjit::X86Mem(a.interruptAddr, 0, 4), 0);
   a.je(noInterrupt);
   a.mov(a.zcx, a.state);
   a.call(asmjit::Ptr(jit_interrupt_stub));
   a.bind(noInterrupt);
}

bool
jit_b(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   jit_b_check_interrupt(a, cia);

   uint32_t nia = sign_extend<26>(instr.li << 2);
   if (!instr.aa) {
      nia += cia;
   }

   if (instr.lk) {
      a.mov(a.eax, cia + 4u);
      a.mov(a.ppclr, a.eax);

      a.mov(a.eax, nia);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
      return true;
   }

   auto i = jumpLabels.find(nia);
   if (i != jumpLabels.end()) {
      a.jmp(i->second);
   } else {
      a.mov(a.eax, nia);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
   }

   return true;
}

template<unsigned flags>
static bool
bcGeneric(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   jit_b_check_interrupt(a, cia);

   uint32_t bo = instr.bo;
   auto doCondFailLbl = a.newLabel();

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         //state->ctr--;
         //ctr_ok = !!((state->ctr != 0) ^ (get_bit<CtrValue>(bo)));

         a.dec(a.ppcctr);

         a.mov(a.eax, a.ppcctr);
         a.cmp(a.eax, 0);
         if (get_bit<CtrValue>(bo)) {
            a.jne(doCondFailLbl);
         } else {
            a.je(doCondFailLbl);
         }
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         //auto crb = get_bit(state->cr.value, 31 - instr.bi);
         //auto crv = get_bit<CondValue>(bo);
         //cond_ok = (crb == crv);

         a.mov(a.eax, a.ppccr);
         a.and_(a.eax, 1 << (31 - instr.bi));
         a.cmp(a.eax, 0);

         if (get_bit<CondValue>(bo)) {
            a.je(doCondFailLbl);
         } else {
            a.jne(doCondFailLbl);
         }
      }
   }

   if (instr.lk) {
      a.mov(a.eax, cia + 4);
      a.mov(a.ppclr, a.eax);
   }

   // Make sure no JMP related instructions end up above
   //   this if-block as we use a JMP instruction with
   //   early exit in the else block...
   if (flags & BcBranchCTR) {
      a.mov(a.eax, a.ppcctr);
      a.and_(a.eax, ~0x3);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
   } else if (flags & BcBranchLR) {
      a.mov(a.eax, a.ppclr);
      a.and_(a.eax, ~0x3);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
   } else {
      uint32_t nia = cia + sign_extend<16>(instr.bd << 2);
      auto i = jumpLabels.find(nia);
      if (i != jumpLabels.end()) {
         a.jmp(i->second);
      } else {
         a.mov(a.eax, nia);
         a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
      }
   }

   a.bind(doCondFailLbl);
   return true;
}

bool
jit_bc(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcCheckCtr | BcCheckCond>(a, instr, cia, jumpLabels);
}

bool
jit_bcctr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcBranchCTR | BcCheckCond>(a, instr, cia, jumpLabels);
}

bool
jit_bclr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcBranchLR | BcCheckCtr | BcCheckCond>(a, instr, cia, jumpLabels);
}

void registerBranchInstructions()
{
   // Branch instructions are handled directly
   //   within the JIT generator...
}

} // namespace jit

} // namespace cpu
