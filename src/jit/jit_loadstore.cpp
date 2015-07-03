#include <algorithm>
#include "bitutils.h"
#include "jit.h"

// Load
enum LoadFlags
{
   LoadUpdate = 1 << 0, // Save EA in rA
   LoadIndexed = 1 << 1, // Use rB instead of d
   LoadSignExtend = 1 << 2, // Sign extend
   LoadByteReverse = 1 << 3, // Swap bytes
   LoadReserve = 1 << 4, // lwarx harware reserve
   LoadZeroRA = 1 << 5, // Use 0 instead of r0
   LoadPairedSingles = 1 << 6, // Set fpr.paired1 = d & fpr.paired2 = d
};

template<typename Type, unsigned flags = 0>
static bool
loadGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if ((flags & LoadZeroRA) && instr.rA == 0) {
      a.mov(a.ecx, 0u);
   }
   else {
      a.mov(a.ecx, a.ppcgpr[instr.rA]);
   }

   if (flags & LoadIndexed) {
      a.add(a.ecx, a.ppcgpr[instr.rB]);
   }
   else {
      a.add(a.ecx, sign_extend<16, int32_t>(instr.d));
   }

   a.mov(a.zdx, a.zcx);
   a.add(a.zdx, a.membase);
   if (sizeof(Type) == 1) {
      a.mov(a.eax, 0);
      a.mov(a.eax.r8(), asmjit::X86Mem(a.zdx, 0));
   } else if (sizeof(Type) == 2) {
      a.mov(a.eax, 0);
      a.mov(a.eax.r16(), asmjit::X86Mem(a.zdx, 0));
      if (!(flags & LoadByteReverse)) {
         a.xchg(a.eax.r8Hi(), a.eax.r8Lo());
      }
   } else if (sizeof(Type) == 4) {
      a.mov(a.eax, asmjit::X86Mem(a.zdx, 0));
      if (!(flags & LoadByteReverse)) {
         a.bswap(a.eax);
      }
   } else if (sizeof(Type) == 8) {
      a.mov(a.zax, asmjit::X86Mem(a.zdx, 0));
      if (!(flags & LoadByteReverse)) {
         a.bswap(a.zax);
      }
   } else {
      assert(0);
   }

   if (std::is_floating_point<Type>::value) {
      if (flags & LoadPairedSingles) {
         a.mov(a.ppcfprps[instr.rD][0], a.eax);
         a.mov(a.ppcfprps[instr.rD][1], a.eax);
      }
      else {
         assert(sizeof(Type) == 8);
         a.mov(a.ppcfpr[instr.rD], a.zax);
      }
   }
   else {
      if (flags & LoadSignExtend) {
         assert(sizeof(Type) == 2);
         a.movsx(a.eax, a.eax.r16());
      }

      a.mov(a.ppcgpr[instr.rD], a.eax);
   }

   if (flags & LoadReserve) {
      a.mov(a.ppcreserve, 1u);
      a.mov(a.ppcreserveAddress, a.ecx);
   }

   if (flags & LoadUpdate) {
      a.mov(a.ppcgpr[instr.rA], a.ecx);
   }
   return true;
}

static bool
lbz(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint8_t, LoadZeroRA>(a, instr);
}

static bool
lbzu(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate>(a, instr);
}

static bool
lbzux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint8_t, LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lbzx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint8_t, LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lha(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadZeroRA>(a, instr);
}

static bool
lhau(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate>(a, instr);
}

static bool
lhaux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lhax(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadSignExtend | LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lhbrx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lhz(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadZeroRA>(a, instr);
}

static bool
lhzu(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate>(a, instr);
}

static bool
lhzux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lhzx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint16_t, LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lwbrx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadByteReverse | LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lwarx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadReserve | LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lwz(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadZeroRA>(a, instr);
}

static bool
lwzu(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate>(a, instr);
}

static bool
lwzux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lwzx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<uint32_t, LoadIndexed | LoadZeroRA>(a, instr);
}

static bool
lfs(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadZeroRA>(a, instr);
}

static bool
lfsu(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadUpdate>(a, instr);
}

static bool
lfsux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lfsx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadZeroRA | LoadIndexed>(a, instr);
}

static bool
lfd(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA>(a, instr);
}

static bool
lfdu(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<double, LoadUpdate>(a, instr);
}

