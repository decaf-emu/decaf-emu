#include "jit.h"
#include "interpreter.h"
#include "instructiondata.h"

static std::vector<jitinstrfptr_t>
sJitInstructionMap;

void JitManager::RegisterFunctions()
{
   static bool didInit = false;

   if (!didInit) {
      // Reserve instruction map
      sJitInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);

      // Register JIT instruction handlers
      registerBranchInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerSystemInstructions();

      didInit = true;
   }
}

void JitManager::registerInstruction(InstructionID id, jitinstrfptr_t fptr)
{
   sJitInstructionMap[static_cast<size_t>(id)] = fptr;
}

enum BoBits
{
   CtrValue = 1,
   NoCheckCtr = 2,
   CondValue = 3,
   NoCheckCond = 4
};
enum BcFlags
{
   BcCheckCtr = 1 << 0,
   BcCheckCond = 1 << 1,
   BcBranchLR = 1 << 2,
   BcBranchCTR = 1 << 3
};

bool JitManager::jit_b(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   uint32_t nia = sign_extend<26>(instr.li << 2);
   if (!instr.aa) {
      nia += cia;
   }

   if (instr.lk) {
      a.mov(a.eax, cia + 4u);
      a.mov(a.ppclr, a.eax);
      a.mov(a.eax, nia);
      a.jmp(asmjit::Ptr(mFinaleFn));
   }
   else {
      auto i = jumpLabels.find(nia);
      if (i == jumpLabels.end()) {
         // This should never happen.
         xLog() << "JIT bailing due to unidentified branch target " << Log::hex(nia);
         return false;
      }

      a.jmp(i->second);
   }

   return true;
}

bool JitManager::jit_bc(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return false;
}

bool JitManager::jit_bcctr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return false;
}

bool JitManager::jit_bclr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return false;
}

JitManager::JitManager() {
   
}

JitCode JitManager::get(uint32_t addr) {
   static bool didInit = false;
   if (!didInit) {
      PPCEmuAssembler a(&mRuntime);

      asmjit::Label introLabel(a);
      asmjit::Label extroLabel(a);

      a.bind(introLabel);
      a.push(a.zbx);
      a.push(a.zsi);
      a.sub(a.zsp, 0x28);
      a.mov(a.zbx, a.zcx);
      a.mov(a.zsi, gMemory.base());
      a.jmp(a.zdx);

      a.bind(extroLabel);
      a.add(a.zsp, 0x28);
      a.pop(a.zsi);
      a.pop(a.zbx);
      a.ret();

      auto basePtr = a.make();
      mCallFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(introLabel));
      mFinaleFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(extroLabel));
      didInit = true;
   }

   auto i = mBlocks.find(addr);
   if (i != mBlocks.end()) {
      return i->second;
   }
   return gen(addr);
}

