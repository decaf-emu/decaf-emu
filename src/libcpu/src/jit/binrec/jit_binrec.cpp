#include "cpu.h"
#include "cpu_breakpoints.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "jit_binrec.h"
#include "interpreter/interpreter.h"
#include "mem.h"
#include "mmu.h"

#include <cfenv>
#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <cstdlib>
#include <fmt/format.h>

#define offsetof2(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))

namespace cpu
{

namespace jit
{

static void *brChainLookup(BinrecCore *core, ppcaddr_t address);
static uint64_t brTimeBaseHandler(BinrecCore *core);
static BinrecCore *brSyscallHandler(BinrecCore *core, espresso::Instruction instr);
static BinrecCore *brTrapHandler(BinrecCore *core);

static void
brLog(void *, binrec::LogLevel level, const char *message);

BinrecBackend::BinrecBackend(size_t codeCacheSize,
                             size_t dataCacheSize)
{
   mCodeCache.initialise(codeCacheSize, dataCacheSize);
   mHandles.fill(nullptr);
}

BinrecBackend::~BinrecBackend()
{
   mCodeCache.free();
}

Core *
BinrecBackend::initialiseCore(uint32_t id)
{
   // See binrec.h.
   static const uint16_t fresTable[64] = {
      0x3FFC,0x3E1, 0x3C1C,0x3A7, 0x3875,0x371, 0x3504,0x340,
      0x31C4,0x313, 0x2EB1,0x2EA, 0x2BC8,0x2C4, 0x2904,0x2A0,
      0x2664,0x27F, 0x23E5,0x261, 0x2184,0x245, 0x1F40,0x22A,
      0x1D16,0x212, 0x1B04,0x1FB, 0x190A,0x1E5, 0x1725,0x1D1,
      0x1554,0x1BE, 0x1396,0x1AC, 0x11EB,0x19B, 0x104F,0x18B,
      0x0EC4,0x17C, 0x0D48,0x16E, 0x0BD7,0x15B, 0x0A7C,0x15B,
      0x0922,0x143, 0x07DF,0x143, 0x069C,0x12D, 0x056F,0x12D,
      0x0442,0x11A, 0x0328,0x11A, 0x020E,0x108, 0x0106,0x106
   };
   static const uint16_t frsqrteTable[64] = {
      0x7FF4,0x7A4, 0x7852,0x700, 0x7154,0x670, 0x6AE4,0x5F2,
      0x64F2,0x584, 0x5F6E,0x524, 0x5A4C,0x4CC, 0x5580,0x47E,
      0x5102,0x43A, 0x4CCA,0x3FA, 0x48D0,0x3C2, 0x450E,0x38E,
      0x4182,0x35E, 0x3E24,0x332, 0x3AF2,0x30A, 0x37E8,0x2E6,
      0x34FD,0x568, 0x2F97,0x4F3, 0x2AA5,0x48D, 0x2618,0x435,
      0x21E4,0x3E7, 0x1DFE,0x3A2, 0x1A5C,0x365, 0x16F8,0x32E,
      0x13CA,0x2FC, 0x10CE,0x2D0, 0x0DFE,0x2A8, 0x0B57,0x283,
      0x08D4,0x261, 0x0673,0x243, 0x0431,0x226, 0x020B,0x20B
   };

   auto core = new BinrecCore {};
   core->id = id;
   core->backend = this;
   core->chainLookup = brChainLookup;
   core->mftbHandler = brTimeBaseHandler;
   core->scHandler = brSyscallHandler;
   core->trapHandler = brTrapHandler;
   core->fresTable = fresTable;
   core->frsqrteTable = frsqrteTable;

#ifdef DECAF_JIT_ALLOW_PROFILING
   core->calledHLE = false;
#endif

   return core;
}

void
BinrecBackend::addReadOnlyRange(uint32_t address, uint32_t size)
{
   mReadOnlyRanges.emplace_back(address, size);
}

void
BinrecBackend::clearCache(uint32_t address, uint32_t size)
{
   if (address == 0 && size == 0xFFFFFFFF) {
      mCodeCache.clear();
      mTotalProfileTime = 0;
   } else {
      mCodeCache.invalidate(address, size);
   }
}

BinrecHandle *
BinrecBackend::createBinrecHandle()
{
   binrec::Setup setup;
   std::memset(&setup, 0, sizeof(setup));
   setup.guest = binrec::Arch::BINREC_ARCH_PPC_7XX;

#ifdef PLATFORM_WINDOWS
   setup.host = binrec::Arch::BINREC_ARCH_X86_64_WINDOWS_SEH;
#else
   setup.host = binrec::native_arch();
#endif

   setup.host_features = binrec::native_features();
   setup.guest_memory_base = reinterpret_cast<void *>(getBaseVirtualAddress());
   setup.state_offsets_ppc.gpr = offsetof2(BinrecCore, gpr);
   setup.state_offsets_ppc.fpr = offsetof2(BinrecCore, fpr);
   setup.state_offsets_ppc.gqr = offsetof2(BinrecCore, gqr);
   setup.state_offsets_ppc.lr = offsetof2(BinrecCore, lr);
   setup.state_offsets_ppc.ctr = offsetof2(BinrecCore, ctr);
   setup.state_offsets_ppc.cr = offsetof2(BinrecCore, cr);
   setup.state_offsets_ppc.xer = offsetof2(BinrecCore, xer);
   setup.state_offsets_ppc.fpscr = offsetof2(BinrecCore, fpscr);
   setup.state_offsets_ppc.pvr = offsetof2(BinrecCore, pvr);
   setup.state_offsets_ppc.pir = offsetof2(BinrecCore, id);
   setup.state_offsets_ppc.reserve_flag = offsetof2(BinrecCore, reserveFlag);
   setup.state_offsets_ppc.reserve_state = offsetof2(BinrecCore, reserveData);
   setup.state_offsets_ppc.nia = offsetof2(BinrecCore, nia);
   setup.state_offsets_ppc.timebase_handler = offsetof2(BinrecCore, mftbHandler);
   setup.state_offsets_ppc.sc_handler = offsetof2(BinrecCore, scHandler);
   setup.state_offsets_ppc.trap_handler = offsetof2(BinrecCore, trapHandler);
   setup.state_offsets_ppc.fres_lut = offsetof2(BinrecCore, fresTable);
   setup.state_offsets_ppc.frsqrte_lut = offsetof2(BinrecCore, frsqrteTable);
   setup.state_offset_chain_lookup = offsetof2(BinrecCore, chainLookup);
   setup.state_offset_branch_exit_flag = offsetof2(BinrecCore, interrupt);
   setup.log = brLog;

   auto handle = new BinrecHandle {};
   if (!handle->initialize(setup)) {
      delete handle;
      return nullptr;
   }

   handle->set_optimization_flags(mOptFlags.common, mOptFlags.guest, mOptFlags.host);
   handle->enable_branch_exit_test(true);
   handle->enable_chaining(mOptFlags.useChaining);

   if (mVerifyEnabled && mVerifyAddress == 0) {
      handle->set_pre_insn_callback(brVerifyPreHandler);
      handle->set_post_insn_callback(brVerifyPostHandler);
   }

   for (const auto &range : mReadOnlyRanges) {
      handle->add_readonly_region(range.first, range.second);
   }

   return handle;
}

CodeBlock *
BinrecBackend::checkForCodeBlockTrampoline(uint32_t address)
{
   // If the first instruction is an unconditional branch (such as for a
   // function wrapping another one), just call the target's code directly.
   // core->nia will eventually be set to a proper value, so we don't need
   // to worry that it will be out of sync here.
   auto instr = mem::read<espresso::Instruction>(address);
   auto data = espresso::decodeInstruction(instr);

   if (data && data->id == espresso::InstructionID::b && !instr.lk) {
      // Watch out for cycles when looking up the target!
      auto target = address;
      for (int tries = 10;
           tries > 0 && data && data->id == espresso::InstructionID::b && !instr.lk;
           --tries) {
         auto branchAddress = target;
         target = sign_extend<26>(instr.li << 2);

         if (!instr.aa) {
            target += branchAddress;
         }

         instr = mem::read<uint32_t>(target);
         data = espresso::decodeInstruction(instr);
      }

      if (target != address) {
         auto block = mCodeCache.getBlockByAddress(target);

         if (block) {
            // Mark this address to point to target block
            mCodeCache.setBlockIndex(address, mCodeCache.getIndex(block));
            return block;
         }
      }
   }

   return nullptr;
}

CodeBlock *
BinrecBackend::getCodeBlock(BinrecCore *core, uint32_t address)
{
   auto indexPtr = mCodeCache.getIndexPointer(address);
   auto blockIndex = indexPtr->load();

   // If block is uncompiled, let's try mark it as compiling!
   if (UNLIKELY(blockIndex == CodeBlockIndexUncompiled)) {
      if (!indexPtr->compare_exchange_strong(blockIndex, CodeBlockIndexCompiling)) {
         // Another thread has started compiling, wait for it to finish.
         while (blockIndex == CodeBlockIndexCompiling) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10us);
            blockIndex = indexPtr->load();
         }
      }
   }