static bool
lfdux(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<double, LoadUpdate | LoadIndexed>(a, instr);
}

static bool
lfdx(PPCEmuAssembler& a, Instruction instr)
{
   return loadGeneric<double, LoadZeroRA | LoadIndexed>(a, instr);
}

// Load Multiple Words
// Fills registers from rD to r31 with consecutive words from memory
static bool
lmw(PPCEmuAssembler& a, Instruction instr)
{
   auto o = sign_extend<16, int32_t>(instr.d);
   if (instr.rA) {
      a.mov(a.ecx, a.ppcgpr[instr.rA]);
      a.add(a.ecx, o);
   }
   else {
      a.mov(a.ecx, o);
   }
   a.add(a.zcx, a.membase);

   for (int r = instr.rD, d = 0; r <= 31; ++r, d += 4) {
      a.mov(a.eax, asmjit::X86Mem(a.zcx, d));
      a.bswap(a.eax);
      a.mov(a.ppcgpr[r], a.eax);
   }
   return true;
}

// Load String Word (byte-by-byte version of lmw)
enum LswFlags
{
   LswIndexed = 1 >> 0,
};

template<unsigned flags = 0>
static bool
lswGeneric(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t ea, i, n, r;

   ea = instr.rA ? state->gpr[instr.rA] : 0;

   if (flags & LswIndexed) {
      ea += state->gpr[instr.rB];
      n = state->xer.byteCount;
   }
   else {
      n = instr.nb ? instr.nb : 32;
   }

   r = instr.rD - 1;
   i = 0;

   while (n > 0) {
      if (i == 0) {
         r = (r + 1) % 32;
         state->gpr[r] = 0;
      }

      state->gpr[r] |= gMemory.read<uint8_t>(ea) << (24 - i);

      i = (i + 8) % 32;
      ea = ea + 1;
      n = n - 1;
   }
   */
}

static bool
lswi(PPCEmuAssembler& a, Instruction instr)
{
   return lswGeneric(a, instr);
}

static bool
lswx(PPCEmuAssembler& a, Instruction instr)
{
   return lswGeneric<LswIndexed>(a, instr);
}

// Store
enum StoreFlags
{
   StoreUpdate = 1 << 0, // Save EA in rA
   StoreIndexed = 1 << 1, // Use rB instead of d
   StoreByteReverse = 1 << 2, // Swap Bytes
   StoreConditional = 1 << 3, // lward/stwcx Conditional
   StoreZeroRA = 1 << 4, // Use 0 instead of r0
   StoreFloatAsInteger = 1 << 5, // stfiwx
   StoreSingle = 1 << 6, // Single
};