JitCode JitManager::gen(uint32_t addr) {
   // Set it to nullptr first so we don't
   //   try to regenerate after a failed attempt.
   mBlocks[addr] = nullptr;

   PPCEmuAssembler a(&mRuntime);

   std::vector<uint32_t> jumpTargets;
   jumpTargets.reserve(100);

   auto fnStart = addr;
   auto fnMax = addr;
   auto fnEnd = addr;
   bool jitFailed = false;

   auto lclCia = addr;
   xLog() << "Attempting to JIT " << Log::hex(lclCia);

   while (lclCia) {
      auto instr = gMemory.read<Instruction>(lclCia);
      auto data = gInstructionTable.decode(instr);

      auto fptr = sJitInstructionMap[static_cast<size_t>(data->id)];
      if (false && !fptr) {
         jitFailed = true;
         xLog() << "JIT bailing due to unimplement JIT handler for " << data->name;
         break;
      }

      uint32_t nia;
      switch (data->id) {
      case InstructionID::b:
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         nia = sign_extend<26>(instr.li << 2);
         if (!instr.aa) {
            nia += lclCia;
         }
         if (!instr.lk) {
            jumpTargets.push_back(nia);
            if (nia > fnMax) {
               fnMax = nia;
            }
         }
         break;
      case InstructionID::bc:
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         nia = sign_extend<16>(instr.bd << 2);
         if (!instr.aa) {
            nia += lclCia;
         }
         if (!instr.lk) {
            jumpTargets.push_back(nia);
            if (nia > fnMax) {
               fnMax = nia;
            }
         }
         break;
      case InstructionID::bcctr:
         // Target is unknown (CTR)
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         break;
      case InstructionID::bclr:
         // Target is unknown (LR)
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         if (get_bit<NoCheckCond>(instr.bo) && get_bit<NoCheckCtr>(instr.bo)) {
            if (lclCia > fnMax) {
               fnMax = lclCia;
               fnEnd = fnMax;
            }
         }
         break;
      default:
         break;
      }

      if (jitFailed || fnEnd != fnStart) {
         // If we found an end, lets stop searching!
         break;
      }

      lclCia += 4;

      if (((lclCia - fnStart) >> 2) > JIT_MAX_INST) {
         xLog() << "Bailing on JIT due to max instruction limit at " << Log::hex(lclCia);
         break;
      }
   }

   if (jitFailed) {
      return nullptr;
   }

   xLog() << "Found end at " << Log::hex(lclCia);

   xLog() << "Found jump-targets:";
   for (auto i : jumpTargets) {
      xLog() << "  " << Log::hex(i);
   }

   std::map<uint32_t, asmjit::Label> jumpLabels;
   for (auto i : jumpTargets) {
      jumpLabels[i] = asmjit::Label(a);
   }

   // Fix VS debug viewer...
   for (int i = 0; i < 8; ++i) {
      a.nop();
   }

   asmjit::Label codeStart(a);
   a.bind(codeStart);

   lclCia = fnStart;
   while (lclCia <= fnEnd) {
      auto ciaLbl = jumpLabels.find(lclCia);
      if (ciaLbl != jumpLabels.end()) {
         a.bind(ciaLbl->second);
      }

      a.mov(a.cia, lclCia);

      auto instr = gMemory.read<Instruction>(lclCia);
      auto data = gInstructionTable.decode(instr);

      if (data->id == InstructionID::b) {
         jit_b(a, instr, lclCia, jumpLabels);
      }
      else if (data->id == InstructionID::bc) {
         jit_bc(a, instr, lclCia, jumpLabels);
      }
      else if (data->id == InstructionID::bcctr) {
         jit_bcctr(a, instr, lclCia, jumpLabels);
      }
      else if (data->id == InstructionID::bclr) {
         jit_bclr(a, instr, lclCia, jumpLabels);
      }
      else {
         auto fptr = sJitInstructionMap[static_cast<size_t>(data->id)];
         if (!fptr) {
            // Should be caught by block finder
            xLog() << "JIT bailing due to unimplement JIT handler for " << data->name;
            //jitFailed = true;
            a.int3();
            //break;
         }
         else {
            bool genSuccess = fptr(a, instr);
            if (!genSuccess) {
               xLog() << "JIT bailed due to generation failure on " << data->name;
               //jitFailed = true;
               a.int3();
               //break;
            }
         }
      }

      a.nop();

      lclCia += 4;
   }

   // Debug Check
   for (auto i = jumpLabels.begin(); i != jumpLabels.end(); ++i) {
      if (!a.isLabelValid(i->second) || !a.isLabelBound(i->second)) {
         xLog() << "Jump target " << Log::hex(i->first) << " was never initialized...";
      }
   }

   if (jitFailed) {
      return nullptr;
   }

   a.mov(a.zax, 0);
   a.jmp(asmjit::Ptr(mFinaleFn));

   JitCode func = asmjit_cast<JitCode>(a.make());
   if (func == nullptr) {
      return nullptr;
   }

   auto baseAddr = asmjit_cast<JitCode>(func, a.getLabelOffset(codeStart));
   mBlocks[fnStart] = baseAddr;
   for (auto i = jumpLabels.cbegin(); i != jumpLabels.cend(); ++i) {
      mBlocks[i->first] = asmjit_cast<JitCode>(func, a.getLabelOffset(i->second));
   }
   return baseAddr;
}

uint32_t JitManager::execute(ThreadState *state, JitCode block) {
   return mCallFn(state, block);
}