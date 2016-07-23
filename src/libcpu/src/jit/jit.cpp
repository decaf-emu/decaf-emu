#include "common/log.h"
#include "common/align.h"
#include "common/bitutils.h"
#include "common/decaf_assert.h"
#include "common/fastregionmap.h"
#include "cpu.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "jit.h"
#include "jit_internal.h"
#include "jit_insreg.h"
#include "jit_verify.h"
#include "jit_vmemruntime.h"
#include "mem.h"
#include <algorithm>
#include <array>
#include <cfenv>
#include <map>
#include <vector>

namespace cpu
{

namespace jit
{

static const bool JIT_DEBUG = true;
static const int JIT_MAX_INST = 3000;
static const bool JIT_REGCACHE = true;

// Insert NOPs at the beginning of a generated block of code.
//  The Visual Studio disassembler can get confused without these.
static const bool JIT_INITIAL_NOPS =
#ifdef _MSC_VER
   true;
#else
   false;
#endif

static std::vector<jitinstrfptr_t>
sInstructionMap;

static VMemRuntime*
sRuntime;

static FastRegionMap<JitCode>
sJitBlocks;

static std::array<uint8_t, 32>
sBaseRelocCode;

static void *
sPreInstr;

static void *
sPostInstr;

JitCall
gCallFn;

JitFinale
gFinaleFn;

void
registerUnwindTable(VMemRuntime *runtime, intptr_t jitCallAddr);

void
unregisterUnwindTable();

JitCode
jit_continue(uint32_t addr, JitCode *jumpSource);

static void
initStubs()
{
   PPCEmuAssembler a(sRuntime);

   if (JIT_DEBUG && JIT_INITIAL_NOPS) {
      for (auto i = 0; i < 12; ++i) {
         a.nop();
      }
   }

   auto introLabel = a.newLabel();
   auto extroLabel = a.newLabel();
   auto exitLabel = a.newLabel();
   auto verifyPreLabel = a.newLabel();
   auto verifyPostLabel = a.newLabel();

   uint32_t stackSpace = 0x20;  // Windows shadow space
   if (gJitMode == jit_mode::verify) {
      // Leave space for the verification args/buffer on the stack:
      //  RSP+32 = Address of instruction being verified
      //  RSP+36 = Instruction word being verified
      //  RSP+40 = verification buffer
      stackSpace += 8 + static_cast<uint32_t>(sizeof(VerifyBuffer));
   }
   // Ensure the stack remains 16-byte aligned -- note that this depends
   //  on the number of pushes below; we have an even number of pushes,
   //  and the incoming CALL instruction adds 8 bytes, so we need to
   //  reserve a stack size of 16n+8.
   stackSpace = align_up(stackSpace - 8, 16) + 8;

   // This is invoked to set up the neccessary state for
   //  the JIT block we want to execute.
   a.bind(introLabel);
   a.push(asmjit::x86::rbp);
   a.push(asmjit::x86::rbx);
#ifdef PLATFORM_WINDOWS
   a.push(asmjit::x86::rdi);  // RSI/RDI are callee-saved in Windows x64 ABI
   a.push(asmjit::x86::rsi);
#endif
   a.push(asmjit::x86::r12);
   a.push(asmjit::x86::r13);
   a.push(asmjit::x86::r14);
   a.push(asmjit::x86::r15);
   a.sub(asmjit::x86::rsp, stackSpace);
   a.mov(a.stateReg, a.sysArgReg[0]);
   a.mov(a.membaseReg, static_cast<uint64_t>(mem::base()));
   a.jmp(a.sysArgReg[1]);

   // This is the piece of code executed when we are finished
   //  executing the block of code started above.
   a.bind(extroLabel);

   a.cmp(a.finaleNiaArgReg, CALLBACK_ADDR);
   a.je(exitLabel);

   // If we should continue generating, lets call the
   //  generator instead to find our new address!
   a.mov(asmjit::x86::rax, asmjit::Ptr(jit_continue));
   a.call(asmjit::x86::rax);
   a.jmp(asmjit::x86::rax);

   // This is how we exit back to the caller
   a.bind(exitLabel);
   a.mov(a.niaMem, a.finaleNiaArgReg);
   a.mov(asmjit::x86::rax, a.stateReg);
   a.add(asmjit::x86::rsp, stackSpace);
   a.pop(asmjit::x86::r15);
   a.pop(asmjit::x86::r14);
   a.pop(asmjit::x86::r13);
   a.pop(asmjit::x86::r12);
#ifdef PLATFORM_WINDOWS
   a.pop(asmjit::x86::rsi);
   a.pop(asmjit::x86::rdi);
#endif
   a.pop(asmjit::x86::rbx);
   a.pop(asmjit::x86::rbp);
   a.ret();

   if (gJitMode == jit_mode::verify) {
      // This wraps the instruction verification setup to minimize the
      //  number of instructions inserted into the translated code stream.
      //  We manually save and restore volatile registers so the process of
      //  verification doesn't change the generated code.
      auto verifyLabel = a.newLabel();
      a.bind(verifyLabel);

      // Save all registers in the JIT register pool
      int extraStackSpace = 0x20;  // Windows shadow space
      int savedGpRegs = 0;
      auto numRegs = static_cast<int32_t>(a.mRegs.size());
      for (int i = 0; i < numRegs; ++i) {
         if (a.mRegs[i].regType == PPCEmuAssembler::RegType::Gp) {
            const asmjit::X86GpReg &reg = a.mGpRegVals[a.mRegs[i].regId];
            if (reg.getRegIndex() != asmjit::kX86RegIndexAx) {  // Saved by entry point
               a.push(reg);
            }
            savedGpRegs++;
         }
      }
      // Realign the stack -- we do this if the push count is even, because
      //  there's a return address on the stack as well
      if (savedGpRegs % 2 == 0) {
         extraStackSpace += 8;
      }
      for (int i = 0; i < numRegs; ++i) {
         if (a.mRegs[i].regType == PPCEmuAssembler::RegType::Xmm) {
            extraStackSpace += 16;
         }
      }
      a.sub(asmjit::x86::rsp, extraStackSpace);
      int xmmPushOffset = 32;
      for (int i = 0; i < numRegs; ++i) {
         if (a.mRegs[i].regType == PPCEmuAssembler::RegType::Xmm) {
            a.movdqa(asmjit::X86Mem(asmjit::x86::rsp, xmmPushOffset, 16), a.mXmmRegVals[a.mRegs[i].regId]);
            xmmPushOffset += 16;
         }
      }

      // Call the verification function
      // Don't forget to take into account the CALL and pushes we just did
      //  when generating these stack references!
      int stackOffset = 8 + 8*savedGpRegs + extraStackSpace;
      a.mov(a.sysArgReg[2], asmjit::X86Mem(asmjit::x86::rsp, stackOffset+32, 4));
      a.mov(a.sysArgReg[3], asmjit::X86Mem(asmjit::x86::rsp, stackOffset+36, 4));
      a.mov(a.sysArgReg[0], a.stateReg);
      a.lea(a.sysArgReg[1], asmjit::X86Mem(asmjit::x86::rsp, stackOffset+40));
      a.call(asmjit::x86::rax);

      // Restore registers (in reverse order!) and return
      for (int i = numRegs - 1; i >= 0; --i) {
         if (a.mRegs[i].regType == PPCEmuAssembler::RegType::Xmm) {
            xmmPushOffset -= 16;
            a.movdqa(a.mXmmRegVals[a.mRegs[i].regId], asmjit::X86Mem(asmjit::x86::rsp, xmmPushOffset, 16));
         }
      }
      a.add(asmjit::x86::rsp, extraStackSpace);
      for (int i = numRegs - 1; i >= 0; --i) {
         if (a.mRegs[i].regType == PPCEmuAssembler::RegType::Gp) {
            a.pop(a.mGpRegVals[a.mRegs[i].regId]);
         }
      }
      a.ret();

      // Call target for pre-instruction verification setup
      a.bind(verifyPreLabel);
      a.push(asmjit::x86::rax);
      a.mov(asmjit::x86::rax, asmjit::Ptr(verifyPre));
      a.jmp(verifyLabel);

      // Call target for post-instruction verification
      a.bind(verifyPostLabel);
      a.push(asmjit::x86::rax);
      a.mov(asmjit::x86::rax, asmjit::Ptr(verifyPost));
      a.jmp(verifyLabel);
   }

   auto basePtr = a.make();
   gCallFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(introLabel));
   gFinaleFn = asmjit_cast<JitCall>(basePtr, a.getLabelOffset(extroLabel));
   if (gJitMode == jit_mode::verify) {
      sPreInstr = asmjit_cast<void *>(basePtr, a.getLabelOffset(verifyPreLabel));
      sPostInstr = asmjit_cast<void *>(basePtr, a.getLabelOffset(verifyPostLabel));
   } else {
      sPreInstr = nullptr;
      sPostInstr = nullptr;
   }