template<typename Type, unsigned flags = 0>
static bool
storeGeneric(PPCEmuAssembler& a, Instruction instr)
{
   if ((flags & StoreZeroRA) && instr.rA == 0) {
      if (flags & StoreIndexed) {
         a.mov(a.ecx, a.ppcgpr[instr.rB]);
      } else {
         a.mov(a.ecx, sign_extend<16, int32_t>(instr.d));
      }
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rA]);

      if (flags & StoreIndexed) {
         a.add(a.ecx, a.ppcgpr[instr.rB]);
      } else {
         a.add(a.ecx, sign_extend<16, int32_t>(instr.d));
      }
   }

   if (flags & StoreConditional) {
      return false;
      /*
      state->cr.cr0 = state->xer.so ? ConditionRegisterFlag::SummaryOverflow : 0;

      if (state->reserve) {
      // Store is succesful, clear reserve bit and set CR0[EQ]
      state->cr.cr0 |= ConditionRegisterFlag::Equal;
      state->reserve = false;
      } else {
      // Reserve bit is not set, do not write.
      return;
      }
      */
   }

   a.mov(a.zdx, a.zcx);
   a.add(a.zdx, a.membase);

   if (flags & StoreFloatAsInteger) {
      return false;
      //s = static_cast<Type>(state->fpr[instr.rS].iw0);
   } else if (std::is_floating_point<Type>::value) {
      if (flags & StoreSingle) {
         assert(sizeof(Type) == 4);
         a.mov(a.eax, a.ppcfprps[instr.rS][0]);
      }
      else {
         assert(sizeof(Type) == 8);
         a.mov(a.zax, a.ppcfpr[instr.rS]);
      }
   } else {
      if (sizeof(Type) == 1) {
         a.mov(a.eax.r8(), a.ppcgpr[instr.rS]);
      } else if (sizeof(Type) == 2) {
         a.mov(a.eax.r16(), a.ppcgpr[instr.rS]);
      } else if (sizeof(Type) == 4) {
         a.mov(a.eax, a.ppcgpr[instr.rS]);
      } else {
         assert(0);
      }
   }

   if (!(flags & StoreByteReverse)) {
      if (sizeof(Type) == 1) {
         // Inverted reverse logic means we have
         //    to check for this but do nothing.
      } else if (sizeof(Type) == 2) {
         a.xchg(a.eax.r8Hi(), a.eax.r8Lo());
      } else if (sizeof(Type) == 4) {
         a.bswap(a.eax);
      } else if (sizeof(Type) == 8) {
         a.bswap(a.zax);
      } else {
         assert(0);
      }
   }

   if (sizeof(Type) == 1) {
      a.mov(asmjit::X86Mem(a.zdx, 0), a.eax.r8());
   } else if (sizeof(Type) == 2) {
      a.mov(asmjit::X86Mem(a.zdx, 0), a.eax.r16());
   } else if (sizeof(Type) == 4) {
      a.mov(asmjit::X86Mem(a.zdx, 0), a.eax);
   } else if (sizeof(Type) == 8) {
      a.mov(asmjit::X86Mem(a.zdx, 0), a.zax);
   } else {
      assert(0);
   }

   if (flags & StoreUpdate) {
      a.mov(a.ppcgpr[instr.rA], a.ecx);
   }

   return true;
}

static bool
stb(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint8_t, StoreZeroRA>(a, instr);
}

static bool
stbu(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint8_t, StoreUpdate>(a, instr);
}

static bool
stbux(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint8_t, StoreUpdate | StoreIndexed>(a, instr);
}

static bool
stbx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint8_t, StoreZeroRA | StoreIndexed>(a, instr);
}

static bool
sth(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint16_t, StoreZeroRA>(a, instr);
}

static bool
sthu(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint16_t, StoreUpdate>(a, instr);
}

static bool
sthux(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint16_t, StoreUpdate | StoreIndexed>(a, instr);
}

static bool
sthx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint16_t, StoreZeroRA | StoreIndexed>(a, instr);
}

static bool
stw(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreZeroRA>(a, instr);
}

static bool
stwu(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreUpdate>(a, instr);
}

static bool
stwux(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreUpdate | StoreIndexed>(a, instr);
}

static bool
stwx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreZeroRA | StoreIndexed>(a, instr);
}

static bool
sthbrx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint16_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(a, instr);
}

static bool
stwbrx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreZeroRA | StoreByteReverse | StoreIndexed>(a, instr);
}

static bool
stwcx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreZeroRA | StoreConditional | StoreIndexed>(a, instr);
}

static bool
stfs(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<float, StoreSingle | StoreZeroRA>(a, instr);
}

static bool
stfsu(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<float, StoreSingle | StoreUpdate>(a, instr);
}

static bool
stfsux(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<float, StoreSingle | StoreUpdate | StoreIndexed>(a, instr);
}

static bool
stfsx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<float, StoreSingle | StoreZeroRA | StoreIndexed>(a, instr);
}

static bool
stfd(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<double, StoreZeroRA>(a, instr);
}

static bool
stfdu(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<double, StoreUpdate>(a, instr);
}

static bool
stfdux(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<double, StoreUpdate | StoreIndexed>(a, instr);
}

static bool
stfdx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<double, StoreZeroRA | StoreIndexed>(a, instr);
}

static bool
stfiwx(PPCEmuAssembler& a, Instruction instr)
{
   return storeGeneric<uint32_t, StoreFloatAsInteger | StoreZeroRA | StoreIndexed>(a, instr);
}

// Store Multiple Words
// Writes consecutive words to memory from rS to r31
static bool
stmw(PPCEmuAssembler& a, Instruction instr)
{
   auto o = sign_extend<16, int32_t>(instr.d);
   if (instr.rA) {
      a.mov(a.ecx, a.ppcgpr[instr.rA]);
      a.add(a.ecx, o);
   } else {
      a.mov(a.ecx, o);
   }
   a.add(a.zcx, a.membase);
   
   for (int r = instr.rS, d = 0; r <= 31; ++r, d += 4) {
      a.mov(a.eax, a.ppcgpr[r]);
      a.bswap(a.eax);
      a.mov(asmjit::X86Mem(a.zcx, d), a.eax);
   }
   return true;
}

