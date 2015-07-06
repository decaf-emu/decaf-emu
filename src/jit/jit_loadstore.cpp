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
   return jit_fallback(a, instr);
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
   if (flags & StoreConditional) {
      // Early out for if statement below.
      return jit_fallback(a, instr);
   }

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
   return jit_fallback(a, instr);
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
   return jit_fallback(a, instr);
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
   return jit_fallback(a, instr);
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
