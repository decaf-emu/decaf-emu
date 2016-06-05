#include <cassert>
#include "../espresso/espresso_spr.h"
#include "../cpu_internal.h"
#include "jit_insreg.h"
#include "utils/bitutils.h"
#include "utils/log.h"

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

   switch (spr) {
   case SPR::XER:
      a.mov(a.eax, a.ppcxer);
      break;
   case SPR::LR:
      a.mov(a.eax, a.ppclr);
      break;
   case SPR::CTR:
      a.mov(a.eax, a.ppcctr);
      break;
   case SPR::UGQR0:
      a.mov(a.eax, a.ppcgpr[0]);
      break;
   case SPR::UGQR1:
      a.mov(a.eax, a.ppcgpr[1]);
      break;
   case SPR::UGQR2:
      a.mov(a.eax, a.ppcgpr[2]);
      break;
   case SPR::UGQR3:
      a.mov(a.eax, a.ppcgpr[3]);
      break;
   case SPR::UGQR4:
      a.mov(a.eax, a.ppcgpr[4]);
      break;
   case SPR::UGQR5:
      a.mov(a.eax, a.ppcgpr[5]);
      break;
   case SPR::UGQR6:
      a.mov(a.eax, a.ppcgpr[6]);
      break;
   case SPR::UGQR7:
      a.mov(a.eax, a.ppcgpr[7]);
      break;
   default:
      gLog->error("Invalid mfspr SPR {}", static_cast<uint32_t>(spr));
   }

   a.mov(a.ppcgpr[instr.rD], a.eax);
   return true;
}

// Move to Special Purpose Register
static bool
mtspr(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rD]);

   auto spr = decodeSPR(instr);

   switch (spr) {
   case SPR::XER:
      a.mov(a.ppcxer, a.eax);
      break;
   case SPR::LR:
      a.mov(a.ppclr, a.eax);
      break;
   case SPR::CTR:
      a.mov(a.ppcctr, a.eax);
      break;
   case SPR::UGQR0:
      a.mov(a.ppcgqr[0], a.eax);
      break;
   case SPR::UGQR1:
      a.mov(a.ppcgqr[1], a.eax);
      break;
   case SPR::UGQR2:
      a.mov(a.ppcgqr[2], a.eax);
      break;
   case SPR::UGQR3:
      a.mov(a.ppcgqr[3], a.eax);
      break;
   case SPR::UGQR4:
      a.mov(a.ppcgqr[4], a.eax);
      break;
   case SPR::UGQR5:
      a.mov(a.ppcgqr[5], a.eax);
      break;
   case SPR::UGQR6:
      a.mov(a.ppcgqr[6], a.eax);
      break;
   case SPR::UGQR7:
      a.mov(a.ppcgqr[7], a.eax);
      break;
   default:
      gLog->error("Invalid mtspr SPR {}", static_cast<uint32_t>(spr));
   }

   return true;
}

// Kernel call
static bool
kc(PPCEmuAssembler& a, Instruction instr)
{
   auto id = instr.kcn;

   auto kc = cpu::get_kernel_call(id);
   if (!kc) {
      gLog->error("Encountered invalid Kernel Call ID {}", id);
      a.int3();
      return false;
   }

   a.mov(a.zcx, a.state);
   a.mov(a.zdx, asmjit::Ptr(kc->second));
   a.call(asmjit::Ptr(kc->first));
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