// Store String Word (byte-by-byte version of lmw)
enum StswFlags
{
   StswIndexed = 1 >> 0,
};

template<unsigned flags = 0>
static bool
stswGeneric(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t ea, i, n, r;

   ea = instr.rA ? state->gpr[instr.rA] : 0;

   if (flags & LswIndexed) {
      ea += state->gpr[instr.rB];
      n = state->xer.byteCount;
   }
   else {
      n = instr.nb ? instr.nb : 32;
   }

   r = instr.rS - 1;
   i = 0;

   while (n > 0) {
      if (i == 0) {
         r = (r + 1) % 32;
      }

      gMemory.write<uint8_t>(ea, (state->gpr[r] >> (24 - i)) & 0xff);

      i = (i + 8) % 32;
      ea = ea + 1;
      n = n - 1;
   }
   */
}

static bool
stswi(PPCEmuAssembler& a, Instruction instr)
{
   return stswGeneric(a, instr);
}

static bool
stswx(PPCEmuAssembler& a, Instruction instr)
{
   return stswGeneric<StswIndexed>(a, instr);
}

// Tables copied from Dolphin source code
const static float dequantizeTable[] =
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

const static float quantizeTable[] =
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

static float
dequantize(uint32_t ea, QuantizedDataType type, uint32_t scale)
{
   float scaleValue = dequantizeTable[scale];
   float result;

   switch (type) {
   case QuantizedDataType::Floating:
      result = gMemory.read<float>(ea);
      break;
   case QuantizedDataType::Unsigned8:
      result = scaleValue * static_cast<float>(gMemory.read<uint8_t>(ea));
      break;
   case QuantizedDataType::Unsigned16:
      result = scaleValue * static_cast<float>(gMemory.read<uint16_t>(ea));
      break;
   case QuantizedDataType::Signed8:
      result = scaleValue * static_cast<float>(gMemory.read<int8_t>(ea));
      break;
   case QuantizedDataType::Signed16:
      result = scaleValue * static_cast<float>(gMemory.read<int16_t>(ea));
      break;
   default:
      assert("Unkown QuantizedDataType");
   }

   return result;
}

template<typename Type>
static inline float
clamp(float value)
{
   float min = static_cast<float>(std::numeric_limits<uint8_t>::min());
   float max = static_cast<float>(std::numeric_limits<uint8_t>::max());
   return std::max(min, std::min(value, max));
}

static bool
quantize(uint32_t ea, float value, QuantizedDataType type, uint32_t scale)
{
   float scaleValue = dequantizeTable[scale];

   switch (type) {
   case QuantizedDataType::Floating:
      gMemory.write(ea, static_cast<float>(value));
      break;
   case QuantizedDataType::Unsigned8:
      value = clamp<uint8_t>(value * scaleValue);
      gMemory.write(ea, static_cast<uint8_t>(value));
      break;
   case QuantizedDataType::Unsigned16:
      value = clamp<uint16_t>(value * scaleValue);
      gMemory.write(ea, static_cast<uint16_t>(value));
      break;
   case QuantizedDataType::Signed8:
      value = clamp<int8_t>(value * scaleValue);
      gMemory.write(ea, static_cast<int8_t>(value));
      break;
   case QuantizedDataType::Signed16:
      value = clamp<int16_t>(value * scaleValue);
      gMemory.write(ea, static_cast<int16_t>(value));
      break;
   default:
      assert("Unkown QuantizedDataType");
   }
}

// Paired Single Load
enum PsqLoadFlags
{
   PsqLoadZeroRA = 1 << 0,
   PsqLoadUpdate = 1 << 1,
   PsqLoadIndexed = 1 << 2,
};

