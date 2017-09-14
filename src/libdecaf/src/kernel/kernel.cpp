#include "debugger/debugger.h"
#include "decaf_events.h"
#include "filesystem/filesystem.h"
#include "kernel.h"
#include "kernel_hle.h"
#include "kernel_internal.h"
#include "kernel_ipc.h"
#include "kernel_loader.h"
#include "kernel_memory.h"
#include "kernel_filesystem.h"
#include "ios/ios.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_appio.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_enum.h"
#include "modules/coreinit/coreinit_mcp.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_shared.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_interrupts.h"
#include "modules/gx2/gx2_event.h"
#include "modules/sci/sci_cafe_settings.h"
#include "modules/sci/sci_caffeine_settings.h"
#include "modules/sci/sci_parental_account_settings_uc.h"
#include "modules/sci/sci_parental_settings.h"
#include "modules/sci/sci_spot_pass_settings.h"
#include "ppcutils/wfunc_call.h"
#include "ppcutils/stackobject.h"

#include <common/decaf_assert.h>
#include <common/platform_dir.h>
#include <common/platform_fiber.h>
#include <common/platform_thread.h>
#include <common/teenyheap.h>
#include <fmt/format.h>
#include <libcpu/mem.h>
#include <pugixml.hpp>

namespace coreinit
{
struct OSContext;
struct OSThread;
}

namespace kernel
{

namespace functions
{
void
kcTraceHandler(const std::string& str)
{
   traceLogSyscall(str);
   gLog->debug(str);
}
}

enum class FaultReason : uint32_t {
   Unknown,
   Segfault,
   IllInst
};

static void
cpuEntrypoint();

static void
cpuInterruptHandler(uint32_t interrupt_flags);

static void
cpuSegfaultHandler(uint32_t address);

static void
cpuIllInstHandler();

static void
cpuBranchTraceHandler(uint32_t target);

static bool
launchGame();

static bool
sRunning = true;

static std::string
sExecutableName;

static TeenyHeap *
sSystemHeap = nullptr;

static loader::LoadedModule *
sUserModule;

static int
sExitCode = 0;

static FaultReason
sFaultReason = FaultReason::Unknown;

static uint32_t
sSegfaultAddress = 0;

static coreinit::OSContext
sInterruptContext[3];

static decaf::GameInfo
sGameInfo;

void
setExecutableFilename(const std::string& name)
{
   sExecutableName = name;
}

void
initialise()
{
   initialiseVirtualMemory();
   ios::start();
   initialiseHleMmodules();
   cpu::setCoreEntrypointHandler(&cpuEntrypoint);
   cpu::setSegfaultHandler(&cpuSegfaultHandler);
   cpu::setIllInstHandler(&cpuIllInstHandler);
   cpu::setInterruptHandler(&cpuInterruptHandler);

   if (decaf::config::log::branch_trace) {
      cpu::setBranchTraceHandler(&cpuBranchTraceHandler);
   }

   auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::SystemHeap);
   sSystemHeap = new TeenyHeap { cpu::VirtualPointer<void> { bounds.start }.getRawPointer(), bounds.size };
}

void
shutdown()
{
   ios::join();
}

TeenyHeap *
getSystemHeap()
{
   return sSystemHeap;
}

static void
cpuBranchTraceHandler(uint32_t target)
{
   auto symNamePtr = loader::findSymbolNameForAddress(target);
   if (!symNamePtr) {
      return;
   }

   gLog->debug("CPU branched to: {}", *symNamePtr);
}

static std::string
coreStateToString(cpu::Core *core)
{
   fmt::MemoryWriter out;
   out.write("nia: 0x{:08x}\n", core->nia);
   out.write("lr: 0x{:08x}\n", core->lr);
   out.write("cr: 0x{:08x}\n", core->cr.value);
   out.write("ctr: 0x{:08x}\n", core->ctr);
   out.write("xer: 0x{:08x}\n", core->xer.value);

   for (auto i = 0u; i < 32; ++i) {
      out.write("gpr[{}]: 0x{:08x}\n", i, core->gpr[i]);
   }

   return out.str();
}

