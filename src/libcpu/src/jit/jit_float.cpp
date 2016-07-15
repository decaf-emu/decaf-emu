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
   // TODO: Check if we can do this without the extra register...
   auto tmp = a.allocXmmTmp();
   a.cvtsd2ss(tmp, reg);
   a.cvtss2sd(reg, tmp);
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

template <bool ShouldTruncate>
static bool
faddGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
   a.movq(dst, srcA);
   a.addsd(dst, tmpSrcB);

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   return true;
}

static bool
fadd(PPCEmuAssembler& a, Instruction instr)
{
   return faddGeneric<false>(a, instr);
}

static bool
fadds(PPCEmuAssembler& a, Instruction instr)
{
   return faddGeneric<true>(a, instr);
}

template <bool ShouldTruncate>
static bool
fdivGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
   a.movq(dst, srcA);
   a.divsd(dst, tmpSrcB);

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   return true;
}

static bool
fdiv(PPCEmuAssembler& a, Instruction instr)
{
   return fdivGeneric<false>(a, instr);
}

static bool
fdivs(PPCEmuAssembler& a, Instruction instr)
{
   return fdivGeneric<true>(a, instr);
}

template <bool ShouldTruncate>
static bool
fmulGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
   a.movq(dst, srcA);
   a.mulsd(dst, tmpSrcC);

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   return true;
}

static bool
fmul(PPCEmuAssembler& a, Instruction instr)
{
   return fmulGeneric<false>(a, instr);
}

static bool
fmuls(PPCEmuAssembler& a, Instruction instr)
{
   return fmulGeneric<true>(a, instr);
}

template <bool ShouldTruncate>
static bool
fsubGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
   a.movq(dst, srcA);
   a.subsd(dst, tmpSrcB);

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

   return true;
}

static bool
fsub(PPCEmuAssembler& a, Instruction instr)
{
   return fsubGeneric<false>(a, instr);
}

static bool
fsubs(PPCEmuAssembler& a, Instruction instr)
{
   return fsubGeneric<true>(a, instr);
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
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
   auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
   a.movq(dst, srcA);
   a.mulsd(dst, tmpSrcC);
   a.addsd(dst, tmpSrcB);

   if (ShouldNegate) {
      negateXmmSd(a, dst);
   }

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

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
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));
   auto tmpSrcC = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frC]));
   a.movq(dst, srcA);
   a.mulsd(dst, tmpSrcC);
   a.subsd(dst, tmpSrcB);

   if (ShouldNegate) {
      negateXmmSd(a, dst);
   }

   if (ShouldTruncate) {
      truncateToSingleSd(a, dst);
   }

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

   return true;
}

template <bool ShouldNegate>
static bool
fabsGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frB]);
   a.movq(dst, srcA);

   absXmmSd(a, dst);

   if (ShouldNegate) {
      negateXmmSd(a, dst);
   }

   return true;
}

static bool
fabs(PPCEmuAssembler& a, Instruction instr)
{
   return fabsGeneric<false>(a, instr);
}

static bool
fnabs(PPCEmuAssembler& a, Instruction instr)
{
   return fabsGeneric<true>(a, instr);
}

static bool
fmr(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterReadWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frB]);
   a.movq(dst, srcA);

   return true;
}

static bool
fneg(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   auto srcA = a.loadRegisterRead(a.fprps[instr.frB]);
   a.movq(dst, srcA);

   negateXmmSd(a, dst);

   return true;
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
