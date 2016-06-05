#include "jit.h"
#include <vector>
#include <cfenv>
#include "cpu/cpu_internal.h"
#include "jit_internal.h"
#include "jit_insreg.h"
#include "cpu/mem.h"
#include "utils/log.h"
#include "utils/bitutils.h"
#include "cpu/espresso/espresso_instructionset.h"

namespace cpu
{

namespace jit
{

static const bool JIT_DEBUG = true;
static const int JIT_MAX_INST = 500;

static std::vector<jitinstrfptr_t>
sInstructionMap;

static asmjit::JitRuntime* sRuntime;
static std::map<uint32_t, JitCode> sBlocks;
static std::map<uint32_t, JitCode> sSingleBlocks;

JitCall gCallFn;
JitFinale gFinaleFn;

void initStubs()
{
   PPCEmuAssembler a(sRuntime);

   auto introLabel = a.newLabel();
   auto extroLabel = a.newLabel();

   a.bind(introLabel);
   a.push(a.zbx);
   a.push(a.zdi);
   a.push(a.zsi);
   a.push(asmjit::x86::r12);
   a.sub(a.zsp, 0x38);
   a.mov(a.zbx, a.zcx);
   a.mov(asmjit::x86::r12, a.zdx);
   a.mov(a.zsi, static_cast<uint64_t>(mem::base()));
   a.jmp(asmjit::x86::r8d);

   a.bind(extroLabel);
   a.add(a.zsp, 0x38);
   a.pop(asmjit::x86::r12);
   a.pop(a.zsi);
   a.pop(a.zdi);
   a.pop(a.zbx);
   a.ret();

   auto basePtr = a.make();
   gCallFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(introLabel));
   gFinaleFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(extroLabel));
}

void initialise()
{
   sRuntime = new asmjit::JitRuntime();
   initStubs();

   sInstructionMap.resize(static_cast<size_t>(espresso::InstructionID::InstructionCount), nullptr);

   // Register instruction handlers
   registerBranchInstructions();
   registerConditionInstructions();
   registerFloatInstructions();
   registerIntegerInstructions();
   registerLoadStoreInstructions();
   registerPairedInstructions();
   registerSystemInstructions();
}

jitinstrfptr_t getInstructionHandler(espresso::InstructionID id)
{
   auto instrId = static_cast<size_t>(id);
   if (instrId >= sInstructionMap.size()) {
      return nullptr;
   }
   return sInstructionMap[instrId];
}