static void
cpuFaultFiberEntryPoint(void *addr)
{
   // We may have been in the middle of a kernel function...
   if (coreinit::internal::isSchedulerLocked()) {
      coreinit::internal::unlockScheduler();
   }

   // Move back an instruction so we can re-exucute the failed instruction
   //  and so that the debugger shows the right stop point.
   cpu::this_core::state()->nia -= 4;

   // Alert the debugger if it cares.
   if (decaf::config::debugger::enabled) {
      coreinit::internal::pauseCoreTime(true);
      debugger::handleDebugBreakInterrupt();
      coreinit::internal::pauseCoreTime(false);

      // This will shut down the thread and reschedule.  This is required
      //  since returning from the segfault handler is an error.
      coreinit::OSExitThread(0);
   }

   auto core = cpu::this_core::state();
   decaf_assert(core, "Uh oh? CPU fault Handler with invalid core");

   gLog->critical("{}", coreStateToString(core));

   if (sFaultReason == FaultReason::Segfault) {
      decaf_abort(fmt::format("Invalid memory access for address {:08X} with nia 0x{:08X}\n",
         sSegfaultAddress, core->nia));
   } else if (sFaultReason == FaultReason::IllInst) {
      decaf_abort(fmt::format("Invalid instruction at nia 0x{:08X}\n",
         core->nia));
   } else {
      decaf_abort(fmt::format("Unexpected fault occured, fault reason was {} at 0x{:08X}\n",
         static_cast<uint32_t>(sFaultReason), core->nia));
   }
}

static void
cpuSegfaultHandler(uint32_t address)
{
   sFaultReason = FaultReason::Segfault;
   sSegfaultAddress = address;

   // A bit of trickery to get a stable stack and other
   //  host platform context after an exception occurs.
   auto thread = coreinit::internal::getCurrentThread();
   reallocateContextFiber(&thread->context, cpuFaultFiberEntryPoint);
}

static void
cpuIllInstHandler()
{
   sFaultReason = FaultReason::IllInst;

   // A bit of trickery to get a stable stack and other
   //  host platform context after an exception occurs.
   auto thread = coreinit::internal::getCurrentThread();
   reallocateContextFiber(&thread->context, cpuFaultFiberEntryPoint);
}

static void
initCoreInterruptContext()
{
   auto coreId = cpu::this_core::id();
   auto &context = sInterruptContext[coreId];

   auto stack = kernel::getSystemHeap()->alloc(1024);

   memset(&context, 0, sizeof(coreinit::OSContext));
   context.gpr[1] = mem::untranslate(stack) + 1024 - 4;
}

void
cpuInterruptHandler(uint32_t interrupt_flags)
{
   if (interrupt_flags & cpu::SRESET_INTERRUPT) {
      platform::exitThread(0);
   }

   if (interrupt_flags & cpu::DBGBREAK_INTERRUPT) {
      if (decaf::config::debugger::enabled) {
         coreinit::internal::pauseCoreTime(true);
         debugger::handleDebugBreakInterrupt();
         coreinit::internal::pauseCoreTime(false);
      }
   }

   auto unsafeInterrupts = cpu::NONMASKABLE_INTERRUPTS | cpu::DBGBREAK_INTERRUPT;
   if (!(interrupt_flags & ~unsafeInterrupts)) {
      // Due to the fact that non-maskable interrupts are not able to be disabled
      // it is possible the application has the scheduler lock or something, so we
      // need to stop processing here or else bad things could happen.
      return;
   }

   // We need to disable the scheduler while we handle interrupts so we
   // do not reschedule before we are done with our interrupts.  We disable
   // interrupts if they were on so any PPC callbacks executed do not
   // immediately and reentrantly interrupt.  We also make sure did not
   // interrupt someone who disabled the scheduler, since that should never
   // happen and will cause bugs.

   decaf_check(coreinit::internal::isSchedulerEnabled());

   coreinit::OSContext savedContext;
   kernel::saveContext(&savedContext);
   kernel::restoreContext(&sInterruptContext[cpu::this_core::id()]);

   auto originalInterruptState = coreinit::OSDisableInterrupts();
   coreinit::internal::disableScheduler();

   auto interruptedThread = coreinit::internal::getCurrentThread();

   if (interrupt_flags & cpu::ALARM_INTERRUPT) {
      coreinit::internal::handleAlarmInterrupt(&interruptedThread->context);
   }

   if (interrupt_flags & cpu::GPU_RETIRE_INTERRUPT) {
      gx2::internal::handleGpuRetireInterrupt();
   }

   if (interrupt_flags & cpu::GPU_FLIP_INTERRUPT) {
      gx2::internal::handleGpuFlipInterrupt();
   }

   if (interrupt_flags & cpu::IPC_INTERRUPT) {
      kernel::ipcDriverKernelHandleInterrupt();
   }

   coreinit::internal::enableScheduler();
   coreinit::OSRestoreInterrupts(originalInterruptState);

   kernel::restoreContext(&savedContext);

   // We must never receive an interrupt while processing a kernel
   // function as if the scheduler is locked, we are in for some shit.
   coreinit::internal::lockScheduler();
   coreinit::internal::checkRunningThreadNoLock(false);
   coreinit::internal::unlockScheduler();
}

