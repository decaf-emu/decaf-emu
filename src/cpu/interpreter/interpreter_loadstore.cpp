#include <algorithm>
#include <cmath>
#include "interpreter_float.h"
#include "interpreter_insreg.h"
#include "mem/mem.h"
#include "utils/bitutils.h"
#include "utils/floatutils.h"

using espresso::QuantizedDataType;
using espresso::ConditionRegisterFlag;

// Load
enum LoadFlags
{
   LoadUpdate        = 1 << 0, // Save EA in rA
   LoadIndexed       = 1 << 1, // Use rB instead of d
   LoadSignExtend    = 1 << 2, // Sign extend
   LoadByteReverse   = 1 << 3, // Swap bytes
   LoadReserve       = 1 << 4, // lwarx harware reserve
   LoadZeroRA        = 1 << 5, // Use 0 instead of r0
};

static double
convertFloatToDouble(float f)
{
   if (!is_signalling_nan(f)) {
      return static_cast<double>(f);
   } else {
      return extend_float(f);
   }
}

static double
loadFloatAsDouble(uint32_t ea)
{
   return convertFloatToDouble(mem::read<float>(ea));
}

template<unsigned flags = 0>
static void
loadFloat(ThreadState *state, Instruction instr)
{
   uint32_t ea;

   if ((flags & LoadZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & LoadIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   const float f = mem::read<float>(ea);
   state->fpr[instr.rD].paired0 = convertFloatToDouble(f);
   state->fpr[instr.rD].paired1 = convertFloatToDouble(f);

   if (flags & LoadUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

template<typename Type, unsigned flags = 0>
static void
loadGeneric(ThreadState *state, Instruction instr)
{
   uint32_t ea;
   Type d;

   if ((flags & LoadZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & LoadIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   if (flags & LoadByteReverse) {
      // Read already does byte_swap, so we readNoSwap for byte reverse
      d = mem::readNoSwap<Type>(ea);
   } else {
      d = mem::read<Type>(ea);
   }

   if (std::is_floating_point<Type>::value) {
      state->fpr[instr.rD].value = static_cast<double>(d);
      // Normally lfd instructions do not modify the second paired-single
      // slot (ps1) of an FPR, but if the lfd is immediately preceded or
      // followed by paired-single instructions, frD(ps1) may receive the
      // high 32 bits of the loaded word or some other value that happens
      // to be stored in the FP unit.  This data hazard is strongly
      // dependent on pipeline state, and we don't attempt to emulate it.
      // (Using a double-precision value with paired-single instructions
      // is documented to result in undefined behavior anyway, so it seems
      // unlikely this sort of instruction sequence would be found in real
      // software.)
   } else {
      if (flags & LoadSignExtend) {
         state->gpr[instr.rD] = static_cast<uint32_t>(sign_extend<bit_width<Type>::value, uint64_t>(static_cast<uint64_t>(d)));
      } else {
         state->gpr[instr.rD] = static_cast<uint32_t>(d);
      }
   }

   if (flags & LoadReserve) {
      state->reserve = true;
      state->reserveAddress = ea;
      state->reserveData = mem::read<uint32_t>(state->reserveAddress);
   }

   if (flags & LoadUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
lbz(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadZeroRA>(state, instr);
}

static void
lbzu(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate>(state, instr);
}

static void
lbzux(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lbzx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lha(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadZeroRA>(state, instr);
}

static void
lhau(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate>(state, instr);
}

static void
lhaux(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate | LoadIndexed>(state, instr);
}

static void
lhax(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lhbrx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lhz(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadZeroRA>(state, instr);
}

static void
lhzu(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate>(state, instr);
}

static void
lhzux(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lhzx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwbrx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwarx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadReserve | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwz(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadZeroRA>(state, instr);
}

static void
lwzu(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate>(state, instr);
}

static void
lwzux(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lwzx(ThreadState *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lfs(ThreadState *state, Instruction instr)
{
   return loadFloat<LoadZeroRA>(state, instr);
}

static void
lfsu(ThreadState *state, Instruction instr)
{
   return loadFloat<LoadUpdate>(state, instr);
}

static void
lfsux(ThreadState *state, Instruction instr)
{
   return loadFloat<LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfsx(ThreadState *state, Instruction instr)
{
   return loadFloat<LoadZeroRA | LoadIndexed>(state, instr);
}

static void
lfd(ThreadState *state, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA>(state, instr);
}

static void
lfdu(ThreadState *state, Instruction instr)
{
   return loadGeneric<double, LoadUpdate>(state, instr);
}

static void
lfdux(ThreadState *state, Instruction instr)
{
   return loadGeneric<double, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfdx(ThreadState *state, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA | LoadIndexed>(state, instr);
}

// Load Multiple Words
// Fills registers from rD to r31 with consecutive words from memory
static void
lmw(ThreadState *state, Instruction instr)
{
   uint32_t b, ea, r;

   if (instr.rA) {
      b = state->gpr[instr.rA];
   } else {
      b = 0;
   }

   ea = b + sign_extend<16, int32_t>(instr.d);

   for (r = instr.rD; r <= 31; ++r, ea += 4) {
      state->gpr[r] = mem::read<uint32_t>(ea);
   }
}

// Load String Word (byte-by-byte version of lmw)
enum LswFlags
{
   LswIndexed = 1 >> 0,
};

template<unsigned flags = 0>
static void
lswGeneric(ThreadState *state, Instruction instr)
{
   uint32_t ea, i, n, r;

   ea = instr.rA ? state->gpr[instr.rA] : 0;

   if (flags & LswIndexed) {
      ea += state->gpr[instr.rB];
      n = state->xer.byteCount;
   } else {
      n = instr.nb ? instr.nb : 32;
   }

   r = instr.rD - 1;
   i = 0;

   while (n > 0) {
      if (i == 0) {
         r = (r + 1) % 32;
         state->gpr[r] = 0;
      }

      state->gpr[r] |= mem::read<uint8_t>(ea) << (24 - i);

      i = (i + 8) % 32;
      ea = ea + 1;
      n = n - 1;
   }
}

static void
lswi(ThreadState *state, Instruction instr)
{
   lswGeneric(state, instr);
}

static void
lswx(ThreadState *state, Instruction instr)
{
   lswGeneric<LswIndexed>(state, instr);
}

// Store
enum StoreFlags
{
   StoreUpdate          = 1 << 0, // Save EA in rA
   StoreIndexed         = 1 << 1, // Use rB instead of d
   StoreByteReverse     = 1 << 2, // Swap Bytes
   StoreConditional     = 1 << 3, // lward/stwcx Conditional
   StoreZeroRA          = 1 << 4, // Use 0 instead of r0
   StoreFloatAsInteger  = 1 << 5, // stfiwx
};

static void
storeDoubleAsFloat(uint32_t ea, double d)
{
   float f;
   if (!is_signalling_nan(d)) {
      f = static_cast<float>(d);
   } else {
      f = truncate_double(d);
   }
   mem::write<float>(ea, f);
}

template<unsigned flags = 0>
static void
storeFloat(ThreadState *state, Instruction instr)
{
   uint32_t ea;

   if ((flags & StoreZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & StoreIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   const double d = state->fpr[instr.rS].value;
   storeDoubleAsFloat(ea, d);

   if (flags & StoreUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

template<typename Type, unsigned flags = 0>
static void
storeGeneric(ThreadState *state, Instruction instr)
{
   uint32_t ea;
   Type s;

   if ((flags & StoreZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & StoreIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   if (flags & StoreConditional) {
      state->cr.cr0 = state->xer.so ? ConditionRegisterFlag::SummaryOverflow : 0;

      if (state->reserve) {
         state->reserve = false;

         if (mem::read<uint32_t>(state->reserveAddress) == state->reserveData) {
            // Store is succesful, clear reserve bit and set CR0[EQ]
            state->cr.cr0 |= ConditionRegisterFlag::Equal;
         } else {
            // Reservation has been written by another process
            return;
         }
      } else {
         // Reserve bit is not set, do not write.
         return;
      }
   }

   if (flags & StoreFloatAsInteger) {
      s = static_cast<Type>(state->fpr[instr.rS].iw1);
   } else if (std::is_floating_point<Type>::value) {
      s = static_cast<Type>(state->fpr[instr.rS].value);
   } else {
      s = static_cast<Type>(state->gpr[instr.rS]);
   }

   if (flags & StoreByteReverse) {
      // Write already does byte_swap, so we writeNoSwap for byte reverse
      mem::writeNoSwap<Type>(ea, s);
   } else {
      mem::write<Type>(ea, s);
   }

   if (flags & StoreUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
stb(ThreadState *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreZeroRA>(state, instr);
}

static void
stbu(ThreadState *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreUpdate>(state, instr);
}

static void
stbux(ThreadState *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stbx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
sth(ThreadState *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA>(state, instr);
}

static void
sthu(ThreadState *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreUpdate>(state, instr);
}

static void
sthux(ThreadState *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
sthx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stw(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA>(state, instr);
}

static void
stwu(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreUpdate>(state, instr);
}

static void
stwux(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stwx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
sthbrx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(state, instr);
}

static void
stwbrx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(state, instr);
}

static void
stwcx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreConditional | StoreIndexed>(state, instr);
}

static void
stfs(ThreadState *state, Instruction instr)
{
   storeFloat<StoreZeroRA>(state, instr);
}

static void
stfsu(ThreadState *state, Instruction instr)
{
   storeFloat<StoreUpdate>(state, instr);
}

static void
stfsux(ThreadState *state, Instruction instr)
{
   storeFloat<StoreUpdate | StoreIndexed>(state, instr);
}

static void
stfsx(ThreadState *state, Instruction instr)
{
   storeFloat<StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stfd(ThreadState *state, Instruction instr)
{
   storeGeneric<double, StoreZeroRA>(state, instr);
}

static void
stfdu(ThreadState *state, Instruction instr)
{
   storeGeneric<double, StoreUpdate>(state, instr);
}

static void
stfdux(ThreadState *state, Instruction instr)
{
   storeGeneric<double, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stfdx(ThreadState *state, Instruction instr)
{
   storeGeneric<double, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stfiwx(ThreadState *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreFloatAsInteger | StoreZeroRA | StoreIndexed>(state, instr);
}

// Store Multiple Words
// Writes consecutive words to memory from rS to r31
static void
stmw(ThreadState *state, Instruction instr)
{
   uint32_t b, ea, r;

   if (instr.rA) {
      b = state->gpr[instr.rA];
   } else {
      b = 0;
   }

   ea = b + sign_extend<16, int32_t>(instr.d);

   for (r = instr.rS; r <= 31; ++r, ea += 4) {
      mem::write<uint32_t>(ea, state->gpr[r]);
   }
}

// Store String Word (byte-by-byte version of lmw)
enum StswFlags
{
   StswIndexed = 1 >> 0,
};

template<unsigned flags = 0>
static void
stswGeneric(ThreadState *state, Instruction instr)
{
   uint32_t ea, i, n, r;

   ea = instr.rA ? state->gpr[instr.rA] : 0;

   if (flags & LswIndexed) {
      ea += state->gpr[instr.rB];
      n = state->xer.byteCount;
   } else {
      n = instr.nb ? instr.nb : 32;
   }

   r = instr.rS - 1;
   i = 0;

   while (n > 0) {
      if (i == 0) {
         r = (r + 1) % 32;
      }

      mem::write<uint8_t>(ea, (state->gpr[r] >> (24 - i)) & 0xff);

      i = (i + 8) % 32;
      ea = ea + 1;
      n = n - 1;
   }
}

static void
stswi(ThreadState *state, Instruction instr)
{
   stswGeneric(state, instr);
}

static void
stswx(ThreadState *state, Instruction instr)
{
   stswGeneric<StswIndexed>(state, instr);
}

static double
dequantize(uint32_t ea, QuantizedDataType type, uint32_t scale)
{
   int exp = static_cast<int>(scale);
   exp -= (exp & 32) << 1;  // Sign extend.
   double result;

   switch (type) {
   case QuantizedDataType::Floating:
      result = loadFloatAsDouble(ea);
      break;
   case QuantizedDataType::Unsigned8:
      result = std::ldexp(static_cast<double>(mem::read<uint8_t>(ea)), -exp);
      break;
   case QuantizedDataType::Unsigned16:
      result = std::ldexp(static_cast<double>(mem::read<uint16_t>(ea)), -exp);
      break;
   case QuantizedDataType::Signed8:
      result = std::ldexp(static_cast<double>(mem::read<int8_t>(ea)), -exp);
      break;
   case QuantizedDataType::Signed16:
      result = std::ldexp(static_cast<double>(mem::read<int16_t>(ea)), -exp);
      break;
   default:
      assert(!"Unknown QuantizedDataType");
   }

   return result;
}

template<typename Type>
inline Type
clamp(double value)
{
   double min = static_cast<double>(std::numeric_limits<Type>::min());
   double max = static_cast<double>(std::numeric_limits<Type>::max());
   return static_cast<Type>(std::max(min, std::min(value, max)));
}

static void
quantize(uint32_t ea, double value, QuantizedDataType type, uint32_t scale)
{
   int exp = static_cast<int>(scale);
   exp -= (exp & 32) << 1;  // Sign extend.

   switch (type) {
   case QuantizedDataType::Floating:
      if (get_float_bits(value).exponent <= 896) {
         // Make sure to write a zero with the correct sign!
         mem::write(ea, bit_cast<float>(static_cast<uint32_t>(std::signbit(value)) << 31));
      } else {
         storeDoubleAsFloat(ea, value);
      }
      break;
   case QuantizedDataType::Unsigned8:
      if (is_nan(value)) {
         mem::write(ea, (uint8_t)(std::signbit(value) ? 0 : 0xFF));
      } else {
         mem::write(ea, clamp<uint8_t>(std::ldexp(value, exp)));
      }
      break;
   case QuantizedDataType::Unsigned16:
      if (is_nan(value)) {
         mem::write(ea, (uint16_t)(std::signbit(value) ? 0 : 0xFFFF));
      } else {
         mem::write(ea, clamp<uint16_t>(std::ldexp(value, exp)));
      }
      break;
   case QuantizedDataType::Signed8:
      if (is_nan(value)) {
         mem::write(ea, (int8_t)(std::signbit(value) ? -0x80 : 0x7F));
      } else {
         mem::write(ea, clamp<int8_t>(std::ldexp(value, exp)));
      }
      break;
   case QuantizedDataType::Signed16:
      if (is_nan(value)) {
         mem::write(ea, (int16_t)(std::signbit(value) ? -0x8000 : 0x7FFF));
      } else {
         mem::write(ea, clamp<int16_t>(std::ldexp(value, exp)));
      }
      break;
   default:
      assert(!"Unknown QuantizedDataType");
   }
}

// Paired Single Load
enum PsqLoadFlags
{
   PsqLoadZeroRA  = 1 << 0,
   PsqLoadUpdate  = 1 << 1,
   PsqLoadIndexed = 1 << 2,
};

template<unsigned flags = 0>
static void
psqLoad(ThreadState *state, Instruction instr)
{
   uint32_t ea, ls, c, i, w;
   QuantizedDataType lt;

   if ((flags & PsqLoadZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & PsqLoadIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<12, int32_t>(instr.qd);
   }

   if (flags & PsqLoadIndexed) {
      i = instr.qi;
      w = instr.qw;
   } else {
      i = instr.i;
      w = instr.w;
   }

   c = 4;
   lt = static_cast<QuantizedDataType>(state->gqr[i].ld_type);
   ls = state->gqr[i].ld_scale;

   if (lt == QuantizedDataType::Unsigned8 || lt == QuantizedDataType::Signed8) {
      c = 1;
   } else if (lt == QuantizedDataType::Unsigned16 || lt == QuantizedDataType::Signed16) {
      c = 2;
   }

   if (w == 0) {
      state->fpr[instr.frD].paired0 = dequantize(ea, lt, ls);
      state->fpr[instr.frD].paired1 = dequantize(ea + c, lt, ls);
   } else {
      state->fpr[instr.frD].paired0 = dequantize(ea, lt, ls);
      state->fpr[instr.frD].paired1 = 1.0;
   }

   if (flags & PsqLoadUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
psq_l(ThreadState *state, Instruction instr)
{
   psqLoad<PsqLoadZeroRA>(state, instr);
}

static void
psq_lu(ThreadState *state, Instruction instr)
{
   psqLoad<PsqLoadUpdate>(state, instr);
}

static void
psq_lx(ThreadState *state, Instruction instr)
{
   psqLoad<PsqLoadZeroRA | PsqLoadIndexed>(state, instr);
}

static void
psq_lux(ThreadState *state, Instruction instr)
{
   psqLoad<PsqLoadUpdate | PsqLoadIndexed>(state, instr);
}

// Paired Single Store
enum PsqStoreFlags
{
   PsqStoreZeroRA    = 1 << 0,
   PsqStoreUpdate    = 1 << 1,
   PsqStoreIndexed   = 1 << 2,
};

template<unsigned flags = 0>
static void
psqStore(ThreadState *state, Instruction instr)
{
   uint32_t ea, sts, c, i, w;
   QuantizedDataType stt;

   if ((flags & PsqStoreZeroRA) && instr.rA == 0) {
      ea = 0;
   } else {
      ea = state->gpr[instr.rA];
   }

   if (flags & PsqStoreIndexed) {
      ea += state->gpr[instr.rB];
   } else {
      ea += sign_extend<12, int32_t>(instr.qd);
   }

   if (flags & PsqStoreIndexed) {
      i = instr.qi;
      w = instr.qw;
   } else {
      i = instr.i;
      w = instr.w;
   }

   c = 4;
   stt = static_cast<QuantizedDataType>(state->gqr[i].st_type);
   sts = state->gqr[i].st_scale;

   if (stt == QuantizedDataType::Unsigned8 || stt == QuantizedDataType::Signed8) {
      c = 1;
   } else if (stt == QuantizedDataType::Unsigned16 || stt == QuantizedDataType::Signed16) {
      c = 2;
   }

   if (w == 0) {
      quantize(ea, state->fpr[instr.frS].paired0, stt, sts);
      quantize(ea + c, state->fpr[instr.frS].paired1, stt, sts);
   } else {
      quantize(ea, state->fpr[instr.frS].paired0, stt, sts);
   }

   if (flags & PsqStoreUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
psq_st(ThreadState *state, Instruction instr)
{
   psqStore<PsqStoreZeroRA>(state, instr);
}

static void
psq_stu(ThreadState *state, Instruction instr)
{
   psqStore<PsqLoadUpdate>(state, instr);
}

static void
psq_stx(ThreadState *state, Instruction instr)
{
   psqStore<PsqStoreZeroRA | PsqStoreIndexed>(state, instr);
}

static void
psq_stux(ThreadState *state, Instruction instr)
{
   psqStore<PsqStoreUpdate | PsqStoreIndexed>(state, instr);
}

void cpu::interpreter::registerLoadStoreInstructions()
{
   RegisterInstruction(lbz);
   RegisterInstruction(lbzu);
   RegisterInstruction(lbzx);
   RegisterInstruction(lbzux);
   RegisterInstruction(lha);
   RegisterInstruction(lhau);
   RegisterInstruction(lhax);
   RegisterInstruction(lhaux);
   RegisterInstruction(lhz);
   RegisterInstruction(lhzu);
   RegisterInstruction(lhzx);
   RegisterInstruction(lhzux);
   RegisterInstruction(lwz);
   RegisterInstruction(lwzu);
   RegisterInstruction(lwzx);
   RegisterInstruction(lwzux);
   RegisterInstruction(lhbrx);
   RegisterInstruction(lwbrx);
   RegisterInstruction(lwarx);
   RegisterInstruction(lmw);
   RegisterInstruction(lswi);
   RegisterInstruction(lswx);
   RegisterInstruction(stb);
   RegisterInstruction(stbu);
   RegisterInstruction(stbx);
   RegisterInstruction(stbux);
   RegisterInstruction(sth);
   RegisterInstruction(sthu);
   RegisterInstruction(sthx);
   RegisterInstruction(sthux);
   RegisterInstruction(stw);
   RegisterInstruction(stwu);
   RegisterInstruction(stwx);
   RegisterInstruction(stwux);
   RegisterInstruction(sthbrx);
   RegisterInstruction(stwbrx);
   RegisterInstruction(stmw);
   RegisterInstruction(stswi);
   RegisterInstruction(stswx);
   RegisterInstruction(stwcx);
   RegisterInstruction(lfs);
   RegisterInstruction(lfsu);
   RegisterInstruction(lfsx);
   RegisterInstruction(lfsux);
   RegisterInstruction(lfd);
   RegisterInstruction(lfdu);
   RegisterInstruction(lfdx);
   RegisterInstruction(lfdux);
   RegisterInstruction(stfs);
   RegisterInstruction(stfsu);
   RegisterInstruction(stfsx);
   RegisterInstruction(stfsux);
   RegisterInstruction(stfd);
   RegisterInstruction(stfdu);
   RegisterInstruction(stfdx);
   RegisterInstruction(stfdux);
   RegisterInstruction(stfiwx);
   RegisterInstruction(psq_l);
   RegisterInstruction(psq_lu);
   RegisterInstruction(psq_lx);
   RegisterInstruction(psq_lux);
   RegisterInstruction(psq_st);
   RegisterInstruction(psq_stu);
   RegisterInstruction(psq_stx);
   RegisterInstruction(psq_stux);
}
