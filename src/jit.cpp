#include "jit.h"
#include "log.h"
#include "interpreter.h"
#include "instructiondata.h"

JitManager
gJitManager;

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
      registerConditionInstructions();
      registerFloatInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerPairedInstructions();
      registerSystemInstructions();

      didInit = true;
   }
}

void JitManager::registerInstruction(InstructionID id, jitinstrfptr_t fptr)
{
   sJitInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool JitManager::initialise() {
   initStubs();
   return true;
}

void JitManager::initStubs() {
   PPCEmuAssembler a(mRuntime);

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
      return true;
   }

   auto i = jumpLabels.find(nia);
   if (i != jumpLabels.end()) {
      a.jmp(i->second);
   } else {
      a.mov(a.eax, nia);
      a.jmp(asmjit::Ptr(mFinaleFn));
   }

   return true;
}

template<unsigned flags>
static bool
bcGeneric(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels, JitFinale finaleFn)
{
   uint32_t bo = instr.bo;
   asmjit::Label doCondFailLbl(a);

   if (flags & BcCheckCtr) {
      if (!get_bit<NoCheckCtr>(bo)) {
         //state->ctr--;
         //ctr_ok = !!((state->ctr != 0) ^ (get_bit<CtrValue>(bo)));

         a.dec(a.ppcctr);

         a.mov(a.eax, a.ppcctr);
         a.cmp(a.eax, 0);
         if (get_bit<CtrValue>(bo)) {
            a.jne(doCondFailLbl);
         } else {
            a.je(doCondFailLbl);
         }
      }
   }

   if (flags & BcCheckCond) {
      if (!get_bit<NoCheckCond>(bo)) {
         //auto crb = get_bit(state->cr.value, 31 - instr.bi);
         //auto crv = get_bit<CondValue>(bo);
         //cond_ok = (crb == crv);

         a.mov(a.eax, a.ppccr);
         a.and_(a.eax, 1 << (31 - instr.bi));
         a.cmp(a.eax, 0);

         if (get_bit<CondValue>(bo)) {
            a.je(doCondFailLbl);
         } else {
            a.jne(doCondFailLbl);
         }
      }
   }

   if (instr.lk) {
      a.mov(a.eax, cia + 4);
      a.mov(a.ppclr, a.eax);
   }

   // Make sure no JMP related instructions end up above
   //   this if-block as we use a JMP instruction with
   //   early exit in the else block...
   if (flags & BcBranchCTR) {
      a.mov(a.eax, a.ppcctr);
      a.and_(a.eax, ~0x3);
      a.jmp(asmjit::Ptr(finaleFn));
   } else if (flags & BcBranchLR) {
      a.mov(a.eax, a.ppclr);
      a.and_(a.eax, ~0x3);
      a.jmp(asmjit::Ptr(finaleFn));
   } else {
      uint32_t nia = cia + sign_extend<16>(instr.bd << 2);
      auto i = jumpLabels.find(nia);
      if (i != jumpLabels.end()) {
         a.jmp(i->second);
      } else {
         a.mov(a.eax, nia);
         a.jmp(asmjit::Ptr(finaleFn));
      }
   }

   a.bind(doCondFailLbl);
   return true;
}

bool JitManager::jit_bc(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcCheckCtr | BcCheckCond>(a, instr, cia, jumpLabels, mFinaleFn);
}

bool JitManager::jit_bcctr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcBranchCTR | BcCheckCond>(a, instr, cia, jumpLabels, mFinaleFn);
}

bool JitManager::jit_bclr(PPCEmuAssembler& a, Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels)
{
   return bcGeneric<BcBranchLR | BcCheckCtr | BcCheckCond>(a, instr, cia, jumpLabels, mFinaleFn);
}

JitManager::JitManager()
   : mRuntime(new asmjit::JitRuntime()) {
}

JitManager::~JitManager() {
   if (mRuntime) {
      delete mRuntime;
      mRuntime = nullptr;
   }
}

void JitManager::clearCache() {
   if (mRuntime) {
      delete mRuntime;
      mRuntime = nullptr;
   }
   
   mRuntime = new asmjit::JitRuntime();
   mBlocks.clear();
   mSingleBlocks.clear();
   initStubs();
}

bool JitManager::prepare(uint32_t addr) {
   return get(addr) != nullptr;
}

JitCode JitManager::get(uint32_t addr) {
   auto i = mBlocks.find(addr);
   if (i != mBlocks.end()) {
      return i->second;
   }

   // Set it to nullptr first so we don't
   //   try to regenerate after a failed attempt.
   mBlocks[addr] = nullptr;

   JitBlock block(addr);

   gLog->debug("Attempting to JIT {:08x}", block.start);

   if (!identBlock(block)) {
      return nullptr;
   }

   gLog->debug("Found end at {:08x}", block.end);

   if (!gen(block)) {
      return nullptr;
   }

   mBlocks[block.start] = block.entry;
   for (auto i = block.targets.cbegin(); i != block.targets.cend(); ++i) {
      if (i->second) {
         mBlocks[i->first] = i->second;
      }
   }
   return block.entry;
}

