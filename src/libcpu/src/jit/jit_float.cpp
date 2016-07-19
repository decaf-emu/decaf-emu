#include "jit_insreg.h"
#include "jit_float.h"
#include "common/bitutils.h"
#include "common/decaf_assert.h"
#include <cstdint>

namespace cpu
{

namespace jit
{

static void
truncateToSingleSd(PPCEmuAssembler& a, const PPCEmuAssembler::XmmRegister& reg)
{
   a.cvtsd2ss(reg, reg);
   a.cvtss2sd(reg, reg);
}

static void
truncateTo24BitSd(PPCEmuAssembler& a, const PPCEmuAssembler::XmmRegister& reg)
{
   auto maskGp = a.allocGpTmp();
   auto maskXmm = a.allocXmmTmp();
   auto tmp = a.allocXmmTmp();
   a.movq(tmp, reg);

   a.mov(maskGp, UINT64_C(0x8000000));
   a.movq(maskXmm, maskGp);
   a.pand(tmp, maskXmm);

   a.mov(maskGp, UINT64_C(0xFFFFFFFFF8000000));
   a.movq(maskXmm, maskGp);
   a.pand(reg, maskXmm);

   a.paddq(reg, tmp);
}

static void
negateXmmSd(PPCEmuAssembler& a, const PPCEmuAssembler::XmmRegister& reg)
{
   auto maskGp = a.allocGpTmp();
   auto maskXmm = a.allocXmmTmp();
   a.mov(maskGp, UINT64_C(0x8000000000000000));
   a.movq(maskXmm, maskGp);
   a.pxor(reg, maskXmm);
}

static void
absXmmSd(PPCEmuAssembler& a, const PPCEmuAssembler::XmmRegister& reg)
{
   auto maskGp = a.allocGpTmp();
   auto maskXmm = a.allocXmmTmp();
   a.mov(maskGp, UINT64_C(0x7FFFFFFFFFFFFFFF));
   a.movq(maskXmm, maskGp);
   a.pand(reg, maskXmm);
}

enum FPArithOperator {
    FPAdd,
    FPSub,
    FPMul,
    FPDiv,
};
template <FPArithOperator op, bool ShouldTruncate>
static bool
fpArithGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto tmpSrcA = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frA]));

   switch (op) {
   case FPAdd: {
      auto srcB = a.loadRegisterRead(a.fprps[instr.frB]);
      a.addsd(tmpSrcA, srcB);
      break;
   }
   case FPSub: {
      auto srcB = a.loadRegisterRead(a.fprps[instr.frB]);
      a.subsd(tmpSrcA, srcB);
      break;
   }
   case FPMul: {
      auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
      if (ShouldTruncate) {
         // PPC has this weird behaviour with fmuls where it truncates the
         //  RHS operator to 24-bits of mantissa before multiplying...
         truncateTo24BitSd(a, tmpSrcC);
      }
      a.mulsd(tmpSrcA, tmpSrcC);
      break;
   }
   case FPDiv: {
      auto srcB = a.loadRegisterRead(a.fprps[instr.frB]);
      a.divsd(tmpSrcA, srcB);
      break;
   }
   }

   if (ShouldTruncate) {
      truncateToSingleSd(a, tmpSrcA);
      auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
      a.movddup(dst, tmpSrcA);
   } else {
      auto tmpDst = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frD]));
      auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
      a.movapd(dst, tmpSrcA);
      a.shufpd(dst, tmpDst, 0x2);
   }

   return true;
}

static bool
fadd(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPAdd, false>(a, instr);
}

static bool
fadds(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPAdd, true>(a, instr);
}

static bool
fsub(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPSub, false>(a, instr);
}

static bool
fsubs(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPSub, true>(a, instr);
}

static bool
fmul(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPMul, false>(a, instr);
}

static bool
fmuls(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPMul, true>(a, instr);
}

static bool
fdiv(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPDiv, false>(a, instr);
}

static bool
fdivs(PPCEmuAssembler& a, Instruction instr)
{
   return fpArithGeneric<FPDiv, true>(a, instr);
}

template <bool ShouldTruncate, bool ShouldNegate>
static bool
fmaddGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);

   {
      auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
      auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
      auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
      a.movq(dst, srcA);
      a.mulsd(dst, tmpSrcC);
      a.addsd(dst, tmpSrcB);
   }

   if (ShouldNegate) {
      negateXmmSd(a, dst);
   }

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   a.movddup(dst, dst);
   return true;
}

