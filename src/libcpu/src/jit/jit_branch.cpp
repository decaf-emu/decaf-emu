#include "jit_insreg.h"
#include "../cpu_internal.h"
#include "common/bitutils.h"

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

static void
jit_interrupt_stub()
{
   this_core::checkInterrupts();
}

static void
jit_b_check_interrupt(PPCEmuAssembler& a)
{
   // We need to evict everything in case we call back to the
   //  interrupt handler which is C++ code...
   a.evictAll();

   // Jump to interrupt handler if there is an interrupt
   auto noInterrupt = a.newLabel();

   a.cmp(a.interruptMem, 0);
   a.je(noInterrupt);

   a.mov(a.niaMem, a.genCia + 4);
   a.call(asmjit::Ptr(jit_interrupt_stub));

   a.bind(noInterrupt);
}

void jit_b_direct(PPCEmuAssembler& a, ppcaddr_t addr);

static bool
b(PPCEmuAssembler& a, Instruction instr)
{
   jit_b_check_interrupt(a);

   uint32_t nia = sign_extend<26>(instr.li << 2);
   if (!instr.aa) {
      nia += a.genCia;
   }

   if (instr.lk) {
      auto tmp = a.allocGpTmp().r32();
      a.mov(tmp, a.genCia + 4u);
      a.mov(a.lrMem, tmp);
   }

   jit_b_direct(a, nia);
   return true;
}

template<unsigned flags>
static bool
bcGeneric(PPCEmuAssembler& a, Instruction instr)
{
   jit_b_check_interrupt(a);

   uint32_t bo = instr.bo;
   auto doCondFailLbl = a.newLabel();

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         a.dec(a.ctrMem);

         auto tmp = a.allocGpTmp().r32();
         a.mov(tmp, a.ctrMem);
         a.cmp(tmp, 0);
         if (get_bit<CtrValue>(bo)) {
            a.jne(doCondFailLbl);
         } else {
            a.je(doCondFailLbl);
         }
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         auto tmp = a.allocGpTmp().r32();
         a.mov(tmp, a.loadRegister(a.cr));
         a.and_(tmp, 1 << (31 - instr.bi));
         a.cmp(tmp, 0);

         if (get_bit<CondValue>(bo)) {
            a.je(doCondFailLbl);
         } else {
            a.jne(doCondFailLbl);
         }
      }
   }

   if (instr.lk) {
      auto tmp = a.allocGpTmp().r32();
      a.mov(tmp, a.genCia + 4);
      a.mov(a.lrMem, tmp);
   }

   // Make sure no JMP related instructions end up above
   //   this if-block as we use a JMP instruction with
   //   early exit in the else block...
   if (flags & BcBranchCTR) {
      a.saveAll();

      a.mov(asmjit::x86::ecx, a.ctrMem);
      a.and_(asmjit::x86::ecx, ~0x3);
      a.mov(asmjit::x86::rdx, 0);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
   } else if (flags & BcBranchLR) {
      a.saveAll();

      a.mov(asmjit::x86::ecx, a.lrMem);
      a.and_(asmjit::x86::ecx, ~0x3);
      a.mov(asmjit::x86::rdx, 0);
      a.jmp(asmjit::Ptr(cpu::jit::gFinaleFn));
   } else {
      uint32_t nia = a.genCia + sign_extend<16>(instr.bd << 2);
      jit_b_direct(a, nia);
   }

   a.bind(doCondFailLbl);

   return true;
}

static bool
bc(PPCEmuAssembler& a, Instruction instr)
{
   return bcGeneric<BcCheckCtr | BcCheckCond>(a, instr);
}

static bool
bcctr(PPCEmuAssembler& a, Instruction instr)
{
   return bcGeneric<BcBranchCTR | BcCheckCond>(a, instr);
}

static bool
bclr(PPCEmuAssembler& a, Instruction instr)
{
   return bcGeneric<BcBranchLR | BcCheckCtr | BcCheckCond>(a, instr);
}

void registerBranchInstructions()
{
   RegisterInstruction(b);
   RegisterInstruction(bc);
   RegisterInstruction(bcctr);
   RegisterInstruction(bclr);
}

} // namespace jit

} // namespace cpu