template<unsigned flags = 0>
static bool
psqLoad(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t ea, ls, c, i, w;
   QuantizedDataType lt;

   if ((flags & PsqLoadZeroRA) && instr.rA == 0) {
      ea = 0;
   }
   else {
      ea = state->gpr[instr.rA];
   }

   if (flags & PsqLoadIndexed) {
      ea += state->gpr[instr.rB];
   }
   else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   if (flags & PsqLoadIndexed) {
      i = instr.qi;
      w = instr.qw;
   }
   else {
      i = instr.i;
      w = instr.w;
   }

   c = 4;
   lt = static_cast<QuantizedDataType>(state->gqr[i].ld_type);
   ls = state->gqr[i].ld_scale;

   if (lt == QuantizedDataType::Unsigned8 || lt == QuantizedDataType::Signed8) {
      c = 1;
   }
   else if (lt == QuantizedDataType::Unsigned16 || lt == QuantizedDataType::Signed16) {
      c = 2;
   }

   if (w == 0) {
      state->fpr[instr.frD].paired0 = dequantize(ea, lt, ls);
      state->fpr[instr.frD].paired1 = dequantize(ea + c, lt, ls);
   }
   else {
      state->fpr[instr.frD].paired0 = dequantize(ea, lt, ls);
      state->fpr[instr.frD].paired1 = 1.0f;
   }

   if (flags & PsqLoadUpdate) {
      state->gpr[instr.rA] = ea;
   }
   */
}

static bool
psq_l(PPCEmuAssembler& a, Instruction instr)
{
   return psqLoad<PsqLoadZeroRA>(a, instr);
}

static bool
psq_lu(PPCEmuAssembler& a, Instruction instr)
{
   return psqLoad<PsqLoadUpdate>(a, instr);
}

static bool
psq_lx(PPCEmuAssembler& a, Instruction instr)
{
   return psqLoad<PsqLoadZeroRA | PsqLoadIndexed>(a, instr);
}

static bool
psq_lux(PPCEmuAssembler& a, Instruction instr)
{
   return psqLoad<PsqLoadUpdate | PsqLoadIndexed>(a, instr);
}

// Paired Single Store
enum PsqStoreFlags
{
   PsqStoreZeroRA = 1 << 0,
   PsqStoreUpdate = 1 << 1,
   PsqStoreIndexed = 1 << 2,
};

template<unsigned flags = 0>
static bool
psqStore(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t ea, sts, c, i, w;
   QuantizedDataType stt;
   float s0, s1;

   if ((flags & PsqStoreZeroRA) && instr.rA == 0) {
      ea = 0;
   }
   else {
      ea = state->gpr[instr.rA];
   }

   if (flags & PsqStoreIndexed) {
      ea += state->gpr[instr.rB];
   }
   else {
      ea += sign_extend<16, int32_t>(instr.d);
   }

   if (flags & PsqStoreIndexed) {
      i = instr.qi;
      w = instr.qw;
   }
   else {
      i = instr.i;
      w = instr.w;
   }

   c = 4;
   stt = static_cast<QuantizedDataType>(state->gqr[instr.qi].st_type);
   sts = state->gqr[instr.qi].st_scale;

   if (stt == QuantizedDataType::Unsigned8 || stt == QuantizedDataType::Signed8) {
      c = 1;
   }
   else if (stt == QuantizedDataType::Unsigned16 || stt == QuantizedDataType::Signed16) {
      c = 2;
   }

   s0 = state->fpr[instr.frS].paired0;
   s1 = state->fpr[instr.frS].paired1;

   if (instr.qw == 0) {
      quantize(ea, s0, stt, sts);
      quantize(ea + c, s1, stt, sts);
   }
   else {
      quantize(ea, s0, stt, sts);
   }

   if (flags & PsqStoreUpdate) {
      state->gpr[instr.rA] = ea;
   }
   */
}

static bool
psq_st(PPCEmuAssembler& a, Instruction instr)
{
   return psqStore<PsqStoreZeroRA>(a, instr);
}

static bool
psq_stu(PPCEmuAssembler& a, Instruction instr)
{
   return psqStore<PsqLoadUpdate>(a, instr);
}

static bool
psq_stx(PPCEmuAssembler& a, Instruction instr)
{
   return psqStore<PsqStoreZeroRA | PsqStoreIndexed>(a, instr);
}

static bool
psq_stux(PPCEmuAssembler& a, Instruction instr)
{
   return psqStore<PsqStoreUpdate | PsqStoreIndexed>(a, instr);
}

void JitManager::registerLoadStoreInstructions()
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
