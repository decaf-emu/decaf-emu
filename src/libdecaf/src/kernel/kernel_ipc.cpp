#include "ios/kernel/ios_kernel_ipc_thread.h"
#include "kernel_ipc.h"
#include "modules/coreinit/coreinit_ipc.h"

#include <condition_variable>
#include <libcpu/cpu.h>
#include <mutex>
#include <queue>

namespace kernel
{

static std::mutex
sIpcMutex;

static std::queue<IpcRequest *>
sIpcResponses[3];


/**
 * Submit a request to the IOS side IPC queue.
 */
void
ipcDriverKernelSubmitRequest(IpcRequest *request)
{
   switch (cpu::this_core::id()) {
   case 0:
      request->cpuId = ios::CpuId::PPC0;
      break;
   case 1:
      request->cpuId = ios::CpuId::PPC1;
      break;
   case 2:
      request->cpuId = ios::CpuId::PPC2;
      break;
   default:
      decaf_abort("Unexpected core id");
   }

   ios::kernel::submitIpcRequest(cpu::translatePhysical(request));
}


/**
 * Submit a reply to the PPC side IPC queue.
 */
void
ipcDriverKernelSubmitReply(IpcRequest *reply)
{
   auto coreId = reply->cpuId - ios::CpuId::PPC0;
   auto &responses = sIpcResponses[coreId];

   sIpcMutex.lock();
   responses.push(reply);
   sIpcMutex.unlock();

   cpu::interrupt(coreId, cpu::IPC_INTERRUPT);
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
      driver->responses[driver->numResponses] = reinterpret_cast<coreinit::IPCBuffer *>(responses.front());
      driver->numResponses++;
      responses.pop();
   }

   sIpcMutex.unlock();

   // Call userland IPCDriver callback
   coreinit::internal::ipcDriverProcessResponses();
}

} // namespace kernel
