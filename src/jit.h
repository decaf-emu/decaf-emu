#pragma once
#include <map>
#include <asmjit/asmjit.h>
#include "memory.h"
#include "instruction.h"
#include "instructionid.h"
#include "ppc.h"

#define RegisterInstruction(x) \
   registerInstruction(InstructionID::##x, &x)

#define RegisterInstructionFn(x, fn) \
   registerInstruction(InstructionID::##x, &fn)

#define RegisterInstructionFallback(x) \
   registerInstruction(InstructionID::##x, &jit_fallback)

static const bool JIT_CONTINUE_ON_ERROR = false;
static const int JIT_MAX_INST = 20000;

/*
Register Assignments:
   RAX . Scratch
   RCX . Scratch
   RDX . Scratch
   RDI . Scratch
   RSI . gMemory.base()
   RBX . ThreadState*
   RBP . 
   RSP . Emu Stack Pointer.
   R8-R15 . PPCGPR Storage
*/

class PPCEmuAssembler : public asmjit::X86Assembler {
public:
   PPCEmuAssembler(asmjit::Runtime* runtime)
      : asmjit::X86Assembler(runtime, asmjit::kArchX64)
   {
      eax = zax.r32();
      ecx = zcx.r32();
      edx = zdx.r32();

#define PPCTSReg(mm) asmjit::X86Mem(zbx, (int32_t)offsetof(ThreadState, mm))
      for (auto i = 0; i < 32; ++i) {
         ppcgpr[i] = PPCTSReg(gpr[i]);
      }
      for (auto i = 0; i < 32; ++i) {
         ppcfpr[i] = PPCTSReg(fpr[i]);
         ppcfprps[i][0] = PPCTSReg(fpr[i].paired0);
         ppcfprps[i][1] = PPCTSReg(fpr[i].paired1);
      }
      ppccr = PPCTSReg(cr);
      ppcxer = PPCTSReg(xer.value);
      ppclr = PPCTSReg(lr);
      ppcctr = PPCTSReg(ctr);
      ppcfpscr = PPCTSReg(fpscr);
      for (auto i = 0; i < 8; ++i) {
         ppcgqr[i] = PPCTSReg(gqr[i].value);
      }
      ppcreserve = PPCTSReg(reserve);
      ppcreserveAddress = PPCTSReg(reserveAddress);
#undef PPCTSReg

      state = zbx;
      membase = zsi;
      cia = zdi;
   }

   void shiftTo(asmjit::X86GpReg reg, int s, int d) {
      if (s > d) {
         shr(reg, s - d);
      } else if (d < s) {
         shl(reg, d - s);
      }
   }

   asmjit::X86GpReg state;
   asmjit::X86GpReg membase;
   asmjit::X86GpReg cia;

   asmjit::X86GpReg eax;
   asmjit::X86GpReg ecx;
   asmjit::X86GpReg edx;

   asmjit::X86Mem ppcgpr[32];
   asmjit::X86Mem ppcfpr[32];
   asmjit::X86Mem ppcfprps[32][2];
   asmjit::X86Mem ppccr;
   asmjit::X86Mem ppcxer;
   asmjit::X86Mem ppclr;
   asmjit::X86Mem ppcctr;
   asmjit::X86Mem ppcfpscr;
   asmjit::X86Mem ppcgqr[8];

   asmjit::X86Mem ppcreserve;
   asmjit::X86Mem ppcreserveAddress;
};

template<typename T, typename Z>
T asmjit_cast(Z* base, size_t offset) {
   return asmjit_cast<T>(((char*)base) + offset);
}

using jitinstrfptr_t = bool(*)(PPCEmuAssembler&, Instruction);

typedef void* JitCode;
typedef void* JitFinale;
typedef uint32_t(*JitCall)(ThreadState*, JitCode);

typedef std::map<uint32_t, asmjit::Label> JumpLabelMap;

struct JitBlock {
   JitBlock(uint32_t _start) {
      start = _start;
      end = _start;
      entry = nullptr;
   }

   uint32_t start;
   uint32_t end;

   JitCode entry;
   std::map<uint32_t, JitCode> targets;
};

class JitManager {
public:
   JitManager();
   ~JitManager();

   bool initialise();

   void initStubs();
   void clearCache();
   bool prepare(uint32_t addr);
   JitCode get(uint32_t addr);
   JitCode getSingle(uint32_t addr);
   uint32_t execute(ThreadState *state, JitCode block);

private:
   bool identBlock(JitBlock& block);
   bool gen(JitBlock& block);
   bool jit_b(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
   bool jit_bc(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
   bool jit_bcctr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
   bool jit_bclr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);

   asmjit::JitRuntime* mRuntime;
   std::map<uint32_t, JitCode> mBlocks;
   std::map<uint32_t, JitCode> mSingleBlocks;
   JitCall mCallFn;
   JitFinale mFinaleFn;

public:
   static void RegisterFunctions();

private:
   static void registerInstruction(InstructionID id, jitinstrfptr_t fptr);
   static void registerBranchInstructions();
   static void registerConditionInstructions();
   static void registerFloatInstructions();
   static void registerIntegerInstructions();
   static void registerLoadStoreInstructions();
   static void registerPairedInstructions();
   static void registerSystemInstructions();

};

bool jit_fallback(PPCEmuAssembler& a, Instruction instr);

extern JitManager gJitManager;