   asmjit::StaticRuntime rr(sBaseRelocCode.data(), sBaseRelocCode.size());
   asmjit::X86Assembler ra(&rr, asmjit::kArchX64);
   ra.mov(a.finaleNiaArgReg, asmjit::imm_u(0x12345678));
   decaf_check(ra.getOffset() == 5);
   ra.mov(a.finaleJmpSrcArgReg, asmjit::imm_u(0x123456789abcdef0));
   decaf_check(ra.getOffset() == 15);
   for (auto i = 15; i < 32; ++i) {
      ra.nop();
   }
   ra.make();
   decaf_check(ra.getOffset() == 32);
}

void
initialiseRuntime()
{
   sRuntime = new VMemRuntime(0x20000, 0x40000000);
   initStubs();
   registerUnwindTable(sRuntime, reinterpret_cast<intptr_t>(gCallFn));
}

void
freeRuntime()
{
   if (sRuntime) {
      unregisterUnwindTable();
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
   // Note: This must not be called unless there is guarenteed to be
   //  nobody currently executing code!

   freeRuntime();
   initialiseRuntime();

   sJitBlocks.clear();
}

using JumpTargetList = std::vector<uint32_t>;

void
jit_b_direct(PPCEmuAssembler& a, ppcaddr_t addr)
{
   a.saveAll();

   auto target = sJitBlocks.find(addr);
   if (target) {
      // We already know where this function is, let's just jump
      //  directly to it rather than wasting time going through
      //  the JIT dispatcher!
      a.jmp(asmjit::Ptr(target));
   } else {
      // We have not JIT'd this section yet.  Let's allocate some
      //  space for an aligned MOV instruction, then mark it as a
      //  relocation so it can be filled by the 'linker' below.
      auto relocLbl = a.newLabel();
      a.bind(relocLbl);

      // Save 32 bytes of memory so we have room to do set up the
      //  call during relocation once we know where its going to
      //  reside in the host jit memory section.
      for (auto i = 0; i < 32; ++i) {
         a.int3();
      }
      a.jmp(asmjit::x86::rax);

      a.relocLabels.emplace_back(addr, relocLbl);
   }
}

bool
gen(JitBlock &block)
{
   PPCEmuAssembler a(sRuntime);
   a.relocLabels.reserve(10);

   struct TargetLblPair {
      uint32_t idx;
      asmjit::Label label;
   };
   std::map<uint32_t, TargetLblPair> targetLbls;
   for (uint32_t i = 0; i < block.targets.size(); ++i) {
      targetLbls.emplace(block.targets[i].first, TargetLblPair{ i, a.newLabel() });
   }

   auto codeStart = a.newLabel();
   uint32_t lclCia;
   a.bind(codeStart);

   if (JIT_DEBUG && JIT_INITIAL_NOPS) {
      for (auto i = 0; i < 12; ++i) {
         a.nop();
      }
   }

   for (lclCia = block.start; lclCia < block.end; lclCia += 4)
   {
      auto targetIter = targetLbls.find(lclCia);
      if (targetIter != targetLbls.end()) {
         // This is a jump target, we should flush any register caches
         //  and then also insert a label so we can find this location.
         a.bind(targetIter->second.label);
      }

      if (JIT_DEBUG) {
         a.mov(a.niaMem, lclCia + 4);
      }

      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         a.ud2();
      } else {
         // Don't attempt to verify non-repeatable instructions
         bool doVerify = (gJitMode == jit_mode::verify
                          && data->id != espresso::InstructionID::kc
                          && data->id != espresso::InstructionID::lwarx
                          && data->id != espresso::InstructionID::stwcx);
         if (doVerify) {
            insertVerifyCall(a, instr, sPreInstr);
         }

         a.genCia = lclCia;

         auto genSuccess = false;

         auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
         if (fptr) {
            genSuccess = fptr(a, instr);
         }

         if (!genSuccess) {
            a.int3();
         }

         if (doVerify) {
            insertVerifyCall(a, instr, sPostInstr);
         }
      }

      if (!JIT_REGCACHE) {
         a.evictAll();
      }

      if (JIT_DEBUG) {
         a.nop();
      }
   }

   jit_b_direct(a, lclCia);

   auto func = asmjit_cast<JitCode>(a.make());

   if (func == nullptr) {
      gLog->error("JIT failed due to asmjit make failure");
      return false;
   }

   // Write in the relocation data that jumps to the Finale, which can
   //  later be overwritten atomically by the generator.
   for (auto &reloc : a.relocLabels) {
      // We use a trick here to save some bytes.  We write the addr
      //  part of the info while relocating in spite of being able
      //  to do it during initial generation, this allows us to move
      //  it to before or after the aligned MOV which saves us some
      //  bytes that would otherwise be wasted on NOP's.

      auto targetAddr = asmjit::Ptr(gFinaleFn);

      // Find our bytes of memory allocated above...
      auto mem = asmjit_cast<uint8_t*>(func, a.getLabelOffset(reloc.second));

      auto aligned_offset = align_up(mem + 15 + 2, 8) - mem;
      auto aligned_mov_offset = aligned_offset - 2;
      auto aligned_base_offset = reinterpret_cast<intptr_t>(mem + aligned_offset);

      // Copy the pregenerated relocation code bytes
      std::copy(sBaseRelocCode.begin(), sBaseRelocCode.end(), mem);

      // Write addr of `MOV finaleNiaArgReg, addr`
      *reinterpret_cast<uint32_t*>(&mem[1]) = reloc.first;

      // Write relmem of `MOV finaleJmpSrcArgReg, relmem`
      *reinterpret_cast<intptr_t*>(&mem[7]) = aligned_base_offset;

      // Write `MOV RAX, target`
      mem[aligned_mov_offset + 0] = 0x48;
      mem[aligned_mov_offset + 1] = 0xB8;
      auto atomicAddr = &mem[aligned_mov_offset + 2];
      decaf_check(align_up(atomicAddr, 8) == atomicAddr);
      *reinterpret_cast<uint64_t*>(atomicAddr) = targetAddr;
   }

   // Calculate the starting address of the block
   auto baseAddr = asmjit_cast<JitCode>(func, a.getLabelOffset(codeStart));
   block.entry = baseAddr;

   // Generate all the offset labels for these relocations
   for (auto &target : targetLbls) {
      if (a.isLabelBound(target.second.label)) {
         block.targets[target.second.idx].second =
            asmjit_cast<JitCode>(func, a.getLabelOffset(target.second.label));
      }
   }

   return true;
}

bool
identBlock(JitBlock& block)
{
   auto fnStart = block.start;
   auto fnEnd = fnStart;
   auto lclCia = fnStart;

   while (lclCia) {
      auto instr = mem::read<espresso::Instruction>(lclCia);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         // Looks like we found a tail call function??
         fnEnd = lclCia + 4;
         gLog->warn("Bailing on JIT {:08x} ident due to failed decode at {:08x}", block.start, lclCia);
         break;
      }

      // Targets should be added to block.targets if we know of any...

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

JitCode
jit_continue(uint32_t nia, JitCode *jumpSource)
{
   // This would be strange...
   decaf_check(nia != CALLBACK_ADDR);

   // Log the branch if branch tracing is enabled
   if (gBranchTraceHandler) {
      gBranchTraceHandler(nia);
   }

   // Locate or generate the next JIT section
   JitCode jitFn = get(nia);

   // We do not update the jumpSource if branch tracing is enabled,
   //  this is because it would cause those branches to avoid calling
   //  here ever again...
   if (jumpSource && !gBranchTraceHandler) {
      // Aligned writes on x64 are guarenteed to be atomic
      *jumpSource = jitFn;
   }

   return jitFn;
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

   decaf_check(core->nia != CALLBACK_ADDR);

   JitCode jitFn = jit_continue(core->nia, nullptr);
   core = execute(core, jitFn);

   decaf_check(core == this_core::state());
   decaf_check(core->nia == CALLBACK_ADDR);
}

bool
PPCEmuAssembler::ErrorHandler::handleError(asmjit::Error code, const char* message, void *origin) noexcept
{
   gLog->error("ASMJit Error {}: {}\n", code, message);
   return true;
}

} // namespace jit

} // namespace cpu