void
cpuEntrypoint()
{
   // Make a fibre, we need to be cautious not to allocate here
   //  as this fibre can be arbitrarily destroyed.
   initCoreFiber();

   // Set up an interrupt context to use with this core
   initCoreInterruptContext();

   // We set up a dummy stack on each core to help us invoke needed stuff
   auto rootStack = static_cast<uint8_t*>(coreinit::internal::sysAlloc(256, 4));
   auto core = cpu::this_core::state();
   core->gpr[1] = mem::untranslate(rootStack + 256 - 4);

   // Maybe launch the game
   if (cpu::this_core::id() == 1) {
      // Run the setup on core 1, which will also run the loader
      launchGame();

      // Trip an interrupt on core 1 to force it to schedule the loader.
      cpu::interrupt(1, cpu::GENERIC_INTERRUPT);
   }

   // Set up the default expected state for the nia/cia of idle threads.
   //  This must be kept in sync with reschedule which sets them to this
   //  for debugging purposes.
   core->nia = 0xFFFFFFFF;
   core->cia = 0xFFFFFFFF;

   // Run the scheduler loop, this is what will
   //   execute when there is nothing else to do.
   while (sRunning) {
      cpu::this_core::waitForInterrupt();
   }
}

static bool
prepareMLC()
{
   auto fileSystem = getFileSystem();

   // Temporarily set mlc to write so we can create folders
   fileSystem->setPermissions("/vol/storage_mlc01", fs::Permissions::ReadWrite, fs::PermissionFlags::Recursive);

   // Create title folder
   auto titleID = sGameInfo.app.title_id;
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto titlePath = fmt::format("/vol/storage_mlc01/sys/title/{:08x}/{:08x}", titleHi, titleLo);
   auto titleFolder = fileSystem->makeFolder(titlePath);

   // Create Mii database folder
   fileSystem->makeFolder("/vol/storage_mlc01/usr/save/00050010/1004a100/user/common/db");

   // Restore mlc to Read only
   fileSystem->setPermissions("/vol/storage_mlc01", fs::Permissions::Read, fs::PermissionFlags::Recursive);

   // Set title folder to ReadWrite
   fileSystem->setPermissions(titlePath, fs::Permissions::ReadWrite, fs::PermissionFlags::Recursive);

   // Set mlc/usr to ReadWrite
   fileSystem->setPermissions("/vol/storage_mlc01/usr", fs::Permissions::ReadWrite, fs::PermissionFlags::Recursive);
   return true;
}

static bool
prepareSLC()
{
   auto fileSystem = getFileSystem();

   // Temporarily set slc to write so we can create folders
   fileSystem->setPermissions("/vol/system_slc", fs::Permissions::ReadWrite, fs::PermissionFlags::Recursive);

   // Initialise settings
   ppcutils::StackObject<sci::SCICafeSettings> cafe;
   sci::SCIInitCafeSettings(cafe);

   ppcutils::StackObject<sci::SCICaffeineSettings> caffeine;
   sci::SCIInitCaffeineSettings(caffeine);

   ppcutils::StackObject<sci::SCIParentalAccountSettingsUC> parentAccountUC;
   sci::SCIInitParentalAccountSettingsUC(parentAccountUC, 1);

   ppcutils::StackObject<sci::SCIParentalSettings> parental;
   sci::SCIInitParentalSettings(parental);

   ppcutils::StackObject<sci::SCISpotPassSettings> spotPass;
   sci::SCIInitSpotPassSettings(spotPass);

   // Restore slc to Read only
   fileSystem->setPermissions("/vol/system_slc", fs::Permissions::Read, fs::PermissionFlags::Recursive);

   // Set configuration folder to ReadWrite
   fileSystem->setPermissions("/vol/system_slc/proc/prefs", fs::Permissions::ReadWrite, fs::PermissionFlags::Recursive);
   return true;
}

