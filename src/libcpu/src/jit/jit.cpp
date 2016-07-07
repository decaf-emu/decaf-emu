#include "common/log.h"
#include "common/bitutils.h"
#include "common/fastregionmap.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "jit.h"
#include "jit_internal.h"
#include "jit_insreg.h"
#include "jit_vmemruntime.h"
#include "mem.h"
#include <vector>
#include <cfenv>

namespace cpu
{

namespace jit
{

static const bool JIT_DEBUG = true;
static const int JIT_MAX_INST = 3000;

static std::vector<jitinstrfptr_t>
sInstructionMap;

static VMemRuntime* sRuntime;
static FastRegionMap<JitCode> sJitBlocks;

JitCall gCallFn;
JitFinale gFinaleFn;

void
registerUnwindTable(VMemRuntime *runtime, intptr_t jitCallAddr);

void
initStubs()
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
   a.mov(a.zsi, static_cast<uint64_t>(mem::base()));
   a.jmp(a.zdx);

   a.bind(extroLabel);
   a.mov(a.ppcnia, a.eax);
   a.mov(a.zax, a.state);
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

void
initialiseRuntime()
{
   sRuntime = new VMemRuntime(0x20000, 0x40000000);
   initStubs();
   // TODO: Need to unregister this when the runtime is destroyed
   registerUnwindTable(sRuntime, reinterpret_cast<intptr_t>(gCallFn));
}

void
freeRuntime()
{
   if (sRuntime) {
      delete sRuntime;
      sRuntime = nullptr;
   }
}

void
initialise()
{
   initialiseRuntime();

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

jitinstrfptr_t
getInstructionHandler(espresso::InstructionID id)
{
   auto instrId = static_cast<size_t>(id);

   if (instrId >= sInstructionMap.size()) {
      return nullptr;
   }

   return sInstructionMap[instrId];
}

void
registerInstruction(espresso::InstructionID id, jitinstrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool
hasInstruction(espresso::InstructionID instrId)
{
   return getInstructionHandler(instrId) != nullptr;
}

void
clearCache()
{
   freeRuntime();
   initialiseRuntime();
   sJitBlocks.clear();
}

using JumpTargetList = std::vector<uint32_t>;

bool
gen(JitBlock &block)
{
   PPCEmuAssembler a(sRuntime);
   auto codeStart = a.newLabel();
   uint32_t lclCia;
   a.bind(codeStart);

   // Visual studio disassembler sucks.  We need to write some NOPS!
   for (auto i = 0; i < 8; ++i) {
      a.nop();
   }

   for (lclCia = block.start; lclCia < block.end; lclCia += 4) {
      if (JIT_DEBUG) {
         a.mov(a.cia, lclCia);
         a.mov(a.ppcnia, lclCia + 4);
      }

      a.genCia = lclCia;

      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);
      auto genSuccess = false;

      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
      if (fptr) {
         genSuccess = fptr(a, instr);
      }

      if (!genSuccess) {
         a.int3();
      }

      if (JIT_DEBUG) {
         a.nop();
      }
   }

   a.mov(a.eax, lclCia);
   a.jmp(asmjit::Ptr(gFinaleFn));

   auto func = asmjit_cast<JitCode>(a.make());

   if (func == nullptr) {
      gLog->error("JIT failed due to asmjit make failure");
      return false;
   }

   auto baseAddr = asmjit_cast<JitCode>(func, a.getLabelOffset(codeStart));
   block.entry = baseAddr;
   return true;
}

bool
identBlock(JitBlock& block)
{
   JumpTargetList jumpTargets;
   auto fnStart = block.start;
   auto fnEnd = fnStart;
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

JitCode
get(uint32_t addr)
{
   auto foundBlock = sJitBlocks.find(addr);
   if (foundBlock) {
      return foundBlock;
   }

   auto block = JitBlock { addr };

   if (!identBlock(block)) {
      return nullptr;
   }

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

Core *
execute(Core *core, JitCode block)
{
   return gCallFn(core, block);
}

void
resume()
{
   // Before we resume, we need to update our states!
   this_core::updateRoundingMode();
   std::feclearexcept(FE_ALL_EXCEPT);

   // Grab the core as we currently know it
   auto core = this_core::state();

   // Just to help when debugging
   core->cia = 0xFFFFFFFD;

   // Loop around executing blocks of JIT'd code
   while (core->nia != cpu::CALLBACK_ADDR) {
      JitCode jitFn = get(core->nia);
      if (!jitFn) {
         throw std::runtime_error("failed to generate JIT block to execute");
      }

      core = execute(core, jitFn);

      if (gBranchTraceHandler) {
         gBranchTraceHandler(core->nia);
      }
   }
}

bool
PPCEmuAssembler::ErrorHandler::handleError(asmjit::Error code, const char* message, void *origin) noexcept
{
   gLog->error("ASMJit Error {}: {}\n", code, message);
   return true;
}

} // namespace jit

} // namespace cpu