   // Check if the block has been compiled
   if (LIKELY(blockIndex >= 0)) {
      auto block = mCodeCache.getBlockByIndex(blockIndex);
      return block;
   }

   // Do not try to recompile again if it failed before
   if (UNLIKELY(blockIndex == CodeBlockIndexError)) {
      return nullptr;
   }

   // Do not compile if there is a breakpoint at address.
   if (UNLIKELY(hasBreakpoint(address))) {
      return nullptr;
   }

   // Check for possible branch trampoline
   if (auto block = checkForCodeBlockTrampoline(address)) {
      return block;
   }

   auto handle = mHandles[core->id];
   if (!handle) {
      handle = createBinrecHandle();
      mHandles[core->id] = handle;
   }

   if (mVerifyEnabled && mVerifyAddress != 0) {
      if (address == mVerifyAddress) {
         handle->set_pre_insn_callback(brVerifyPreHandler);
         handle->set_post_insn_callback(brVerifyPostHandler);
      } else {
         handle->set_pre_insn_callback(nullptr);
         handle->set_post_insn_callback(nullptr);
      }
   }

   // In extreme cases (such as dense floating-point code with no
   // optimizations enabled), translation could fail due to internal
   // libbinrec limits, so try repeatedly with smaller code ranges if
   // the first translation attempt fails.
   auto limit = 4096u;
   auto size = long { 0 };
   void *buffer = nullptr;

