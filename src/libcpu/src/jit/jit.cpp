#include "cpu.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter.h"
#include "jit.h"
#include "jit_internal.h"
#include "jit_verify.h"
#include "jit_vmemruntime.h"
#include "mem.h"

#include <cfenv>
#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <common/fastregionmap.h>
#include <common/log.h>
#include <common/platform_compiler.h>
#include <cstdlib>

#include <binrec++.h>

namespace cpu
{

namespace jit
{

// Undefined this to disable all profiling logic.  The profiler is fairly
//  non-intrusive if disabled, but it still costs a few cycles per JIT
//  control transfer.
#define JIT_ENABLE_PROFILING


using JitEntry = void (*)(Core *core, uintptr_t membase);

using BinrecHandle = binrec::Handle<Core *>;


// Start address for single-unit verification (0 = verify all code)
static ppcaddr_t
sVerifyAddress;

// Maximum size of JIT code cache
static size_t
sCacheSize;

// JIT code cache manager
static VMemRuntime*
sRuntime;

// Mapping from entry addresses to JIT code
static FastRegionMap<JitEntry>
sJitBlocks;

// (Stats) Total size of generated code
static uint64_t
sTotalCodeSize;

// (Stats) Bitmask of cores to profile
static unsigned int
sProfilingMask;

// (Stats) Total time spent in JIT code (RDTSC cycles)
static uint64_t
sTotalProfileTime;

// (Stats) Time spent in each JIT block (RDTSC cycles)
static FastRegionMap<uint64_t>
sBlockProfileTime;

// (Stats) Number of calls to each JIT block
static FastRegionMap<uint64_t>
sBlockProfileCount;


struct OptFlagInfo
{
   enum {
      OPTFLAG_COMMON,
      OPTFLAG_GUEST,
      OPTFLAG_HOST,
      OPTFLAG_CHAIN
   } type;
   unsigned int value;
};

static const std::map<std::string, OptFlagInfo>
sOptFlags = {
   {"BASIC",                    {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::BASIC}},
   {"DECONDITION",              {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DECONDITION}},
   {"DEEP_DATA_FLOW",           {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DEEP_DATA_FLOW}},
   {"DSE",                      {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DSE}},
   {"DSE_FP",                   {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DSE_FP}},
   {"FOLD_CONSTANTS",           {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::FOLD_CONSTANTS}},
   {"FOLD_FP_CONSTANTS",        {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::FOLD_FP_CONSTANTS}},
   {"NATIVE_IEEE_NAN",          {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::NATIVE_IEEE_NAN}},
   {"NATIVE_IEEE_UNDERFLOW",    {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::NATIVE_IEEE_UNDERFLOW}},

   {"PPC_ASSUME_NO_SNAN",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::ASSUME_NO_SNAN}},
   {"PPC_CONSTANT_GQRS",        {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::CONSTANT_GQRS}},
   {"PPC_FAST_FCTIW",           {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FCTIW}},
   {"PPC_FAST_FMADDS",          {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FMADDS}},
   {"PPC_FAST_FMULS",           {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FMULS}},
   {"PPC_FAST_STFS",            {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_STFS}},
   {"PPC_FNMADD_ZERO_SIGN",     {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FNMADD_ZERO_SIGN}},
   {"PPC_FORWARD_LOADS",        {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FORWARD_LOADS}},
   {"PPC_IGNORE_FPSCR_VXFOO",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::IGNORE_FPSCR_VXFOO}},
   {"PPC_NATIVE_RECIPROCAL",    {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::NATIVE_RECIPROCAL}},
   {"PPC_NO_FPSCR_STATE",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::NO_FPSCR_STATE}},
   {"PPC_PAIRED_LWARX_STWCX",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::PAIRED_LWARX_STWCX}},
   {"PPC_PS_STORE_DENORMALS",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::PS_STORE_DENORMALS}},
   {"PPC_TRIM_CR_STORES",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::TRIM_CR_STORES}},
   {"PPC_USE_SPLIT_FIELDS",     {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::USE_SPLIT_FIELDS}},

   {"X86_ADDRESS_OPERANDS",     {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::ADDRESS_OPERANDS}},
   {"X86_BRANCH_ALIGNMENT",     {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::BRANCH_ALIGNMENT}},
   {"X86_CONDITION_CODES",      {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::CONDITION_CODES}},
   {"X86_FIXED_REGS",           {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::FIXED_REGS}},
   {"X86_FORWARD_CONDITIONS",   {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::FORWARD_CONDITIONS}},
   {"X86_MERGE_REGS",           {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::MERGE_REGS}},
   {"X86_STORE_IMMEDIATE",      {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::STORE_IMMEDIATE}},

   // Maps to sUseChaining instead of a flag value
   {"CHAIN",                    {OptFlagInfo::OPTFLAG_CHAIN}},
};

