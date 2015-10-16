#include <algorithm>
#include "interpreter_insreg.h"
#include "bitutils.h"
#include "mem/mem.h"

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
      state->fpr[instr.rD].paired0 = static_cast<double>(d);
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
   return loadGeneric<float, LoadZeroRA>(state, instr);
}

static void
lfsu(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadUpdate>(state, instr);
}

static void
lfsux(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfsx(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadZeroRA | LoadIndexed>(state, instr);
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
      s = static_cast<Type>(state->fpr[instr.rS].iw0);
   } else if (std::is_floating_point<Type>::value) {
      s = static_cast<Type>(state->fpr[instr.rS].paired0);
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
   storeGeneric<float, StoreZeroRA>(state, instr);
}

static void
stfsu(ThreadState *state, Instruction instr)
{
   storeGeneric<float, StoreUpdate>(state, instr);
}

static void
stfsux(ThreadState *state, Instruction instr)
{
   storeGeneric<float, StoreUpdate | StoreIndexed>(state, instr);
}

static void
stfsx(ThreadState *state, Instruction instr)
{
   storeGeneric<float, StoreZeroRA | StoreIndexed>(state, instr);
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

// Tables copied from Dolphin source code
const static double dequantizeTable[] =
{
   1.0 / (1ULL << 0), 1.0 / (1ULL << 1), 1.0 / (1ULL << 2), 1.0 / (1ULL << 3),
   1.0 / (1ULL << 4), 1.0 / (1ULL << 5), 1.0 / (1ULL << 6), 1.0 / (1ULL << 7),
   1.0 / (1ULL << 8), 1.0 / (1ULL << 9), 1.0 / (1ULL << 10), 1.0 / (1ULL << 11),
   1.0 / (1ULL << 12), 1.0 / (1ULL << 13), 1.0 / (1ULL << 14), 1.0 / (1ULL << 15),
   1.0 / (1ULL << 16), 1.0 / (1ULL << 17), 1.0 / (1ULL << 18), 1.0 / (1ULL << 19),
   1.0 / (1ULL << 20), 1.0 / (1ULL << 21), 1.0 / (1ULL << 22), 1.0 / (1ULL << 23),
   1.0 / (1ULL << 24), 1.0 / (1ULL << 25), 1.0 / (1ULL << 26), 1.0 / (1ULL << 27),
   1.0 / (1ULL << 28), 1.0 / (1ULL << 29), 1.0 / (1ULL << 30), 1.0 / (1ULL << 31),
   (1ULL << 32), (1ULL << 31), (1ULL << 30), (1ULL << 29),
   (1ULL << 28), (1ULL << 27), (1ULL << 26), (1ULL << 25),
   (1ULL << 24), (1ULL << 23), (1ULL << 22), (1ULL << 21),
   (1ULL << 20), (1ULL << 19), (1ULL << 18), (1ULL << 17),
   (1ULL << 16), (1ULL << 15), (1ULL << 14), (1ULL << 13),
   (1ULL << 12), (1ULL << 11), (1ULL << 10), (1ULL << 9),
   (1ULL << 8), (1ULL << 7), (1ULL << 6), (1ULL << 5),
   (1ULL << 4), (1ULL << 3), (1ULL << 2), (1ULL << 1),
};

const static double quantizeTable[] =
{
   (1ULL << 0), (1ULL << 1), (1ULL << 2), (1ULL << 3),
   (1ULL << 4), (1ULL << 5), (1ULL << 6), (1ULL << 7),
   (1ULL << 8), (1ULL << 9), (1ULL << 10), (1ULL << 11),
   (1ULL << 12), (1ULL << 13), (1ULL << 14), (1ULL << 15),
   (1ULL << 16), (1ULL << 17), (1ULL << 18), (1ULL << 19),
   (1ULL << 20), (1ULL << 21), (1ULL << 22), (1ULL << 23),
   (1ULL << 24), (1ULL << 25), (1ULL << 26), (1ULL << 27),
   (1ULL << 28), (1ULL << 29), (1ULL << 30), (1ULL << 31),
   1.0 / (1ULL << 32), 1.0 / (1ULL << 31), 1.0 / (1ULL << 30), 1.0 / (1ULL << 29),
   1.0 / (1ULL << 28), 1.0 / (1ULL << 27), 1.0 / (1ULL << 26), 1.0 / (1ULL << 25),
   1.0 / (1ULL << 24), 1.0 / (1ULL << 23), 1.0 / (1ULL << 22), 1.0 / (1ULL << 21),
   1.0 / (1ULL << 20), 1.0 / (1ULL << 19), 1.0 / (1ULL << 18), 1.0 / (1ULL << 17),
   1.0 / (1ULL << 16), 1.0 / (1ULL << 15), 1.0 / (1ULL << 14), 1.0 / (1ULL << 13),
   1.0 / (1ULL << 12), 1.0 / (1ULL << 11), 1.0 / (1ULL << 10), 1.0 / (1ULL << 9),
   1.0 / (1ULL << 8), 1.0 / (1ULL << 7), 1.0 / (1ULL << 6), 1.0 / (1ULL << 5),
   1.0 / (1ULL << 4), 1.0 / (1ULL << 3), 1.0 / (1ULL << 2), 1.0 / (1ULL << 1),
};

static double
dequantize(uint32_t ea, QuantizedDataType type, uint32_t scale)
{
   double scaleValue = dequantizeTable[scale];
   double result;

   switch (type) {
   case QuantizedDataType::Floating:
      result = static_cast<double>(mem::read<float>(ea));
      break;
   case QuantizedDataType::Unsigned8:
      result = scaleValue * static_cast<double>(mem::read<uint8_t>(ea));
      break;
   case QuantizedDataType::Unsigned16:
      result = scaleValue * static_cast<double>(mem::read<uint16_t>(ea));
      break;
   case QuantizedDataType::Signed8:
      result = scaleValue * static_cast<double>(mem::read<int8_t>(ea));
      break;
   case QuantizedDataType::Signed16:
      result = scaleValue * static_cast<double>(mem::read<int16_t>(ea));
      break;
   default:
      assert("Unkown QuantizedDataType");
   }

   return result;
}

template<typename Type>
static inline double
clamp(double value)
{
   double min = static_cast<double>(std::numeric_limits<uint8_t>::min());
   double max = static_cast<double>(std::numeric_limits<uint8_t>::max());
   return std::max(min, std::min(value, max));
}

static void
quantize(uint32_t ea, double value, QuantizedDataType type, uint32_t scale)
{
   double scaleValue = dequantizeTable[scale];

   switch (type) {
   case QuantizedDataType::Floating:
      mem::write(ea, static_cast<float>(value));
      break;
   case QuantizedDataType::Unsigned8:
      value = clamp<uint8_t>(value * scaleValue);
      mem::write(ea, static_cast<uint8_t>(value));
      break;
   case QuantizedDataType::Unsigned16:
      value = clamp<uint16_t>(value * scaleValue);
      mem::write(ea, static_cast<uint16_t>(value));
      break;
   case QuantizedDataType::Signed8:
      value = clamp<int8_t>(value * scaleValue);
      mem::write(ea, static_cast<int8_t>(value));
      break;
   case QuantizedDataType::Signed16:
      value = clamp<int16_t>(value * scaleValue);
      mem::write(ea, static_cast<int16_t>(value));
      break;
   default:
      assert("Unkown QuantizedDataType");
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
      state->fpr[instr.frD].paired1 = 1.0f;
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
   double s0, s1;

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
   stt = static_cast<QuantizedDataType>(state->gqr[instr.qi].st_type);
   sts = state->gqr[instr.qi].st_scale;

   if (stt == QuantizedDataType::Unsigned8 || stt == QuantizedDataType::Signed8) {
      c = 1;
   } else if (stt == QuantizedDataType::Unsigned16 || stt == QuantizedDataType::Signed16) {
      c = 2;
   }

   s0 = state->fpr[instr.frS].paired0;
   s1 = state->fpr[instr.frS].paired1;

   if (instr.qw == 0) {
      quantize(ea, s0, stt, sts);
      quantize(ea + c, s1, stt, sts);
   } else {
      quantize(ea, s0, stt, sts);
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