void registerInstruction(espresso::InstructionID id, jitinstrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool hasInstruction(espresso::InstructionID instrId)
{
   switch (instrId) {
   case espresso::InstructionID::b:
      return true;
   case espresso::InstructionID::bc:
      return true;
   case espresso::InstructionID::bcctr:
      return true;
   case espresso::InstructionID::bclr:
      return true;
   default:
      return getInstructionHandler(instrId) != nullptr;
   }
}

void clearCache()
{
   if (sRuntime) {
      delete sRuntime;
      sRuntime = nullptr;
   }

   sRuntime = new asmjit::JitRuntime();
   sBlocks.clear();
   sSingleBlocks.clear();
   initStubs();
}

bool jit_b(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
bool jit_bc(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
bool jit_bcctr(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);
bool jit_bclr(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia, const JumpLabelMap& jumpLabels);

using JumpTargetList = std::vector<uint32_t>;

bool gen(JitBlock& block)
{
   PPCEmuAssembler a(sRuntime);

   JumpLabelMap jumpLabels;
   for (auto i = block.targets.begin(); i != block.targets.end(); ++i) {
      if (i->first >= block.start && i->first < block.end) {
         jumpLabels[i->first] = a.newLabel();
      }
   }

   // Fix VS debug viewer...
   if (JIT_DEBUG) {
      for (int i = 0; i < 8; ++i) {
         a.nop();
      }
   }

   auto codeStart = a.newLabel();
   a.bind(codeStart);

   auto lclCia = block.start;
   while (lclCia < block.end) {
      auto ciaLbl = jumpLabels.find(lclCia);
      if (ciaLbl != jumpLabels.end()) {
         a.bind(ciaLbl->second);
      }

      if (JIT_DEBUG) {
         a.mov(a.cia, lclCia);
      }

      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      bool genSuccess = false;
      if (data->id == espresso::InstructionID::b) {
         genSuccess = jit_b(a, instr, lclCia, jumpLabels);
      } else if (data->id == espresso::InstructionID::bc) {
         genSuccess = jit_bc(a, instr, lclCia, jumpLabels);
      } else if (data->id == espresso::InstructionID::bcctr) {
         genSuccess = jit_bcctr(a, instr, lclCia, jumpLabels);
      } else if (data->id == espresso::InstructionID::bclr) {
         genSuccess = jit_bclr(a, instr, lclCia, jumpLabels);
      } else {
         auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
         if (fptr) {
            genSuccess = fptr(a, instr);
         }
      }

      if (!genSuccess) {
         a.int3();
      }

      if (JIT_DEBUG) {
         a.nop();
      }

      lclCia += 4;
   }

   // Debug Check
   for (auto i = jumpLabels.begin(); i != jumpLabels.end(); ++i) {
      if (!a.isLabelValid(i->second) || !a.isLabelBound(i->second)) {
         gLog->error("Jump target {:08x} was never initialized...", i->first);
      }
   }

   a.mov(a.eax, block.end);
   a.jmp(asmjit::Ptr(gFinaleFn));

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

bool identBlock(JitBlock& block)
{
   auto fnStart = block.start;
   auto fnMax = fnStart;
   auto fnEnd = fnStart;
   JumpTargetList jumpTargets;

   auto lclCia = fnStart;
   while (lclCia) {
      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         // Looks like we found a tail call function??

         fnMax = lclCia - 4;
         fnEnd = fnMax + 4;
         gLog->warn("Bailing on JIT {:08x} ident due to failed decode at {:08x}", block.start, lclCia);
         break;
      }

      uint32_t nia;
      switch (data->id) {
      case espresso::InstructionID::b:
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
      case espresso::InstructionID::bc:
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
      case espresso::InstructionID::bcctr:
         // Target is unknown (CTR)
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         break;
      case espresso::InstructionID::bclr:
         // Target is unknown (LR)
         if (instr.lk) {
            jumpTargets.push_back(lclCia + 4);
         }

         if (get_bit<2>(instr.bo) && get_bit<4>(instr.bo)) {
            if (lclCia > fnMax) {
               fnMax = lclCia;
               fnEnd = fnMax + 4;
            }
         }
         break;
      default:
         break;
      }

      if (fnEnd != fnStart) {
         // If we found an end, lets stop searching!
         break;
      }

      lclCia += 4;

      if (((lclCia - fnStart) >> 2) > JIT_MAX_INST) {
         fnMax = lclCia - 4;
         fnEnd = fnMax + 4;
         gLog->trace("Bailing on JIT {:08x} due to max instruction limit at {:08x}", block.start, lclCia);
         break;
      }
   }

   block.end = fnEnd;

   for (auto i : jumpTargets) {
      block.targets[i] = nullptr;
   }

   return true;
}

static std::mutex sMutex;

JitCode get(uint32_t addr)
{
   std::unique_lock<std::mutex> lock(sMutex);

   auto i = sBlocks.find(addr);
   if (i != sBlocks.end()) {
      return i->second;
   }

   // Set it to nullptr first so we don't
   //   try to regenerate after a failed attempt.
   sBlocks[addr] = nullptr;

   JitBlock block(addr);

   gLog->debug("Attempting to JIT {:08x}", block.start);

   if (!identBlock(block)) {
      return nullptr;
   }

   gLog->debug("Found end at {:08x}", block.end);

   if (!gen(block)) {
      return nullptr;
   }

   sBlocks[block.start] = block.entry;
   for (auto i = block.targets.cbegin(); i != block.targets.cend(); ++i) {
      if (i->second) {
         sBlocks[i->first] = i->second;
      }
   }
   return block.entry;
}

bool prepare(uint32_t addr)
{
   return get(addr) != nullptr;
}

JitCode getSingle(uint32_t addr)
{
   auto i = sSingleBlocks.find(addr);
   if (i != sSingleBlocks.end()) {
      return i->second;
   }

   sSingleBlocks[addr] = nullptr;

   JitBlock block(addr);
   block.end = block.start + 4;

   if (!gen(block)) {
      return nullptr;
   }

   sSingleBlocks[addr] = block.entry;
   return block.entry;
}

uint32_t execute(Core *core, JitCode block)
{
   return gCallFn(core, block);
}

void resume(Core *core)
{
   // Before we resume, we need to update our states!
   this_core::update_rounding_mode();
   std::feclearexcept(FE_ALL_EXCEPT);

   while (core->nia != cpu::CALLBACK_ADDR) {
      JitCode jitFn = get(core->nia);
      if (!jitFn) {
         throw;
      }

      auto newNia = execute(core, jitFn);
      core->cia = 0;
      core->nia = newNia;
   }
}

bool PPCEmuAssembler::ErrorHandler::handleError(asmjit::Error code, const char* message, void *origin) noexcept
{
   gLog->error("ASMJit Error {}: {}\n", code, message);
   return true;
}

} // namespace jit

} // namespace cpu