unsigned int
gCommonOpt;

unsigned int
gGuestOpt;

unsigned int
gHostOpt;

static bool
sUseChaining;

static std::vector<std::pair<ppcaddr_t, uint32_t>>
sReadOnlyRanges;


static inline JitEntry
getJitEntry(Core *core, ppcaddr_t address);


//////// JIT callback functions

static void *
chainLookup(Core *core, ppcaddr_t address)
{
   return (void *)getJitEntry(core, address);
}


static uint64_t
mftbHandler(Core *core)
{
   return core->tb();
}


static void
scHandler(Core *core)
{
   auto instr = mem::read<espresso::Instruction>(core->nia - 4);
   auto id = instr.kcn;
   auto kc = cpu::getKernelCall(id);
   decaf_assert(kc, fmt::format("Encountered invalid Kernel Call ID {}", id));

   kc->func(core, kc->user_data);

   // We might have been rescheduled on a new core.
   core = this_core::state();

   // If the next instruction is a blr, execute it ourselves rather than
   // spending the overhead of calling into JIT for just that instruction.
   auto next_instr = mem::read<uint32_t>(core->nia);
   if (next_instr == 0x4E800020) {
      core->nia = core->lr;
   }

#ifdef JIT_ENABLE_PROFILING
   core->jitCalledHLE = true;  // Suppress profiling for this call.
#endif
}


static void
trapHandler(Core *core)
{
   decaf_abort("Game raised a trap exception. It's probably panicking.");
}


static void
verifyPreHandler(Core *core, uint32_t address)
{
   auto instr = mem::read<uint32_t>(address);
   verifyPre(core, core->jitVerifyBuffer, address, instr);
}


static void
verifyPostHandler(Core *core, uint32_t address)
{
   auto instr = mem::read<uint32_t>(address);
   verifyPost(core, core->jitVerifyBuffer, address, instr);
}


//////// Unwind table management (Windows-specific)

// TODO: This code is completely untested.

#ifdef PLATFORM_WINDOWS

static void
*sFunctionTableHandle;

static RUNTIME_FUNCTION
*sFunctionTable;

static size_t
sFunctionTableSize;

static size_t
sFunctionTableLen;

static bool
registerUnwindInfo(void *code, void *unwindInfo, size_t unwindSize)
{
   // TODO: RtlAddGrowableFunctionTable() is Windows 8+ only; Windows 7 and
   //  earlier would need one table per function, or another method such as
   //  RtlInstallFunctionTableCallback().
   if (!sFunctionTable) {
      // TODO: Assuming >=256 bytes/function on average to keep things simple.
      sFunctionTableSize = sCacheSize / 256;
      sFunctionTable = new RUNTIME_FUNCTION[sFunctionTableSize];
      sFunctionTableLen = 0;

      auto runtimeBase = static_cast<ULONG_PTR>(sRuntime->getRootAddress());
      auto runtimeLimit = static_cast<ULONG_PTR>(sRuntime->getRootAddress() + sCacheSize);
      NTSTATUS result = RtlAddGrowableFunctionTable(&sFunctionTableHandle,
                                                    sFunctionTable,
                                                    0,
                                                    sFunctionTableSize,
                                                    runtimeBase,
                                                    runtimeLimit);
      if (result != STATUS_SUCCESS) {
         gLog->error("Failed to create unwind function table: {:08X}", result);
         return false;
      }
   }

   void *unwindBuf = sRuntime->allocate(unwindSize, 8);
   if (!unwindBuf) {
      gLog->error("Out of memory for unwind info at 0x{:X} ({} bytes)", address, unwindSize);
      return false;
   }
   memcpy(unwindBuf, unwindInfo, unwindSize);

   if (sFunctionTableLen >= sFunctionTableSize) {
      gLog->error("Unwind function table is full");
      return false;
   }
   auto index = sFunctionTableLen++;

   sFunctionTable[index].BeginAddress = static_cast<DWORD>(static_cast<uintptr_t>(code) - sRuntime->getRootAddress());
   sFunctionTable[index].EndAddress = sFunctionTable[index].EndAddress + codeSize;
   sFunctionTable[index].UnwindData = static_cast<DWORD>(reinterpter_cast<uintptr_t>(unwindBuf) - sRuntime->getRootAddress);

   RtlGrowFunctionTable(sFunctionTableHandle, sFunctionTableLen);

   return true;
}


