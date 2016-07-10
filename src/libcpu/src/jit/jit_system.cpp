#include "common/decaf_assert.h"
#include "common/bitutils.h"
#include "common/log.h"
#include "cpu_internal.h"
#include "espresso/espresso_spr.h"
#include "jit_insreg.h"

using espresso::SPR;

namespace cpu
{

namespace jit
{

// Enforce In-Order Execution of I/O
static bool
eieio(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Synchronise
static bool
sync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Instruction Synchronise
static bool
isync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Move from Special Purpose Register
static bool
mfspr(PPCEmuAssembler& a, Instruction instr)
{
   auto spr = decodeSPR(instr);

   auto dst = a.allocRegister(a.gpr[instr.rD]);

   switch (spr) {
   case SPR::XER:
      a.mov(dst, a.loadRegister(a.xer));
      break;
   case SPR::LR:
      a.mov(dst, a.lrMem);
      break;
   case SPR::CTR:
      a.mov(dst, a.ctrMem);
      break;
   case SPR::UGQR0:
      a.mov(dst, a.loadRegister(a.gqr[0]));
      break;
   case SPR::UGQR1:
      a.mov(dst, a.loadRegister(a.gqr[1]));
      break;
   case SPR::UGQR2:
      a.mov(dst, a.loadRegister(a.gqr[2]));
      break;
   case SPR::UGQR3:
      a.mov(dst, a.loadRegister(a.gqr[3]));
      break;
   case SPR::UGQR4:
      a.mov(dst, a.loadRegister(a.gqr[4]));
      break;
   case SPR::UGQR5:
      a.mov(dst, a.loadRegister(a.gqr[5]));
      break;
   case SPR::UGQR6:
      a.mov(dst, a.loadRegister(a.gqr[6]));
      break;
   case SPR::UGQR7:
      a.mov(dst, a.loadRegister(a.gqr[7]));
      break;
   case SPR::UPIR:
      a.mov(dst, a.coreIdMem);
      break;
   default:
      decaf_abort(fmt::format("Invalid mfspr SPR {}", static_cast<uint32_t>(spr)));
   }

   return true;
}

// Move to Special Purpose Register
static bool
mtspr(PPCEmuAssembler& a, Instruction instr)
{
   auto spr = decodeSPR(instr);

   auto src = a.loadRegister(a.gpr[instr.rD]);

   switch (spr) {
   case SPR::XER:
      a.mov(a.allocRegister(a.xer), src);
      break;
   case SPR::LR:
      a.mov(a.lrMem, src);
      break;
   case SPR::CTR:
      a.mov(a.ctrMem, src);
      break;
   case SPR::UGQR0:
      a.mov(a.allocRegister(a.gqr[0]), src);
      break;
   case SPR::UGQR1:
      a.mov(a.allocRegister(a.gqr[1]), src);
      break;
   case SPR::UGQR2:
      a.mov(a.allocRegister(a.gqr[2]), src);
      break;
   case SPR::UGQR3:
      a.mov(a.allocRegister(a.gqr[3]), src);
      break;
   case SPR::UGQR4:
      a.mov(a.allocRegister(a.gqr[4]), src);
      break;
   case SPR::UGQR5:
      a.mov(a.allocRegister(a.gqr[5]), src);
      break;
   case SPR::UGQR6:
      a.mov(a.allocRegister(a.gqr[6]), src);
      break;
   case SPR::UGQR7:
      a.mov(a.allocRegister(a.gqr[7]), src);
      break;
   default:
      decaf_abort(fmt::format("Invalid mtspr SPR {}", static_cast<uint32_t>(spr)));
   }

   return true;
}

static Core *
kc_stub(cpu::KernelCallFunction func, void *userData)
{
   auto core = cpu::this_core::state();
   func(core, userData);
   // We grab new core since it may have changed while executing!
   return cpu::this_core::state();
}

// Kernel call
static bool
kc(PPCEmuAssembler& a, Instruction instr)
{
   auto id = instr.kcn;
   auto kc = cpu::getKernelCall(id);
   decaf_assert(kc, fmt::format("Encountered invalid Kernel Call ID {}", id));

   // Evict all stored register as a KC might read or modify them.
   a.evictAll();

   // Save NIA back to memory in case KC reads/writes it
   a.mov(a.niaMem, a.genCia + 4);

   // Call the KC
   a.mov(asmjit::x86::rcx, asmjit::Ptr(kc->func));
   a.mov(asmjit::x86::rdx, asmjit::Ptr(kc->user_data));
   a.call(asmjit::Ptr(&kc_stub));
   a.mov(a.stateReg, asmjit::x86::rax);

   // Check if the KC adjusted nia.  If it has, we need to return
   //  to the dispatcher.  Note that we assume the cache was already
   //  cleared before this instruction since KC requires that anyways.
   auto niaUnchangedLbl = a.newLabel();

   a.cmp(a.niaMem, a.genCia + 4);
   a.je(niaUnchangedLbl);

   a.mov(asmjit::x86::rcx, a.niaMem);
   a.mov(asmjit::x86::rdx, 0);
   a.jmp(asmjit::Ptr(gFinaleFn));

   a.bind(niaUnchangedLbl);

   return true;
}

void
registerSystemInstructions()
{
   RegisterInstructionFallback(dcbf);
   RegisterInstructionFallback(dcbi);
   RegisterInstructionFallback(dcbst);
   RegisterInstructionFallback(dcbt);
   RegisterInstructionFallback(dcbtst);
   RegisterInstructionFallback(dcbz);
   RegisterInstructionFallback(dcbz_l);
   RegisterInstruction(eieio);
   RegisterInstruction(isync);
   RegisterInstruction(sync);
   RegisterInstruction(mfspr);
   RegisterInstruction(mtspr);
   RegisterInstructionFallback(mftb);
   RegisterInstructionFallback(mfmsr);
   RegisterInstructionFallback(mtmsr);
   RegisterInstructionFallback(mfsr);
   RegisterInstructionFallback(mfsrin);
   RegisterInstructionFallback(mtsr);
   RegisterInstructionFallback(mtsrin);
   RegisterInstruction(kc);
}

} // namespace jit

} // namespace cpu
