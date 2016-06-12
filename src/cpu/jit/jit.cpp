#include "jit.h"
#include <vector>
#include <cfenv>
#include "cpu/cpu_internal.h"
#include "jit_internal.h"
#include "jit_insreg.h"
#include "cpu/mem.h"
#include "common/log.h"
#include "common/bitutils.h"
#include "cpu/espresso/espresso_instructionset.h"
#include "common/fastregionmap.h"

namespace cpu
{

namespace jit
{

static const bool JIT_DEBUG = true;
static const int JIT_MAX_INST = 500;

static std::vector<jitinstrfptr_t>
sInstructionMap;

static asmjit::JitRuntime* sRuntime;
static FastRegionMap<JitCode> sJitBlocks;

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

   sJitBlocks.clear();
   sRuntime = new asmjit::JitRuntime();
   initStubs();
}

bool jit_b(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia);
bool jit_bc(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia);
bool jit_bcctr(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia);
bool jit_bclr(PPCEmuAssembler& a, espresso::Instruction instr, uint32_t cia);

using JumpTargetList = std::vector<uint32_t>;

bool gen(JitBlock& block)
{
   PPCEmuAssembler a(sRuntime);

   auto codeStart = a.newLabel();
   a.bind(codeStart);

   auto lclCia = block.start;
   while (lclCia < block.end) {
      if (JIT_DEBUG) {
         a.mov(a.cia, lclCia);
      }

      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      bool genSuccess = false;
      if (data->id == espresso::InstructionID::b) {
         genSuccess = jit_b(a, instr, lclCia);
      } else if (data->id == espresso::InstructionID::bc) {
         genSuccess = jit_bc(a, instr, lclCia);
      } else if (data->id == espresso::InstructionID::bcctr) {
         genSuccess = jit_bcctr(a, instr, lclCia);
      } else if (data->id == espresso::InstructionID::bclr) {
         genSuccess = jit_bclr(a, instr, lclCia);
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

   a.mov(a.eax, lclCia);
   a.jmp(asmjit::Ptr(gFinaleFn));

   JitCode func = asmjit_cast<JitCode>(a.make());
   if (func == nullptr) {
      gLog->error("JIT failed due to asmjit make failure");
      return false;
   }

   auto baseAddr = asmjit_cast<JitCode>(func, a.getLabelOffset(codeStart));
   block.entry = baseAddr;

   return true;
}

bool identBlock(JitBlock& block)
{
   auto fnStart = block.start;
   auto fnEnd = fnStart;
   JumpTargetList jumpTargets;

   auto lclCia = fnStart;
   while (lclCia) {
      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         // Looks like we found a tail call function??

         fnEnd = lclCia;
         gLog->warn("Bailing on JIT {:08x} ident due to failed decode at {:08x}", block.start, lclCia);
         break;
      }

      switch (data->id) {
      case espresso::InstructionID::b:
      case espresso::InstructionID::bc:
      case espresso::InstructionID::bcctr:
      case espresso::InstructionID::bclr:
         fnEnd = lclCia + 4;
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
         fnEnd = lclCia;
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

JitCode get(uint32_t addr)
{
   auto foundBlock = sJitBlocks.find(addr);
   if (foundBlock) {
      return foundBlock;
   }

   JitBlock block(addr);

   gLog->debug("Attempting to JIT {:08x}", block.start);

   if (!identBlock(block)) {
      return nullptr;
   }

   gLog->debug("Found end at {:08x}", block.end);

   if (!gen(block)) {
      return nullptr;
   }

   sJitBlocks.set(addr, block.entry);
   for (auto i = block.targets.cbegin(); i != block.targets.cend(); ++i) {
      if (i->second) {
         sJitBlocks.set(i->first, i->second);
      }
   }
   return block.entry;
}

uint32_t execute(Core *core, JitCode block)
{
   return gCallFn(core, reinterpret_cast<uint32_t*>(&core->interrupt), block);
}

void resume(Core *core)
{
   // Before we resume, we need to update our states!
   this_core::updateRoundingMode();
   std::feclearexcept(FE_ALL_EXCEPT);

   while (core->nia != cpu::CALLBACK_ADDR) {
      JitCode jitFn = get(core->nia);
      if (!jitFn) {
         throw std::runtime_error("failed to generate JIT block to execute");
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