static void
unregisterUnwindTable()
{
   if (sFunctionTable) {
      RtlDeleteGrowableFunctionTable(sFunctionTable);
      sFunctionTableHandle = nullptr;
      delete[] sFunctionTable;
      sFunctionTable = nullptr;
   }
}

#endif  // PLATFORM_WINDOWS


//////// Miscellaneous internal functions

static void
initRuntime()
{
   sRuntime = new VMemRuntime(0x20000, sCacheSize);
}


static void
freeRuntime()
{
#ifdef PLATFORM_WINDOWS
   unregisterUnwindTable();
#endif

   delete sRuntime;
   sRuntime = nullptr;
}


static void
binrecLog(void *, binrec::LogLevel level, const char *message)
{
   switch (level) {
   case BINREC_LOGLEVEL_ERROR:
      gLog->error("[libbinrec] {}", message);
      break;
   case BINREC_LOGLEVEL_WARNING:
      gLog->warn("[libbinrec] {}", message);
      break;
   case BINREC_LOGLEVEL_INFO:  // Nothing really important here, so output as debug instead
      gLog->debug("[libbinrec] {}", message);
      break;
   }
}


static BinrecHandle *
createBinrecHandle()
{
   binrec::Setup setup;
   memset(&setup, 0, sizeof(setup));
   setup.guest = binrec::Arch::BINREC_ARCH_PPC_7XX;
#ifdef PLATFORM_WINDOWS
   setup.host = binrec::Arch::BINREC_ARCH_X86_64_WINDOWS_SEH;
#else
   setup.host = binrec::native_arch();
#endif
   setup.host_features = binrec::native_features();
   setup.guest_memory_base = (void *)mem::base();
   setup.state_offset_gpr = offsetof2(Core,gpr);
   setup.state_offset_fpr = offsetof2(Core,fpr);
   setup.state_offset_gqr = offsetof2(Core,gqr);
   setup.state_offset_cr = offsetof2(Core,cr);
   setup.state_offset_lr = offsetof2(Core,lr);
   setup.state_offset_ctr = offsetof2(Core,ctr);
   setup.state_offset_xer = offsetof2(Core,xer);
   setup.state_offset_fpscr = offsetof2(Core,fpscr);
   setup.state_offset_reserve_flag = offsetof2(Core,reserveFlag);
   setup.state_offset_reserve_state = offsetof2(Core,reserveData);
   setup.state_offset_nia = offsetof2(Core,nia);
   setup.state_offset_timebase_handler = offsetof2(Core,mftbHandler);
   setup.state_offset_sc_handler = offsetof2(Core,scHandler);
   setup.state_offset_trap_handler = offsetof2(Core,trapHandler);
   setup.state_offset_chain_lookup = offsetof2(Core,chainLookup);
   setup.state_offset_branch_exit_flag = offsetof2(Core,interrupt);
   setup.state_offset_fres_lut = offsetof2(Core,fresTable);
   setup.state_offset_frsqrte_lut = offsetof2(Core,frsqrteTable);
   setup.log = binrecLog;

   auto handle = new BinrecHandle;
   if (!handle->initialize(setup)) {
      delete handle;
      return nullptr;
   }
   handle->set_optimization_flags(gCommonOpt, gGuestOpt, gHostOpt);
   handle->enable_branch_exit_test(true);
   handle->enable_chaining(sUseChaining);

   if (gJitMode == jit_mode::verify && sVerifyAddress == 0) {
      handle->set_pre_insn_callback(verifyPreHandler);
      handle->set_post_insn_callback(verifyPostHandler);
   }

   for (const auto &i : sReadOnlyRanges) {
      handle->add_readonly_region(i.first, i.second);
   }

   return handle;
}


