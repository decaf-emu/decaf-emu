#include "jit_insreg.h"
#include "bitutils.h"
#include "jit_float.h"

namespace cpu
{
namespace jit
{
	alignas(16) static const u64 psAbsMask[2] = { 0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL };
	alignas(16) static const u64 psSignBits[2] = { 0x8000000000000000ULL, 0x0000000000000000ULL };
	alignas(16) static const u64 psGeneratedQNaN[2] = { 0x7FF8000000000000ULL, 0x7FF8000000000000ULL };
	alignas(16) static const double half_qnan_and_s32_max[2] = { 0x7FFFFFFF, -0x80000 };

   void
      updateFloatConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2)
   {
      //state->cr.cr1 = state->fpscr.cr1;
      assert(0);
   }

   static bool fctiw(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.movdqa(a.xmm1, a.ptr(half_qnan_and_s32_max));
	   a.minsd(a.xmm1, a.xmm0);
	   a.cvtpd2dq(a.xmm1, a.xmm1);
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm1);

	   return true;
   }

   static bool fctiwz(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.movdqa(a.xmm1, a.ptr(half_qnan_and_s32_max));
	   a.minsd(a.xmm1, a.xmm0);
	   a.cvttpd2dq(a.xmm1, a.xmm1);
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm1);

	   return true;
   }

   static bool frsp(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(xmm0, a.ppcfpr[instr.frB]);
	   a.cvtsd2ss(xmm0, xmm0);
	   a.cvtss2sd(xmm0, xmm0);
	   a.movddup(xmm0, xmm0);
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm0);

	   return true;
   }

   static bool fabs(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.andps(a.xmm0, a.ptr(psAbsMask));
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm0);

	   return true;
   }

   static bool fnabs(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.orps(a.xmm0, a.ptr(psSignBits);
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm0);

	   return true;
   }

   static bool fmr(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.movsd(a.ppcgpr[instr.frD], a.xmm0);

	   return true;
   }

   static bool fsign(PPCEmuAssembler& a, Instruction instr)
   {
	   a.movdqu(a.xmm0, a.ppcfpr[instr.frB]);
	   a.xorps(a.xmm0, a.ptr(psSignBits);
	   a.movdqu(a.ppcgpr[instr.frD], a.xmm0);

	   return true;
   }

   void registerFloatInstructions()
   {
      RegisterInstructionFallback(fadd);
      RegisterInstructionFallback(fadds);
      RegisterInstructionFallback(fdiv);
      RegisterInstructionFallback(fdivs);
      RegisterInstructionFallback(fmul);
      RegisterInstructionFallback(fmuls);
      RegisterInstructionFallback(fsub);
      RegisterInstructionFallback(fsubs);
      RegisterInstructionFallback(fres);
      RegisterInstructionFallback(frsqrte);
      RegisterInstructionFallback(fsel);
      RegisterInstructionFallback(fmadd);
      RegisterInstructionFallback(fmadds);
      RegisterInstructionFallback(fmsub);
      RegisterInstructionFallback(fmsubs);
      RegisterInstructionFallback(fnmadd);
      RegisterInstructionFallback(fnmadds);
      RegisterInstructionFallback(fnmsub);
      RegisterInstructionFallback(fnmsubs);
      RegisterInstruction(fctiw);
      RegisterInstruction(fctiwz);
      RegisterInstruction(frsp);
      RegisterInstruction(fabs);
      RegisterInstruction(fnabs);
      RegisterInstruction(fmr);
      RegisterInstruction(fneg);
   }

   }
}