static bool
fmadd(PPCEmuAssembler& a, Instruction instr)
{
   return fmaddGeneric<false, false>(a, instr);
}

static bool
fmadds(PPCEmuAssembler& a, Instruction instr)
{
   return fmaddGeneric<true, false>(a, instr);
}

template <bool ShouldTruncate, bool ShouldNegate>
static bool
fmsubGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);

   {
      auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
      auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
      auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
      a.movq(dst, srcA);
      a.mulsd(dst, tmpSrcC);
      a.subsd(dst, tmpSrcB);
   }

   if (ShouldNegate) {
      negateXmmSd(a, dst);
   }

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   a.movddup(dst, dst);
   return true;
}

static bool
fmsub(PPCEmuAssembler& a, Instruction instr)
{
   return fmsubGeneric<false, false>(a, instr);
}

static bool
fmsubs(PPCEmuAssembler& a, Instruction instr)
{
   return fmsubGeneric<true, false>(a, instr);
}

static bool
fnmadd(PPCEmuAssembler& a, Instruction instr)
{
   return fmaddGeneric<false, true>(a, instr);
}

static bool
fnmadds(PPCEmuAssembler& a, Instruction instr)
{
   return fmaddGeneric<true, true>(a, instr);
}

static bool
fnmsub(PPCEmuAssembler& a, Instruction instr)
{
   return fmsubGeneric<false, true>(a, instr);
}

static bool
fnmsubs(PPCEmuAssembler& a, Instruction instr)
{
   return fmsubGeneric<true, true>(a, instr);
}

static bool
frsp(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frB]);
   a.movq(dst, srcA);

   truncateToSingleSd(a, dst);

   a.movddup(dst, dst);
   return true;
}

template <bool ShouldAbs, bool ShouldNegate>
static bool
fmrGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   auto tmpSrc = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));

   if (ShouldAbs) {
      absXmmSd(a, tmpSrc);
   }

   if (ShouldNegate) {
      negateXmmSd(a, tmpSrc);
   }

   // We need to preserve the second double (ps1) of frD, but shufpd only
   //  allows preserving the first double, so we need a temporary.
   auto tmpDst = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frD]));
   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   a.movapd(dst, tmpSrc);
   a.shufpd(dst, tmpDst, 0x2);

   return true;
}

static bool
fabs(PPCEmuAssembler& a, Instruction instr)
{
   return fmrGeneric<true, false>(a, instr);
}

static bool
fnabs(PPCEmuAssembler& a, Instruction instr)
{
   return fmrGeneric<true, true>(a, instr);
}

static bool
fmr(PPCEmuAssembler& a, Instruction instr)
{
   return fmrGeneric<false, false>(a, instr);
}

static bool
fneg(PPCEmuAssembler& a, Instruction instr)
{
   return fmrGeneric<false, true>(a, instr);
}

void registerFloatInstructions()
{
   // TODO: fmXXX instructions are CLOSE, but not perfectly
   //   accurate...

   RegisterInstruction(fadd);
   RegisterInstruction(fadds);
   RegisterInstruction(fdiv);
   RegisterInstruction(fdivs);
   RegisterInstruction(fmul);
   RegisterInstruction(fmuls);
   RegisterInstruction(fsub);
   RegisterInstruction(fsubs);
   RegisterInstructionFallback(fres);
   RegisterInstructionFallback(frsqrte);
   RegisterInstructionFallback(fsel);
   RegisterInstruction(fmadd);
   RegisterInstruction(fmadds);
   RegisterInstruction(fmsub);
   RegisterInstruction(fmsubs);
   RegisterInstruction(fnmadd);
   RegisterInstruction(fnmadds);
   RegisterInstruction(fnmsub);
   RegisterInstruction(fnmsubs);
   RegisterInstructionFallback(fctiw);
   RegisterInstructionFallback(fctiwz);
   RegisterInstruction(frsp);
   RegisterInstruction(fabs);
   RegisterInstruction(fnabs);
   RegisterInstruction(fmr);
   RegisterInstruction(fneg);
   RegisterInstructionFallback(mffs);
   RegisterInstructionFallback(mtfsb0);
   RegisterInstructionFallback(mtfsb1);
   RegisterInstructionFallback(mtfsf);
   RegisterInstructionFallback(mtfsfi);
}

} // namespace jit

} // namespace cpu
