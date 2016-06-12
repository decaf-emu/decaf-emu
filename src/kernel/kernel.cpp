#include "kernel.h"
#include "kernel_internal.h"
#include "kernel_hle.h"
#include "kernel_filesystem.h"
#include <pugixml.hpp>
#include <excmd.h>
#include "debugcontrol.h"
#include "filesystem/filesystem.h"
#include "platform/platform_fiber.h"
#include "platform/platform_thread.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_interrupts.h"
#include "modules/coreinit/coreinit_internal_loader.h"
#include "modules/gx2/gx2_event.h"
#include "cpu/mem.h"
#include "ppcutils/wfunc_call.h"
#include "common/teenyheap.h"

namespace coreinit
{
struct OSContext;
struct OSThread;
}

namespace kernel
{

namespace functions
{

bool
enableTrace = false;

void
kcTraceHandler(const std::string& str)
{
   traceLogSyscall(str);
   gLog->debug(str);
}

}

static void
cpuEntrypoint();

static void
cpuInterruptHandler(uint32_t interrupt_flags);

static void
cpuSegfaultHandler(uint32_t address);

static bool
launchGame();

static bool
gRunning = true;

static std::string
gGameName;

static TeenyHeap *
sSystemHeap;

void
setGameName(const std::string& name)
{
   gGameName = name;
}

void
initialise()
{
   initialiseHleMmodules();
   cpu::setCoreEntrypointHandler(&cpuEntrypoint);
   cpu::setSegfaultHandler(&cpuSegfaultHandler);
   cpu::setInterruptHandler(&cpuInterruptHandler);

   sSystemHeap = new TeenyHeap(mem::translate(mem::SystemBase), mem::SystemSize);
}

TeenyHeap *
getSystemHeap()
{
   return sSystemHeap;
}

void
cpuSegfaultHandler(uint32_t address)
{
   // We may have been in the middle of a kernel function...
   if (coreinit::internal::isSchedulerLocked()) {
      coreinit::internal::unlockScheduler();
   }

   // Alert the debugger if it cares.
   printf("Segfault occured at %08x\n", address);

   // This will shut down the thread and reschedule.  This is required
   //  since returning from the segfault handler is an error.
   coreinit::OSExitThread(0);
}

void
cpuInterruptHandler(uint32_t interrupt_flags)
{
   if (interrupt_flags & cpu::SRESET_INTERRUPT) {
      platform::exitThread(0);
   }

   if (interrupt_flags & cpu::DBGBREAK_INTERRUPT) {
      gDebugControl.handleDbgBreakInterrupt();
   }

   if (!(interrupt_flags & ~cpu::NONMASKABLE_INTERRUPTS)) {
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

   emuassert(coreinit::internal::isSchedulerEnabled());

   coreinit::OSContext savedContext;
   kernel::saveContext(&savedContext);

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

   if (cpu::this_core::id() == 1) {
      // Run the setup on core 1, which will also run the loader
      launchGame();

      // Trip an interrupt on core 1 to force it to schedule the loader.
      cpu::interrupt(1, cpu::GENERIC_INTERRUPT);
   }

   // Run the scheduler loop, this is what will
   //   execute when there is nothing else to do.
   while (gRunning) {
      cpu::this_core::waitForInterrupt();
   }
}

bool
launchGame()
{
   // Read cos.xml if found
   auto maxCodeSize = 0x0E000000u;
   auto rpx = gGameName;
   auto fs = getFileSystem();

   if (auto fh = fs->openFile("/vol/code/cos.xml", fs::File::Read)) {
      auto size = fh->size();
      auto buffer = std::vector<uint8_t>(size);
      fh->read(buffer.data(), size, 1);
      fh->close();

      // Parse cos.xml
      pugi::xml_document doc;
      auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

      if (!parseResult) {
         gLog->error("Error parsing /vol/code/cos.xml");
         return false;
      }

      auto app = doc.child("app");
      rpx = std::string{ app.child("argstr").child_value() };
      maxCodeSize = std::stoul(app.child("max_codesize").child_value(), 0, 16);
   }
   else {
      gLog->warn("Could not open /vol/code/cos.xml, using default values");
   }

   if (auto fh = fs->openFile("/vol/code/app.xml", fs::File::Read)) {
      auto size = fh->size();
      auto buffer = std::vector<uint8_t>(size);
      fh->read(buffer.data(), size, 1);
      fh->close();

      // Parse app.xml
      pugi::xml_document doc;
      auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

      if (!parseResult) {
         gLog->error("Error parsing /vol/code/app.xml");
         return false;
      }

      // Set os_version and title_id
      auto app = doc.child("app");
      auto os_version = std::stoull(app.child("os_version").child_value(), 0, 16);
      auto title_id = std::stoull(app.child("title_id").child_value(), 0, 16);

      coreinit::internal::setSystemID(os_version);
      coreinit::internal::setTitleID(title_id);
   }
   else {
      gLog->warn("Could not open /vol/code/app.xml, using default values");
   }

   using namespace coreinit;

   // Load the application
   auto appModule = internal::loadRPX(maxCodeSize, rpx);

   if (!appModule) {
      gLog->error("Could not load {}", rpx);
      return false;
   }

   gLog->debug("Succesfully loaded {}", rpx);

   internal::startAlarmCallbackThreads();

   // Create default threads
   for (auto i = 0u; i < CoreCount; ++i) {
      auto thread = coreinit::internal::sysAlloc<OSThread>();
      auto stackSize = appModule->defaultStackSize;
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("Default Thread {}", i));

      OSCreateThread(thread, 0u, 0, nullptr,
         reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, 16,
         static_cast<OSThreadAttributes>(1 << i));
      OSSetDefaultThread(i, thread);
      OSSetThreadName(thread, name);
   }

   // Run thread 1
   auto coreinitModule = internal::findModule("coreinit");
   auto gameThreadEntry = coreinitModule->findFuncExport<uint32_t, uint32_t, void*>("GameThreadEntry");
   OSRunThread(OSGetDefaultThread(1), gameThreadEntry, 0, nullptr);

   return true;
}

} // namespace kernel