JitCode JitManager::getSingle(uint32_t addr) {
   auto i = mSingleBlocks.find(addr);
   if (i != mSingleBlocks.end()) {
      return i->second;
   }

   mSingleBlocks[addr] = nullptr;

   JitBlock block(addr);
   block.end = block.start + 4;

   if (!gen(block)) {
      return nullptr;
   }

   mSingleBlocks[addr] = block.entry;
   return block.entry;
}

typedef std::vector<uint32_t> JumpTargetList;

bool JitManager::identBlock(JitBlock& block) {
   auto fnStart = block.start;
   auto fnMax = fnStart;
   auto fnEnd = fnStart;
   bool jitFailed = false;
   JumpTargetList jumpTargets;

   auto lclCia = fnStart;
   while (lclCia) {
      auto instr = gMemory.read<Instruction>(lclCia);
      auto data = gInstructionTable.decode(instr);

      if (!JIT_CONTINUE_ON_ERROR) {
         // These ifs should match the generator loop below...
         if (data->id == InstructionID::b) {
         } else if (data->id == InstructionID::bc) {
         } else if (data->id == InstructionID::bcctr) {
         } else if (data->id == InstructionID::bclr) {
         } else {
            auto fptr = sJitInstructionMap[static_cast<size_t>(data->id)];

            if (!fptr) {
               gLog->debug("JIT bailing due to unimplemented instruction {}", data->name);
               jitFailed = true;
               break;
            }
         }
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
               fnEnd = fnMax + 4;
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
         gLog->debug("Bailing on JIT due to max instruction limit at {:08x}", lclCia);
         jitFailed = true;
         break;
      }
   }

   if (jitFailed) {
      return false;
   }

   block.end = fnEnd;

   for (auto i : jumpTargets) {
      block.targets[i] = nullptr;
   }
   return true;
}

bool JitManager::gen(JitBlock& block)
{
   PPCEmuAssembler a(mRuntime);
   bool jitFailed = false;

   JumpLabelMap jumpLabels;
   for (auto i = block.targets.begin(); i != block.targets.end(); ++i) {
      if (i->first < block.start || i->first > block.end) {
         jumpLabels[i->first] = asmjit::Label(a);
      }
   }

   // Fix VS debug viewer...
   for (int i = 0; i < 8; ++i) {
      a.nop();
   }

   asmjit::Label codeStart(a);
   a.bind(codeStart);

   auto lclCia = block.start;
   while (lclCia < block.end) {
      auto ciaLbl = jumpLabels.find(lclCia);
      if (ciaLbl != jumpLabels.end()) {
         a.bind(ciaLbl->second);
      }

      a.mov(a.cia, lclCia);

      auto instr = gMemory.read<Instruction>(lclCia);
      auto data = gInstructionTable.decode(instr);

      bool genSuccess = false;
      if (data->id == InstructionID::b) {
         genSuccess = jit_b(a, instr, lclCia, jumpLabels);
      } else if (data->id == InstructionID::bc) {
         genSuccess = jit_bc(a, instr, lclCia, jumpLabels);
      } else if (data->id == InstructionID::bcctr) {
         genSuccess = jit_bcctr(a, instr, lclCia, jumpLabels);
      } else if (data->id == InstructionID::bclr) {
         genSuccess = jit_bclr(a, instr, lclCia, jumpLabels);
      } else {
         auto fptr = sJitInstructionMap[static_cast<size_t>(data->id)];
         if (fptr) {
            genSuccess = fptr(a, instr);
         }
      }

      if (!genSuccess) {
         gLog->debug("JIT bailed due to generation failure on {}", data->name);

         if (!JIT_CONTINUE_ON_ERROR) {
            jitFailed = true;
            break;
         } else {
            a.int3();
         }
      }

      a.nop();

      lclCia += 4;
   }

   if (jitFailed) {
      return false;
   }

   // Debug Check
   for (auto i = jumpLabels.begin(); i != jumpLabels.end(); ++i) {
      if (!a.isLabelValid(i->second) || !a.isLabelBound(i->second)) {
         gLog->debug("Jump target {:08x} was never initialized...", i->first);
      }
   }

   a.mov(a.eax, block.end);
   a.jmp(asmjit::Ptr(mFinaleFn));

   JitCode func = asmjit_cast<JitCode>(a.make());
   if (func == nullptr) {
      gLog->error("JIT failed due to asmjit make failure");
      return false;
   }

   auto baseAddr = asmjit_cast<JitCode>(func, a.getLabelOffset(codeStart));
   block.entry = baseAddr;
   for (auto i = jumpLabels.cbegin(); i != jumpLabels.cend(); ++i) {
      block.targets[i->first] = asmjit_cast<JitCode>(func, a.getLabelOffset(i->second));
   }
   
   return true;
}

uint32_t JitManager::execute(ThreadState *state, JitCode block) {
   return mCallFn(state, block);
}