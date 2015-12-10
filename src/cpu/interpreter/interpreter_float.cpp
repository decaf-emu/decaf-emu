#include <cfenv>
#include <numeric>
#include "interpreter_insreg.h"
#include "interpreter.h"
#include "utils/bitutils.h"
#include "utils/floatutils.h"

const int fres_expected_base[] =
{
   0x7ff800, 0x783800, 0x70ea00, 0x6a0800,
   0x638800, 0x5d6200, 0x579000, 0x520800,
   0x4cc800, 0x47ca00, 0x430800, 0x3e8000,
   0x3a2c00, 0x360800, 0x321400, 0x2e4a00,
   0x2aa800, 0x272c00, 0x23d600, 0x209e00,
   0x1d8800, 0x1a9000, 0x17ae00, 0x14f800,
   0x124400, 0x0fbe00, 0x0d3800, 0x0ade00,
   0x088400, 0x065000, 0x041c00, 0x020c00,
};

const int fres_expected_dec[] =
{
   0x3e1, 0x3a7, 0x371, 0x340,
   0x313, 0x2ea, 0x2c4, 0x2a0,
   0x27f, 0x261, 0x245, 0x22a,
   0x212, 0x1fb, 0x1e5, 0x1d1,
   0x1be, 0x1ac, 0x19b, 0x18b,
   0x17c, 0x16e, 0x15b, 0x15b,
   0x143, 0x143, 0x12d, 0x12d,
   0x11a, 0x11a, 0x108, 0x106,
};

double
ppc_estimate_reciprocal(double v)
{
   auto bits = get_float_bits(v);

   if (bits.mantissa == 0 && bits.exponent == 0) {
      return std::copysign(std::numeric_limits<double>::infinity(), v);
   }

   if (bits.exponent == bits.exponent_max) {
      if (bits.mantissa == 0) {
         return std::copysign(0.0, v);
      }
      return static_cast<float>(v);
   }

   if (bits.exponent < 895) {
      return std::copysign(std::numeric_limits<float>::max(), v);
   }

   if (bits.exponent > 1149) {
      return std::copysign(0.0, v);
   }

   int idx = (int)(bits.mantissa >> 37);
   bits.exponent = 0x7FD - bits.exponent;
   bits.mantissa = (int64_t)(fres_expected_base[idx / 1024] - (fres_expected_dec[idx / 1024] * (idx % 1024) + 1) / 2) << 29;
   return bits.v;
}

void
updateFEX_VX(ThreadState *state)
{
   auto &fpscr = state->fpscr;

   // Invalid Operation Summary
   fpscr.vx =
        fpscr.vxsnan
      | fpscr.vxisi
      | fpscr.vxidi
      | fpscr.vxzdz
      | fpscr.vximz
      | fpscr.vxvc
      | fpscr.vxsqrt
      | fpscr.vxsoft
      | fpscr.vxcvi;

   // FP Enabled Exception Summary
   fpscr.fex =
        (fpscr.vx & fpscr.ve)
      | (fpscr.ox & fpscr.oe)
      | (fpscr.ux & fpscr.ue)
      | (fpscr.zx & fpscr.ze)
      | (fpscr.xx & fpscr.xe);
}


void
updateFX_FEX_VX(ThreadState *state, uint32_t oldValue)
{
   auto &fpscr = state->fpscr;

   updateFEX_VX(state);

   // FP Exception Summary
   const uint32_t newBits = (oldValue ^ fpscr.value) & fpscr.value;
   if (newBits & FPSCRRegisterBits::AllExceptions) {
      fpscr.fx = 1;
   }
}

void
updateFPSCR(ThreadState *state, uint32_t oldValue)
{
   auto except = std::fetestexcept(FE_ALL_EXCEPT);
   auto round = std::fegetround();
   auto &fpscr = state->fpscr;

   // Underflow
   fpscr.ux |= !!(except & FE_UNDERFLOW);

   // Overflow
   fpscr.ox |= !!(except & FE_OVERFLOW);

   // Zerodivide
   fpscr.zx |= !!(except & FE_DIVBYZERO);

   // Inexact
   fpscr.fi = !!(except & FE_INEXACT);
   fpscr.xx |= fpscr.fi;

   // Fraction Rounded
   fpscr.fr = !!(round & FE_UPWARD);

   updateFX_FEX_VX(state, oldValue);

   std::feclearexcept(FE_ALL_EXCEPT);
}

