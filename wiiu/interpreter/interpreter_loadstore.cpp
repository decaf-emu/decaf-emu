#include "bitutils.h"
#include "interpreter.h"
#include "memory.h"

// Load
enum LoadFlags
{
   LoadUpdate        = 1 << 0, // Save EA in rA
   LoadIndexed       = 1 << 1, // Use rB instead of d
   LoadSignExtend    = 1 << 2, // Sign extend
   LoadByteReverse   = 1 << 3, // Swap bytes
   LoadReserve       = 1 << 4, // lwarx harware reserve
   LoadZeroRA        = 1 << 5, // Use 0 instead of r0
   LoadPairedSingles = 1 << 6, // Set fpr.paired1 = d & fpr.paired2 = d
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
      d = gMemory.readNoSwap<Type>(ea);
   } else {
      d = gMemory.read<Type>(ea);
   }
  
   if (std::is_floating_point<Type>::value) {
      if (flags & LoadPairedSingles) {
         state->fpr[instr.rD].value = static_cast<double>(d);
      } else {
         state->fpr[instr.rD].paired0 = static_cast<float>(d);
         state->fpr[instr.rD].paired1 = static_cast<float>(d);
      }
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
   return loadGeneric<float, LoadPairedSingles | LoadZeroRA>(state, instr);
}

static void
lfsu(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadUpdate>(state, instr);
}

static void
lfsux(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadUpdate | LoadIndexed>(state, instr);
}

static void
lfsx(ThreadState *state, Instruction instr)
{
   return loadGeneric<float, LoadPairedSingles | LoadZeroRA | LoadIndexed>(state, instr);
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
      state->gpr[r] = gMemory.read<uint32_t>(ea);
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

      state->gpr[r] |= gMemory.read<uint8_t>(ea) << (24 - i);

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
   StoreUpdate       = 1 << 0, // Save EA in rA
   StoreIndexed      = 1 << 1, // Use rB instead of d
   StoreByteReverse  = 1 << 2, // Swap Bytes
   StoreConditional  = 1 << 3, // lward/stwcx Conditional
   StoreZeroRA       = 1 << 4, // Use 0 instead of r0
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
         // Store is succesful, clear reserve bit and set CR0[EQ]
         state->cr.cr0 |= ConditionRegisterFlag::Equal;
         state->reserve = false;
      } else {
         // Reserve bit is not set, so clear CR0[EQ] and do not write
         state->cr.cr0 &= ~ConditionRegisterFlag::Equal;
         return;
      }
   }

   if (std::is_floating_point<Type>::value) {
      s = static_cast<Type>(state->fpr[instr.rS].value);
   } else {
      s = static_cast<Type>(state->gpr[instr.rS]);
   }

   if (flags & StoreByteReverse) {
      // Write already does byte_swap, so we writeNoSwap for byte reverse
      gMemory.writeNoSwap<Type>(ea, s);
   } else {
      gMemory.write<Type>(ea, s);
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
      gMemory.write<uint32_t>(ea, state->gpr[r]);
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

      gMemory.write<uint8_t>(ea, (state->gpr[r] >> (24 - i)) & 0xff);

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

void Interpreter::registerLoadStoreInstructions()
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
}
