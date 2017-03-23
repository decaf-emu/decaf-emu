#include "kernel_ios.h"
#include "kernel_ipc.h"
#include "modules/coreinit/coreinit_ipc.h"

#include <condition_variable>
#include <libcpu/cpu.h>
#include <mutex>
#include <queue>

namespace kernel
{

static std::thread
sIpcThread;

static std::atomic_bool
sIpcThreadRunning;

static std::mutex
sIpcMutex;

static std::condition_variable
sIpcCond;

static std::queue<IPCBuffer *>
sIpcRequests;

static std::queue<IPCBuffer *>
sIpcResponses[3];

static void
ipcThreadEntry();


/**
 * Start the IPC thread.
 */
void
ipcStart()
{
   std::unique_lock<std::mutex> lock { sIpcMutex };
   sIpcThreadRunning.store(true);
   sIpcThread = std::thread { ipcThreadEntry };
}


/**
 * Stop the IPC thread.
 */
void
ipcShutdown()
{
   std::unique_lock<std::mutex> lock { sIpcMutex };

   if (sIpcThreadRunning.exchange(false)) {
      sIpcCond.notify_all();
      lock.unlock();

      sIpcThread.join();
   }
}


/**
 * Submit an buffer to the IPC queue.
 */
void
ipcDriverKernelSubmitRequest(IPCBuffer *buffer)
{
   switch (cpu::this_core::id()) {
   case 0:
      buffer->cpuId = IOSCpuId::PPC0;
      break;
   case 1:
      buffer->cpuId = IOSCpuId::PPC1;
      break;
   case 2:
      buffer->cpuId = IOSCpuId::PPC2;
      break;
   default:
      decaf_abort("Unexpected core id");
   }

   sIpcMutex.lock();
   sIpcRequests.push(buffer);
   sIpcCond.notify_all();
   sIpcMutex.unlock();
}


/**
 * Handle PPC interrupt for when we have IPC responses to dispatch.
 */
void
ipcDriverKernelHandleInterrupt()
{
   auto driver = coreinit::internal::getIPCDriver();
   auto &responses = sIpcResponses[driver->coreId];

   // Copy respones to IPCDriver structure
   sIpcMutex.lock();

   while (responses.size()) {
      driver->responses[driver->numResponses] = responses.front();
      driver->numResponses++;
      responses.pop();
   }

   sIpcMutex.unlock();

   // Call userland IPCDriver callback
   coreinit::internal::ipcDriverProcessResponses();
}


/**
 * Main thread entry point for the IPC thread.
 *
 * This thread represents the IOS side of the IPC mechanism.
 *
 * Responsible for receiving IPC requests and dispatching them to the
 * correct IOS device.
 */
void
ipcThreadEntry()
{
   std::unique_lock<std::mutex> lock { sIpcMutex };

   while (true) {
      if (!sIpcRequests.empty()) {
         auto request = sIpcRequests.front();
         sIpcRequests.pop();
         lock.unlock();
         iosDispatchIpcRequest(request);
         lock.lock();

         switch (request->cpuId) {
         case IOSCpuId::PPC0:
            sIpcResponses[0].push(request);
            cpu::interrupt(0, cpu::IPC_INTERRUPT);
            break;
         case IOSCpuId::PPC1:
            sIpcResponses[1].push(request);
            cpu::interrupt(1, cpu::IPC_INTERRUPT);
            break;
         case IOSCpuId::PPC2:
            sIpcResponses[2].push(request);
            cpu::interrupt(2, cpu::IPC_INTERRUPT);
            break;
         default:
            decaf_abort("Unexpected cpu id");
         }
      }

      if (!sIpcThreadRunning.load()) {
         break;
      }

      if (sIpcRequests.empty()) {
         sIpcCond.wait(lock);
      }
   }

   sIpcThreadRunning.store(false);
}

} // namespace kernel