   while (!handle->translate(core, address, address + limit - 1, &buffer, &size)) {
      limit /= 2;

      if (limit < 256) {
         gLog->warn("Failed to translate code at 0x{:X}", address);
         indexPtr->store(CodeBlockIndexError);
         return nullptr;
      }
   }

#ifdef PLATFORM_WINDOWS
   // First 8 bytes of buffer is offset to start of code
   auto codeOffset = *reinterpret_cast<uint64_t *>(buffer);
   auto unwindInfo = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + 8);
   auto unwindSize = codeOffset - 8;
   auto code = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + codeOffset);
   auto codeSize = size - codeOffset;
#else
   auto code = buffer;
   auto codeSize = size;
   void *unwindInfo = nullptr;
   auto unwindSize = size_t { 0 };
#endif

   auto block = mCodeCache.registerCodeBlock(address, code, codeSize, unwindInfo, unwindSize);
   decaf_check(block);
   free(buffer);

   // Clear any floating-point exceptions raised by the translation so
   // the translated code doesn't pick them up.
   std::feclearexcept(FE_ALL_EXCEPT);
   return block;
}

inline CodeBlock *
BinrecBackend::getCodeBlockFast(BinrecCore *core, uint32_t address)
{
   auto indexPtr = mCodeCache.getConstIndexPointer(address);
   if (LIKELY(indexPtr)) {
      auto blockIndex = indexPtr->load();
      if (LIKELY(blockIndex >= 0)) {
         auto block = mCodeCache.getBlockByIndex(blockIndex);
         return block;
      }
   }

   // Block is not yet compiled, so take the slow path.
   return getCodeBlock(core, address);
}

static inline uint64_t
rdtsc()
{
#ifdef _MSC_VER
   return __rdtsc();
#else
   uint64_t tsc;
   __asm__ volatile("rdtsc; shl $32,%%rdx; or %%rdx,%%rax"
                    : "=a" (tsc) : : "rdx");
   return tsc;
#endif
}

