#include "ios.h"
#include "ios_alarm_thread.h"
#include "ios_core.h"
#include "ios_worker_thread.h"
#include "kernel/ios_kernel.h"
#include "kernel/ios_kernel_hardware.h"
#include "kernel/ios_kernel_scheduler.h"

#include <array>
#include <common/decaf_assert.h>
#include <common/platform_thread.h>
#include <condition_variable>
#include <fmt/format.h>
#include <mutex>
#include <thread>
#include <queue>

namespace ios
{

constexpr auto NumIosCores = 1u;

static std::array<Core, NumIosCores>
sCores;

static thread_local Core *
sCurrentCore = nullptr;

static std::atomic<bool>
sRunning = false;

static std::condition_variable
sInterruptCV;

static std::mutex
sInterruptMutex;

static std::atomic<uint32_t>
sInterruptFlags = 0u;

static std::unique_ptr<::fs::FileSystem>
sFileSystem;

namespace internal
{

Core *
getCurrentCore()
{
   return sCurrentCore;
}

uint32_t
getCoreCount()
{
   return NumIosCores;
}

CoreId
getCurrentCoreId()
{
   return sCurrentCore->id;
}

void
interruptCore(CoreId core,
              CoreInterruptFlags flags)
{
   sCores[core].interruptFlags.fetch_or(flags);
   sInterruptCV.notify_all();
}

void
interruptAllCores(CoreInterruptFlags flags)
{
   for (auto i = 0u; i < sCores.size(); ++i) {
      sCores[i].interruptFlags.fetch_or(flags);
   }

   sInterruptCV.notify_all();
}

void
interrupt(InterruptFlags flags)
{
   sInterruptFlags.fetch_or(flags);
   sInterruptCV.notify_one();
}

} // namespace internal

static void
iosCoreEntry(Core *core)
{
   sCurrentCore = core;
   core->fiber = platform::getThreadFiber();

   while (sRunning.load()) {
      std::unique_lock<std::mutex> lock { sInterruptMutex };
      auto coreInterruptFlags = 0u;
      auto interruptFlags = 0u;

      do {
         coreInterruptFlags = core->interruptFlags.fetch_and(0);
         if (coreInterruptFlags) {
            break;
         }

         interruptFlags = sInterruptFlags.fetch_and(0);
         if (interruptFlags) {
            break;
         }

         sInterruptCV.wait(lock);
      } while (true);

      lock.unlock();

      if (coreInterruptFlags & CoreInterruptFlags::Shutdown) {
         break;
      }

      if (interruptFlags & InterruptFlags::Ahb) {
         kernel::internal::handleAhbInterrupts();
      }

      if (coreInterruptFlags & CoreInterruptFlags::Scheduler) {
         kernel::internal::handleSchedulerInterrupt();
      }
   }
}

void
start()
{
   sRunning.store(true);
   internal::startAlarmThread();
   internal::startWorkerThread();

   for (auto i = 0u; i < sCores.size(); ++i) {
      sCores[i].id = i;
      sCores[i].thread = std::thread { iosCoreEntry, &sCores[i] };
      platform::setThreadName(&sCores[i].thread, fmt::format("IOS Core {}", i));
   }

   kernel::start();
   internal::interruptAllCores(CoreInterruptFlags::Scheduler);
}

void
join()
{
   sRunning.store(false);
   internal::joinAlarmThread();
   internal::joinWorkerThread();
   internal::interruptAllCores(CoreInterruptFlags::Shutdown);

   for (auto i = 0u; i < sCores.size(); ++i) {
      if (sCores[i].thread.joinable()) {
         sCores[i].thread.join();
      }
   }
}

void
setFileSystem(std::unique_ptr<::fs::FileSystem> fs)
{
   sFileSystem = std::move(fs);
}

::fs::FileSystem *
getFileSystem()
{
   return sFileSystem.get();
}

} // namespace ios