bool
launchGame()
{
   if (!loadGameInfo(sGameInfo)) {
      gLog->warn("Could not load game info.");
   } else {
      gLog->info("Loaded game info: '{}' - {} v{}", sGameInfo.meta.shortnames[decaf::Language::English], sGameInfo.meta.product_code, sGameInfo.meta.title_version);
   }

   auto rpx = sGameInfo.cos.argstr;

   if (rpx.empty()) {
      rpx = sExecutableName;
   }

   if (rpx.empty()) {
      gLog->error("Could not find game executable to load.");
      return false;
   }

   // Set up the application memory with max_codesize
   initialiseAppMemory(sGameInfo.cos.max_codesize);

   // Load the application-level kernel binding
   auto coreinitModule = loader::loadRPL("coreinit");

   if (!coreinitModule) {
      gLog->error("Could not load system coreinit library");
      return false;
   }

   // Load the application
   auto appModule = loader::loadRPL(rpx);

   if (!appModule) {
      gLog->error("Could not load {}", rpx);
      return false;
   }

   gLog->debug("Succesfully loaded {}", rpx);
   sUserModule = appModule;

   // Setup title path
   auto fileSystem = getFileSystem();

   if (!prepareMLC()) {
      gLog->error("Failed to prepare MLC");
      return false;
   }

   // Mount SD card if game has permission to use it
   if ((sGameInfo.cos.permission_fs & decaf::CosXML::SdCardRead) ||
       (sGameInfo.cos.permission_fs & decaf::CosXML::SdCardWrite)) {
      // Ensure sdcard_path exists
      platform::createDirectory(decaf::config::system::sdcard_path);

      // Mount sdcard
      auto sdcardPath = fs::HostPath { decaf::config::system::sdcard_path };
      auto permission = fs::Permissions::Read;

      if (sGameInfo.cos.permission_fs & decaf::CosXML::SdCardWrite) {
         permission = fs::Permissions::ReadWrite;
      }

      fileSystem->mountHostFolder("/dev/sdcard01", sdcardPath, permission);
   }

   // We need to set some default stuff up...
   auto core = cpu::this_core::state();
   core->gqr[2].value = 0x40004;
   core->gqr[3].value = 0x50005;
   core->gqr[4].value = 0x60006;
   core->gqr[5].value = 0x70007;

   // Setup coreinit threads
   coreinit::internal::startAlarmCallbackThreads();
   coreinit::internal::startAppIoThreads();
   coreinit::internal::startDefaultCoreThreads();
   coreinit::internal::startDeallocatorThreads();

   // Initialise CafeOS
   coreinit::internal::mcpInit();
   coreinit::internal::loadSharedData();

   // Notify frontend that game has loaded
   decaf::event::onGameLoaded(sGameInfo);

   // Start the entry thread!
   auto gameThreadEntry = coreinitModule->findFuncExport<uint32_t, uint32_t, void*>("GameThreadEntry");
   coreinit::OSRunThread(coreinit::OSGetDefaultThread(1), gameThreadEntry, 0, nullptr);

   return true;
}

void
kernelEntry()
{
   if (!prepareSLC()) {
      gLog->error("Failed to prepare SLC");
   }
}

loader::LoadedModule *
getUserModule()
{
   return sUserModule;
}

loader::LoadedModule *
getTLSModule(uint32_t index)
{
   auto modules = loader::getLoadedModules();

   for (auto &module : modules) {
      if (module.second->tlsModuleIndex == index) {
         return module.second;
      }
   }

   return nullptr;
}

void
exitProcess(int code)
{
   sExitCode = code;
   sRunning = false;
   cpu::halt();
   setContext(nullptr);
}

bool
hasExited()
{
   return !sRunning;
}

int
getExitCode()
{
   return sExitCode;
}

const decaf::GameInfo &
getGameInfo()
{
   return sGameInfo;
}

} // namespace kernel