static NEVER_INLINE JitEntry
createJitEntry(Core *core, ppcaddr_t address)
{
   // If the first instruction is an unconditional branch (such as for a
   // function wrapping another one), just call the target's code directly.
   // core->nia will eventually be set to a proper value, so we don't need
   // to worry that it will be out of sync here.
   auto instr = mem::read<espresso::Instruction>(address);
   auto data = espresso::decodeInstruction(instr);
   if (data && data->id == espresso::InstructionID::b && !instr.lk) {
      // Watch out for cycles when looking up the target!
      ppcaddr_t target = address;
      for (int tries = 10;
           tries > 0 && data && data->id == espresso::InstructionID::b && !instr.lk;
           --tries) {
         ppcaddr_t branchAddress = target;
         target = sign_extend<26>(instr.li << 2);
         if (!instr.aa) {
            target += branchAddress;
         }
         instr = mem::read<uint32_t>(target);
         data = espresso::decodeInstruction(instr);
      }

      if (target != address) {
         auto targetEntry = getJitEntry(core, target);
         if (targetEntry) {
            sJitBlocks.set(address, targetEntry);
            return targetEntry;
         }
      }
   }

   static thread_local BinrecHandle *handle;  // TODO: not cleaned up

   if (!handle) {
      handle = createBinrecHandle();
      if (!handle) {
         return nullptr;
      }
   }

   if (gJitMode == jit_mode::verify && sVerifyAddress != 0) {
      if (address == sVerifyAddress) {
         handle->set_pre_insn_callback(verifyPreHandler);
         handle->set_post_insn_callback(verifyPostHandler);
      } else {
         handle->set_pre_insn_callback(nullptr);
         handle->set_post_insn_callback(nullptr);
      }
   }

   // In extreme cases (such as dense floating-point code with no
   // optimizations enabled), translation could fail due to internal
   // libbinrec limits, so try repeatedly with smaller code ranges if
   // the first translation attempt fails.
   uint32_t limit = 4096;
   void *code;
   long size;
   while (!handle->translate(core, address, address+limit-1, &code, &size)) {
      limit /= 2;
      if (limit < 256) {
         gLog->warn("Failed to translate code at 0x{:X}", address);
         return nullptr;
      }
   }

#ifdef PLATFORM_WINDOWS
   auto codeOffset = *reinterpret_cast<uint64_t *>(code);
   auto unwindInfo = static_cast<void *>(static_cast<uintptr_t>(code) + 8);
   auto unwindSize = codeOffset - 8;
   code = static_cast<void *>(static_cast<uintptr_t>(code) + codeOffset);
   size -= codeOffset;
#endif

   void *entryBuf = sRuntime->allocate(size, 16);
   if (!entryBuf) {
      gLog->error("Out of memory for translated code at 0x{:X} ({} bytes)", address, size);
      free(code);
      return nullptr;
   }

   memcpy(entryBuf, code, size);
   sJitBlocks.set(address, (JitEntry)entryBuf);
   sTotalCodeSize += size;

#ifdef PLATFORM_WINDOWS
   if (!registerUnwindInfo(entryBuf, size, unwindInfo, unwindSize)) {
      free(code);
      return nullptr;
   }
#endif

   free(code);

   // Clear any floating-point exceptions raised by the translation so
   // the translated code doesn't pick them up.
   std::feclearexcept(FE_ALL_EXCEPT);

   return (JitEntry)entryBuf;
}


static inline JitEntry
getJitEntry(Core *core, ppcaddr_t address)
{
   auto entry = sJitBlocks.find(address);
   if (LIKELY(entry)) {
      return entry;
   }

   return createJitEntry(core, address);
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


//////// libcpu interface

void
initialise(size_t cacheSize)
{
   sCacheSize = cacheSize;
   initRuntime();
}


void
initCore(Core *core)
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

   core->chainLookup = chainLookup;
   core->mftbHandler = mftbHandler;
   core->scHandler = scHandler;
   core->trapHandler = trapHandler;
   core->fresTable = fresTable;
   core->frsqrteTable = frsqrteTable;

#ifdef JIT_ENABLE_PROFILING
   core->jitCalledHLE = false;
#endif
}


void
clearCache()
{
   freeRuntime();
   initRuntime();
   sJitBlocks.clear();
   sTotalCodeSize = 0;
   sTotalProfileTime = 0;
   sBlockProfileTime.clear();
   sBlockProfileCount.clear();
}


void
setOptFlags(const std::vector<std::string> &optList)
{
   gCommonOpt = gGuestOpt = gHostOpt = 0;

   for (const auto &i : optList) {
      auto flag = sOptFlags.find(i);
      if (flag != sOptFlags.end()) {
         if (flag->second.type == OptFlagInfo::OPTFLAG_CHAIN) {
            sUseChaining = true;
         } else {
            unsigned int &flagVar =
               flag->second.type == OptFlagInfo::OPTFLAG_GUEST ? gGuestOpt :
               flag->second.type == OptFlagInfo::OPTFLAG_HOST ? gHostOpt :
               gCommonOpt;
            flagVar |= flag->second.value;
         }
      } else {
         gLog->warn("Unknown optimization flag: {}", i);
      }
   }
}


void
addReadOnlyRange(ppcaddr_t start, uint32_t size)
{
   sReadOnlyRanges.emplace_back(start, size);
}