void
BinrecBackend::resumeExecution()
{
   auto memBase = cpu::getBaseVirtualAddress();

   // Prepare FPU state for guest code execution.
   this_core::updateRoundingMode();
   std::feclearexcept(FE_ALL_EXCEPT);

   auto core = reinterpret_cast<BinrecCore *>(this_core::state());
   decaf_check(core->nia != CALLBACK_ADDR);

   if (mVerifyEnabled) {
      // Use a separate routine for verify mode so we don't have to check
      //  the current mode on every iteration through the loop.  Note that
      //  we don't attempt to profile while verifying.
      return resumeVerifyExecution();
   }

   do {
      if (UNLIKELY(core->interrupt.load())) {
         this_core::checkInterrupts();
         // We might have been rescheduled onto a different core.
         core = reinterpret_cast<BinrecCore *>(this_core::state());
      }

      const ppcaddr_t address = core->nia;
      auto block = getCodeBlockFast(core, address);

      // To keep overhead in the non-profiling case as low as possible, we
      //  only check for zeroness of the profiling mask here, which is just
      //  a memory-immediate compare and a non-taken branch on x86.  If the
      //  mask is nonzero, we'll check again for the specific core bit on
      //  the profiling side of the test.
#ifdef DECAF_JIT_ALLOW_PROFILING
      if (LIKELY(!mProfilingMask)) {
#else
      if (1) {
#endif

         if (LIKELY(block)) {
            auto entry = reinterpret_cast<BinrecEntry>(block->code);
            core = entry(core, memBase);
         } else {
            // Step over the current instruction, in case it's confusing
            // the translator.  TODO: Consider blacklisting the address to
            // avoid trying to translate it every time we encounter it.
            interpreter::step_one(core);

            // If we just returned from a system call, we might have been
            //  rescheduled onto a different core.
            core = reinterpret_cast<BinrecCore *>(this_core::state());
         }
      } else { // mProfilingMask != 0
         const uint64_t start = rdtsc();

         if (block) {
            auto entry = reinterpret_cast<BinrecEntry>(block->code);
            core = entry(core, memBase);
         } else {
            interpreter::step_one(core);
            core = reinterpret_cast<BinrecCore *>(this_core::state());
         }

         // Don't count profiling data for HLE calls since those have
         //  nothing to do with JIT performance (and might also have
         //  caused us to switch cores, so the RDTSC difference wouldn't
         //  make any sense).
         if (UNLIKELY(core->calledHLE)) {
            core->calledHLE = false;
         } else if (block && mProfilingMask & (1 << core->id)) {
            const uint64_t time = rdtsc() - start;
            mTotalProfileTime += time;
            block->profileData.time += time;
            block->profileData.count++;
         }
      }
   } while (core->nia != CALLBACK_ADDR);
}


/**
 * Get a sample of JIT stats.
 */
bool
BinrecBackend::sampleStats(JitStats &stats)
{
   stats.totalTimeInCodeBlocks = mTotalProfileTime;
   stats.compiledBlocks = mCodeCache.getCompiledCodeBlocks();
   stats.usedCodeCacheSize = mCodeCache.getCodeCacheSize();
   stats.usedDataCacheSize = mCodeCache.getDataCacheSize();
   return true;
}


/**
 * Reset JIT profiling stats.
 */
void
BinrecBackend::resetProfileStats()
{
   // Clear block stats
   auto blocks = mCodeCache.getCompiledCodeBlocks();
   for (auto &block : blocks) {
      block.profileData.count = 0;
      block.profileData.time = 0;
   }

   // Clear generic stats
   mTotalProfileTime = 0;
}


/**
 * Set which cores to profile.
 */
void
BinrecBackend::setProfilingMask(unsigned mask)
{
   mProfilingMask = mask;
}


/**
 * Get which cores are being profiled.
 */
unsigned
BinrecBackend::getProfilingMask()
{
   return mProfilingMask;
}


/**
 * Callback from libbinrec to look up translated blocks for function chaining.
 */
void *
brChainLookup(BinrecCore *core, ppcaddr_t address)
{
   auto block = core->backend->getCodeBlock(core, address);
   if (!block) {
      return nullptr;
   }

   return block->code;
}


/**
 * Callback from libbinrec to handle time base reads.
 */
uint64_t
brTimeBaseHandler(BinrecCore *core)
{
   return core->tb();
}


/**
 * Callback from libbinrec to handle system calls.
 */
BinrecCore *
brSyscallHandler(BinrecCore *core,
                 espresso::Instruction instr)
{
   core->systemCallStackHead = core->gpr[1];
   auto handler = cpu::getSystemCallHandler(instr.kcn);
   auto newCore = handler(core, instr.kcn);

   // We might have been rescheduled on a new core.
   core = reinterpret_cast<BinrecCore *>(newCore);

   // If the next instruction is a blr, execute it ourselves rather than
   // spending the overhead of calling into JIT for just that instruction.
   auto next_instr = mem::read<uint32_t>(core->nia);
   if (next_instr == 0x4E800020) {
      core->nia = core->lr;
   }

#ifdef DECAF_JIT_ALLOW_PROFILING
   core->calledHLE = true;  // Suppress profiling for this call.
#endif
   return core;
}


/**
 * Callback from libbinrec to handle PPC trap exceptions.
 */
BinrecCore *
brTrapHandler(BinrecCore *core)
{
   if (!cpu::hasBreakpoint(core->nia)) {
      decaf_abort(fmt::format("Game raised a trap exception at 0x{:08X}.",
                              core->nia));
   }

   // If we have a breakpoint, we will fall back to interpreter to handle it.
   return core;
}


/**
 * Callback from libbinrec to handle log output.
 */
void
brLog(void *, binrec::LogLevel level, const char *message)
{
   switch (level) {
   case BINREC_LOGLEVEL_ERROR:
      gLog->error("[libbinrec] {}", message);
      break;
   case BINREC_LOGLEVEL_WARNING:
      gLog->warn("[libbinrec] {}", message);
      break;
   case BINREC_LOGLEVEL_INFO:
      // Nothing really important here, so output as debug instead
      gLog->debug("[libbinrec] {}", message);
      break;
   }
}

} // namespace jit

} // namespace cpu