template<typename Type>
void
updateFPRF(ThreadState *state, Type value)
{
   auto cls = std::fpclassify(value);
   auto flags = 0u;

   if (cls == FP_NAN) {
      flags |= FloatingPointResultFlags::ClassDescriptor;
      flags |= FloatingPointResultFlags::Unordered;
   } else if (value != 0) {
      if (value > 0) {
         flags |= FloatingPointResultFlags::Positive;
      } else {
         flags |= FloatingPointResultFlags::Negative;
      }
      if (cls == FP_INFINITE) {
         flags |= FloatingPointResultFlags::Unordered;
      } else if (cls == FP_SUBNORMAL) {
         flags |= FloatingPointResultFlags::ClassDescriptor;
      }
   } else {
      flags |= FloatingPointResultFlags::Equal;
      if (std::signbit(value)) {
         flags |= FloatingPointResultFlags::ClassDescriptor;
      }
   }

   state->fpscr.fprf = flags;
}

void
updateFloatConditionRegister(ThreadState *state)
{
   state->cr.cr1 = state->fpscr.cr1;
}

// Floating Arithmetic
enum FPArithOperator {
    FPAdd,
    FPSub,
    FPMul,
    FPDiv,
};
template<FPArithOperator op, typename Type>
static void
fpArithGeneric(ThreadState *state, Instruction instr)
{
   double a, b;
   Type d;
   a = state->fpr[instr.frA].value;
   b = state->fpr[op == FPMul ? instr.frC : instr.frB].value;

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b);
   bool vxisi, vximz, vxidi, vxzdz, zx;
   switch (op) {
   case FPAdd:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) != std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPSub:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) == std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPMul:
      vxisi = false;
      vximz = (is_infinity(a) && is_zero(b)) || (is_zero(a) && is_infinity(b));
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPDiv:
      vxisi = false;
      vximz = false;
      vxidi = is_infinity(a) && is_infinity(b);
      vxzdz = is_zero(a) && is_zero(b);
      zx = !(vxzdz || vxsnan) && is_zero(b);
      break;
   }

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;
   state->fpscr.vxidi |= vxidi;
   state->fpscr.vxzdz |= vxzdz;

   if ((vxsnan || vxisi || vximz || vxidi || vxzdz) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      if (is_nan(a)) {
         d = make_quiet(a);
      } else if (is_nan(b)) {
         d = make_quiet(b);
      } else if (vxisi || vximz || vxidi || vxzdz) {
         d = make_nan<double>();
      } else {
         // The Espresso appears to use double precision arithmetic even for
         // single-precision instructions (for example, 2^128 * 0.5 does not
         // cause overflow), so we do the same here.
         switch (op) {
         case FPAdd:
            d = static_cast<Type>(a + b);
            break;
         case FPSub:
            d = static_cast<Type>(a - b);
            break;
         case FPMul:
            d = static_cast<Type>(a * b);
            break;
         case FPDiv:
            d = static_cast<Type>(a / b);
            break;
         }
      }
      if (std::is_same<Type, float>::value) {
         state->fpr[instr.frD].paired0 = d;
         state->fpr[instr.frD].paired1 = d;
      } else {
         state->fpr[instr.frD].value = d;
      }
      updateFPRF(state, d);
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Add Double
static void
fadd(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPAdd, double>(state, instr);
}

// Floating Add Single
static void
fadds(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPAdd, float>(state, instr);
}

// Floating Divide Double
static void
fdiv(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPDiv, double>(state, instr);
}

// Floating Divide Single
static void
fdivs(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPDiv, float>(state, instr);
}

// Floating Multiply Double
static void
fmul(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPMul, double>(state, instr);
}

// Floating Multiply Single
static void
fmuls(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPMul, float>(state, instr);
}

// Floating Subtract Double
static void
fsub(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPSub, double>(state, instr);
}

// Floating Subtract Single
static void
fsubs(ThreadState *state, Instruction instr)
{
   fpArithGeneric<FPSub, float>(state, instr);
}

// Floating Reciprocal Estimate Single
static void
fres(ThreadState *state, Instruction instr)
{
   double b;
   float d;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   const bool zx = is_zero(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;

   if (vxsnan && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      d = static_cast<float>(ppc_estimate_reciprocal(b));
      state->fpr[instr.frD].paired0 = d;
      // paired1 is left undefined in the UISA.  TODO: Check actual behavior.
      updateFPRF(state, d);
      state->fpscr.zx |= zx;
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Reciprocal Square Root Estimate
static void
frsqrte(ThreadState *state, Instruction instr)
{
   double b, d;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   const bool vxsqrt = (b < 0);
   const bool zx = is_zero(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxsqrt |= vxsqrt;

   if ((vxsnan || vxsqrt) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      if (vxsqrt) {
         d = make_nan<double>();
      } else {
         d = 1.0 / std::sqrt(b);
      }
      state->fpr[instr.frD].value = d;
      updateFPRF(state, d);
      state->fpscr.zx |= zx;
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fsel(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].value;
   b = state->fpr[instr.frB].value;
   c = state->fpr[instr.frC].value;

   if (a >= 0.0) {
      d = c;
   } else {
      d = b;
   }

   state->fpr[instr.frD].value = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Fused multiply-add instructions
enum FMAFlags
{
   FMASubtract   = 1 << 0, // Subtract instead of add
   FMANegate     = 1 << 1, // Negate result
   FMASinglePrec = 1 << 2, // Round result to single precision
};

template<unsigned flags>
static void
fmaGeneric(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].value;
   b = state->fpr[instr.frB].value;
   c = state->fpr[instr.frC].value;
   const double addend = (flags & FMASubtract) ? -b : b;

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   const bool vxisi = ((is_infinity(a) || is_infinity(c)) && is_infinity(b)
                       && (std::signbit(a) ^ std::signbit(c)) != std::signbit(addend));
   const bool vximz = (is_infinity(a) && is_zero(c)) || (is_zero(a) && is_infinity(c));

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;

   if ((vxsnan || vxisi || vximz) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      if (is_nan(a)) {
         d = make_quiet(a);
      } else if (is_nan(b)) {
         d = make_quiet(b);
      } else if (is_nan(c)) {
         d = make_quiet(c);
      } else if (vxisi || vximz) {
         d = make_nan<double>();
      } else {
         d = std::fma(a, c, addend);
         if (flags & FMANegate) {
            d = -d;
         }
      }
      if (flags & FMASinglePrec) {
         state->fpr[instr.frD].paired0 = static_cast<float>(d);
         state->fpr[instr.frD].paired1 = static_cast<float>(d);
      } else {
         state->fpr[instr.frD].value = d;
      }
      updateFPRF(state, d);
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Add
static void
fmadd(ThreadState *state, Instruction instr)
{
   return fmaGeneric<0>(state, instr);
}

// Floating Multiply-Add Single
static void
fmadds(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMASinglePrec>(state, instr);
}

// Floating Multiply-Sub
static void
fmsub(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMASubtract>(state, instr);
}

// Floating Multiply-Sub Single
static void
fmsubs(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMASubtract | FMASinglePrec>(state, instr);
}

// Floating Negative Multiply-Add
static void
fnmadd(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMANegate>(state, instr);
}

// Floating Negative Multiply-Add Single
static void
fnmadds(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASinglePrec>(state, instr);
}

// Floating Negative Multiply-Sub
static void
fnmsub(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASubtract>(state, instr);
}

// Floating Negative Multiply-Sub Single
static void
fnmsubs(ThreadState *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASubtract | FMASinglePrec>(state, instr);
}

// fctiw/fctiwz common implementation
static void
fctiwGeneric(ThreadState *state, Instruction instr, FloatingPointRoundMode::FloatingPointRoundMode roundMode)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   bool vxcvi, fi;

   if (is_nan(b)) {
      vxcvi = true;
      fi = false;
      bi = INT_MIN;
   } else if (b > static_cast<double>(INT_MAX)) {
      vxcvi = true;
      fi = false;
      bi = INT_MAX;
   } else if (b < static_cast<double>(INT_MIN)) {
      vxcvi = true;
      fi = false;
      bi = INT_MIN;
   } else {
      vxcvi = false;
      switch (roundMode) {
      case FloatingPointRoundMode::Nearest:
         // We have to use nearbyint() instead of round() here, because
         // round() rounds 0.5 away from zero instead of to the nearest
         // even integer.  nearbyint() is dependent on the host's FPU
         // rounding mode, but since that will reflect FPSCR here, it's
         // safe to use.
         bi = static_cast<int32_t>(std::nearbyint(b));
         break;
      case FloatingPointRoundMode::Zero:
         bi = static_cast<int32_t>(std::trunc(b));
         break;
      case FloatingPointRoundMode::Positive:
         bi = static_cast<int32_t>(std::ceil(b));
         break;
      case FloatingPointRoundMode::Negative:
         bi = static_cast<int32_t>(std::floor(b));
         break;
      }
      fi = get_float_bits(b).exponent < 1075 && bi != b;
   }

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxcvi |= vxcvi;

   if ((vxsnan || vxcvi) && state->fpscr.ve) {
      state->fpscr.fr = 0;
      state->fpscr.fi = 0;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      state->fpr[instr.frD].iw1 = bi;
      state->fpr[instr.frD].iw0 = 0xFFF80000 | (is_negative_zero(b) ? 1 : 0);
      updateFPSCR(state, oldFPSCR);
      // We need to set FPSCR[FI] manually since the rounding functions
      // don't always raise inexact exceptions.
      if (fi) {
         state->fpscr.fi = 1;
         state->fpscr.xx = 1;
         updateFX_FEX_VX(state, oldFPSCR);
      }
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fctiw(ThreadState *state, Instruction instr)
{
   return fctiwGeneric(state, instr, static_cast<FloatingPointRoundMode::FloatingPointRoundMode>(state->fpscr.rn));
}

// Floating Convert to Integer Word with Round toward Zero
static void
fctiwz(ThreadState *state, Instruction instr)
{
   return fctiwGeneric(state, instr, FloatingPointRoundMode::Zero);
}

// Floating Round to Single
static void
frsp(ThreadState *state, Instruction instr)
{
   auto b = state->fpr[instr.frB].value;
   auto vxsnan = is_signalling_nan(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;

   if (vxsnan && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      auto d = static_cast<float>(b);
      state->fpr[instr.frD].paired0 = d;
      updateFPRF(state, d);
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// TODO: do fabs/fnabs/fneg behave like fmr w.r.t. paired singles?
// Floating Absolute Value
static void
fabs(ThreadState *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = clear_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Absolute Value
static void
fnabs(ThreadState *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = set_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Move Register
static void
fmr(ThreadState *state, Instruction instr)
{
   state->fpr[instr.frD].idw = state->fpr[instr.frB].idw;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negate
static void
fneg(ThreadState *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = flip_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move from FPSCR
static void
mffs(ThreadState *state, Instruction instr)
{
   state->fpr[instr.frD].iw1 = state->fpscr.value;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Bit 0
static void
mtfsb0(ThreadState *state, Instruction instr)
{
   state->fpscr.value = clear_bit(state->fpscr.value, 31 - instr.crbD);
   updateFEX_VX(state);
   if (instr.crbD >= 30) {
       cpu::setRoundingMode(state);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Bit 1
static void
mtfsb1(ThreadState *state, Instruction instr)
{
   const uint32_t oldValue = state->fpscr.value;
   state->fpscr.value = set_bit(state->fpscr.value, 31 - instr.crbD);
   updateFX_FEX_VX(state, oldValue);
   if (instr.crbD >= 30) {
       cpu::setRoundingMode(state);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Fields
static void
mtfsf(ThreadState *state, Instruction instr)
{
   const uint32_t value = state->fpr[instr.frB].iw1;
   for (int field = 0; field < 8; field++) {
      // Technically field 0 is at the high end, but as long as the bit
      // position in the mask and the field we operate on match up, it
      // doesn't matter which direction we go in.  So we use host bit
      // order for simplicity.
      if (get_bit(instr.fm, field)) {
         const uint32_t mask = make_bitmask(4 * field, 4 * field + 3);
         state->fpscr.value &= ~mask;
         state->fpscr.value |= value & mask;
      }
   }
   updateFEX_VX(state);
   if (get_bit(instr.fm, 0)) {
       cpu::setRoundingMode(state);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Field Immediate
static void
mtfsfi(ThreadState *state, Instruction instr)
{
   const int shift = 4 * (7 - instr.crfD);
   state->fpscr.value &= ~(0xF << shift);
   state->fpscr.value |= instr.imm << shift;
   updateFEX_VX(state);
   if (instr.crfD == 7) {
       cpu::setRoundingMode(state);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

void
cpu::interpreter::registerFloatInstructions()
{
   RegisterInstruction(fadd);
   RegisterInstruction(fadds);
   RegisterInstruction(fdiv);
   RegisterInstruction(fdivs);
   RegisterInstruction(fmul);
   RegisterInstruction(fmuls);
   RegisterInstruction(fsub);
   RegisterInstruction(fsubs);
   RegisterInstruction(fres);
   RegisterInstruction(frsqrte);
   RegisterInstruction(fsel);
   RegisterInstruction(fmadd);
   RegisterInstruction(fmadds);
   RegisterInstruction(fmsub);
   RegisterInstruction(fmsubs);
   RegisterInstruction(fnmadd);
   RegisterInstruction(fnmadds);
   RegisterInstruction(fnmsub);
   RegisterInstruction(fnmsubs);
   RegisterInstruction(fctiw);
   RegisterInstruction(fctiwz);
   RegisterInstruction(frsp);
   RegisterInstruction(fabs);
   RegisterInstruction(fnabs);
   RegisterInstruction(fmr);
   RegisterInstruction(fneg);
   RegisterInstruction(mffs);
   RegisterInstruction(mtfsb0);
   RegisterInstruction(mtfsb1);
   RegisterInstruction(mtfsf);
   RegisterInstruction(mtfsfi);
}