static void
resumeWithVerify()
{
   auto core = this_core::state();

   do {
      if (core->interrupt.load()) {
         this_core::checkInterrupts();
         core = this_core::state();
      }

      const ppcaddr_t address = core->nia;
      auto entry = getJitEntry(core, core->nia);

      if (entry) {
         VerifyBuffer verifyBuf;
         core->jitVerifyBuffer = &verifyBuf;
         if (!sVerifyAddress || address == sVerifyAddress) {
            verifyInit(core, &verifyBuf);
         }
         entry(core, mem::base());
      } else {
         interpreter::step_one(core);
      }

      core = this_core::state();
   } while (core->nia != CALLBACK_ADDR);
}


void
resume()
{
   auto memBase = mem::base();

   // Prepare FPU state for guest code execution.
   this_core::updateRoundingMode();
   std::feclearexcept(FE_ALL_EXCEPT);

   auto core = this_core::state();
   decaf_check(core->nia != CALLBACK_ADDR);

   if (gJitMode == jit_mode::verify) {
      // Use a separate routine for verify mode so we don't have to check
      //  the current mode on every iteration through the loop.  Note that
      //  we don't attempt to profile while verifying.
      resumeWithVerify();
      return;
   }

   do {
      if (UNLIKELY(core->interrupt.load())) {
         this_core::checkInterrupts();
         // We might have been rescheduled onto a different core.
         core = this_core::state();
      }

      const ppcaddr_t address = core->nia;
      auto entry = getJitEntry(core, address);

      // To keep overhead in the non-profiling case as low as possible, we
      //  only check for zeroness of the profiling mask here, which is just
      //  a memory-immediate compare and a non-taken branch on x86.  If the
      //  mask is nonzero, we'll check again for the specific core bit on
      //  the profiling side of the test.
#ifdef JIT_ENABLE_PROFILING
      if (LIKELY(!sProfilingMask)) {
#else
      if (1) {
#endif

         if (LIKELY(entry)) {
            entry(core, memBase);
         } else {
            // Step over the current instruction, in case it's confusing
            // the translator.  TODO: Consider blacklisting the address to
            // avoid trying to translate it every time we encounter it.
            interpreter::step_one(core);
         }

         // If we just returned from a system call, we might have been
         //  rescheduled onto a different core.
         core = this_core::state();

      } else {  // sProfilingMask != 0

         const uint64_t start = rdtsc();

         if (entry) {
            entry(core, memBase);
         } else {
            interpreter::step_one(core);
         }

         core = this_core::state();

         // Don't count profiling data for HLE calls since those have
         //  nothing to do with JIT performance (and might also have
         //  caused us to switch cores, so the RDTSC difference wouldn't
         //  make any sense).
         if (UNLIKELY(core->jitCalledHLE)) {
            core->jitCalledHLE = false;
         } else if (sProfilingMask & (1 << core->id)) {
            const uint64_t time = rdtsc() - start;
            sTotalProfileTime += time;
            auto timePtr = sBlockProfileTime.getPtr(address);
            timePtr->store(timePtr->load() + time);
            auto countPtr = sBlockProfileCount.getPtr(address);
            countPtr->store(countPtr->load() + 1);
         }

      }

   } while (core->nia != CALLBACK_ADDR);
}


} // namespace jit


void
setJitVerifyAddress(ppcaddr_t address)
{
   jit::sVerifyAddress = address;
}


uint64_t
getJitCodeSize()
{
   return jit::sTotalCodeSize;
}


void
setJitProfilingMask(unsigned int coreMask)
{
   jit::sProfilingMask = coreMask;
}


unsigned int
getJitProfilingMask()
{
   return jit::sProfilingMask;
}


uint64_t
getJitProfileData(const FastRegionMap<uint64_t> **blockTime_ret,
                  const FastRegionMap<uint64_t> **blockCount_ret)
{
   if (blockTime_ret) {
      *blockTime_ret = &jit::sBlockProfileTime;
   }

   if (blockCount_ret) {
      *blockCount_ret = &jit::sBlockProfileCount;
   }

   return jit::sTotalProfileTime;
}


const void *
findJitEntry(ppcaddr_t address)
{
   return (void *)jit::sJitBlocks.find(address);
}


void
resetJitProfileData()
{
   // Profiling data is only intended as an estimate, so we don't bother
   //  with any sort of locking here (which means we have to iterate over
   //  the table instead of just clearing it).

   jit::sTotalProfileTime = 0;

   uint32_t address;
   uint64_t time;
   if (jit::sBlockProfileTime.getFirstEntry(&address, &time)) {
      do {
         jit::sBlockProfileTime.set(address, 0);
         jit::sBlockProfileCount.set(address, 0);
      } while (jit::sBlockProfileTime.getNextEntry(&address, &time));
   }
}


} // namespace cpu
