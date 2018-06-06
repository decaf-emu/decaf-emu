#include "interpreter_float.h"
#include "interpreter_insreg.h"
#include "mem.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <common/floatutils.h>
#include <fmt/format.h>
#include <mutex>

using espresso::QuantizedDataType;
using espresso::ConditionRegisterFlag;

// Load
enum LoadFlags
{
   LoadUpdate        = 1 << 0, // Save EA in rA
   LoadIndexed       = 1 << 1, // Use rB instead of d
   LoadSignExtend    = 1 << 2, // Sign extend
   LoadByteReverse   = 1 << 3, // Swap bytes
   LoadReserve       = 1 << 4, // lwarx hardware reserve
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
loadFloat(cpu::Core *state, Instruction instr)
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
loadGeneric(cpu::Core *state, Instruction instr)
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

   Type memd = mem::readNoSwap<Type>(ea);

   if (flags & LoadByteReverse) {
      d = memd;
   } else {
      d = byte_swap(memd);
   }

   if (flags & LoadReserve) {
      static_assert(!(flags & LoadReserve) || sizeof(Type) == 4, "Reserved reads are only valid on 32-bit values");

      state->reserveFlag = true;
      state->reserveData = *reinterpret_cast<uint32_t*>(&memd);
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

   if (flags & LoadUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
lbz(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadZeroRA>(state, instr);
}

static void
lbzu(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate>(state, instr);
}

static void
lbzux(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lbzx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint8_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lha(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadZeroRA>(state, instr);
}

static void
lhau(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate>(state, instr);
}

static void
lhaux(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate | LoadIndexed>(state, instr);
}

static void
lhax(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lhbrx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lhz(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadZeroRA>(state, instr);
}

static void
lhzu(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate>(state, instr);
}

static void
lhzux(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lhzx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint16_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwbrx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwarx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadReserve | LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lwz(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadZeroRA>(state, instr);
}

static void
lwzu(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate>(state, instr);
}

static void
lwzux(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lwzx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<uint32_t, LoadIndexed | LoadZeroRA>(state, instr);
}

static void
lfs(cpu::Core *state, Instruction instr)
{
   return loadFloat<LoadZeroRA>(state, instr);
}

static void
lfsu(cpu::Core *state, Instruction instr)
{
   return loadFloat<LoadUpdate>(state, instr);
}

static void
lfsux(cpu::Core *state, Instruction instr)
{
   return loadFloat<LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfsx(cpu::Core *state, Instruction instr)
{
   return loadFloat<LoadZeroRA | LoadIndexed>(state, instr);
}

static void
lfd(cpu::Core *state, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA>(state, instr);
}

static void
lfdu(cpu::Core *state, Instruction instr)
{
   return loadGeneric<double, LoadUpdate>(state, instr);
}

static void
lfdux(cpu::Core *state, Instruction instr)
{
   return loadGeneric<double, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfdx(cpu::Core *state, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA | LoadIndexed>(state, instr);
}

// Load Multiple Words
// Fills registers from rD to r31 with consecutive words from memory
static void
lmw(cpu::Core *state, Instruction instr)
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
lswGeneric(cpu::Core *state, Instruction instr)
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
lswi(cpu::Core *state, Instruction instr)
{
   lswGeneric(state, instr);
}

static void
lswx(cpu::Core *state, Instruction instr)
{
   lswGeneric<LswIndexed>(state, instr);
}

// Store
enum StoreFlags
{
   StoreUpdate          = 1 << 0, // Save EA in rA
   StoreIndexed         = 1 << 1, // Use rB instead of d
   StoreByteReverse     = 1 << 2, // Swap Bytes
   StoreConditional     = 1 << 3, // lwarx/stwcx Conditional
   StoreZeroRA          = 1 << 4, // Use 0 instead of r0
   StoreFloatAsInteger  = 1 << 5, // stfiwx
};

static void
storeDoubleAsFloat(uint32_t ea, double d)
{
   auto dBits = get_float_bits(d);
   uint32_t fBits;
   if (dBits.exponent > 896 || dBits.uv << 1 == 0) {
      // Not truncate_double()!  See the PowerPC documentation.
      fBits = truncate_double_bits(dBits.uv);
   } else {
      fBits = static_cast<uint32_t>(((1u << 23) | (dBits.mantissa >> 29)) >> (897 - dBits.exponent));
      fBits |= dBits.sign << 31;
   }
   mem::write<uint32_t>(ea, fBits);
}

template<unsigned flags = 0>
static void
storeFloat(cpu::Core *state, Instruction instr)
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

template<bool IsReserved>
struct ReservedWrite { };

template<>
struct ReservedWrite<true> {
   template<typename Type>
   static inline bool write(cpu::Core *state, uint32_t ea, Type s)
   {
      static_assert(sizeof(Type) == 4, "Reserved writes are only valid on 32-bit values");
      static_assert(sizeof(std::atomic<Type>) == sizeof(Type), "Non-locking reserve relies on zero-overhead atomics");
      auto atomicPtr = reinterpret_cast<std::atomic<Type>*>(mem::translate(ea));

      state->cr.cr0 = state->xer.so ? ConditionRegisterFlag::SummaryOverflow : 0;

      bool reserveFlag = state->reserveFlag;
      state->reserveFlag = false;
      if (!reserveFlag) {
         // The processor did not have a reservation
         return false;
      }

      auto reserveData = state->reserveData;

      if (!atomicPtr->compare_exchange_strong(reserveData, s)) {
         // The data has been modified since it was reserved.
         return false;
      }

      // Store was succesful, set CR0[EQ]
      state->cr.cr0 |= ConditionRegisterFlag::Equal;
      return true;
   }
};

template<>
struct ReservedWrite<false> {
   template<typename Type>
   static inline bool write(cpu::Core *state, uint32_t ea, Type s)
   {
      mem::writeNoSwap(ea, s);
      return true;
   }
};

template<typename Type, unsigned flags = 0>
static void
storeGeneric(cpu::Core *state, Instruction instr)
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

   if (flags & StoreFloatAsInteger) {
      s = static_cast<Type>(state->fpr[instr.rS].iw1);
   } else if (std::is_floating_point<Type>::value) {
      s = static_cast<Type>(state->fpr[instr.rS].value);
   } else {
      s = static_cast<Type>(state->gpr[instr.rS]);
   }

   if (!(flags & StoreByteReverse)) {
      s = byte_swap(s);
   }

   if (!ReservedWrite<(flags & StoreConditional) != 0>::write(state, ea, s)) {
      return;
   }

   if (flags & StoreUpdate) {
      state->gpr[instr.rA] = ea;
   }
}

static void
stb(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreZeroRA>(state, instr);
}

static void
stbu(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreUpdate>(state, instr);
}

static void
stbux(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stbx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint8_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
sth(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA>(state, instr);
}

static void
sthu(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreUpdate>(state, instr);
}

static void
sthux(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
sthx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stw(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA>(state, instr);
}

static void
stwu(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreUpdate>(state, instr);
}

static void
stwux(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stwx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
sthbrx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint16_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(state, instr);
}

static void
stwbrx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(state, instr);
}

static void
stwcx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreZeroRA | StoreConditional | StoreIndexed>(state, instr);
}

static void
stfs(cpu::Core *state, Instruction instr)
{
   storeFloat<StoreZeroRA>(state, instr);
}

static void
stfsu(cpu::Core *state, Instruction instr)
{
   storeFloat<StoreUpdate>(state, instr);
}

static void
stfsux(cpu::Core *state, Instruction instr)
{
   storeFloat<StoreUpdate | StoreIndexed>(state, instr);
}

static void
stfsx(cpu::Core *state, Instruction instr)
{
   storeFloat<StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stfd(cpu::Core *state, Instruction instr)
{
   storeGeneric<double, StoreZeroRA>(state, instr);
}

static void
stfdu(cpu::Core *state, Instruction instr)
{
   storeGeneric<double, StoreUpdate>(state, instr);
}

static void
stfdux(cpu::Core *state, Instruction instr)
{
   storeGeneric<double, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stfdx(cpu::Core *state, Instruction instr)
{
   storeGeneric<double, StoreZeroRA | StoreIndexed>(state, instr);
}

static void
stfiwx(cpu::Core *state, Instruction instr)
{
   storeGeneric<uint32_t, StoreFloatAsInteger | StoreZeroRA | StoreIndexed>(state, instr);
}

// Store Multiple Words
// Writes consecutive words to memory from rS to r31
static void
stmw(cpu::Core *state, Instruction instr)
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
stswGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t ea, i, n, r;

   ea = instr.rA ? state->gpr[instr.rA] : 0;

   if (flags & StswIndexed) {
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
stswi(cpu::Core *state, Instruction instr)
{
   stswGeneric(state, instr);
}

static void
stswx(cpu::Core *state, Instruction instr)
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
      decaf_abort(fmt::format("Unknown QuantizedDataType {}", static_cast<int>(type)));
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
      decaf_abort(fmt::format("Unknown QuantizedDataType {}", static_cast<int>(type)));
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
psqLoad(cpu::Core *state, Instruction instr)
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
psq_l(cpu::Core *state, Instruction instr)
{
   psqLoad<PsqLoadZeroRA>(state, instr);
}

static void
psq_lu(cpu::Core *state, Instruction instr)
{
   psqLoad<PsqLoadUpdate>(state, instr);
}

static void
psq_lx(cpu::Core *state, Instruction instr)
{
   psqLoad<PsqLoadZeroRA | PsqLoadIndexed>(state, instr);
}

static void
psq_lux(cpu::Core *state, Instruction instr)
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
psqStore(cpu::Core *state, Instruction instr)
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
psq_st(cpu::Core *state, Instruction instr)
{
   psqStore<PsqStoreZeroRA>(state, instr);
}

static void
psq_stu(cpu::Core *state, Instruction instr)
{
   psqStore<PsqLoadUpdate>(state, instr);
}

static void
psq_stx(cpu::Core *state, Instruction instr)
{
   psqStore<PsqStoreZeroRA | PsqStoreIndexed>(state, instr);
}

static void
psq_stux(cpu::Core *state, Instruction instr)
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
