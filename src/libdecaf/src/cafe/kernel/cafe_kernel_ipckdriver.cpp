#include "cafe_kernel_ipckdriver.h"
#include "ios/kernel/ios_kernel_ipc_thread.h"
#include "cafe/libraries/coreinit/coreinit_ipcdriver.h"

#include <condition_variable>
#include <libcpu/cpu.h>
#include <mutex>
#include <queue>

namespace cafe::kernel
{

static std::mutex
sIpcMutex;

static std::queue<virt_ptr<IPCKDriverRequest>>
sIpcResponses[3];

static std::vector<std::pair<virt_ptr<IPCKDriverRequest>, phys_ptr<ios::IpcRequest>>>
sPendingRequests;

/**
 * Submit a request to the IOS side IPC queue.
 */
ios::Error
ipcDriverKernelSubmitRequest(virt_ptr<IPCKDriverRequest> request)
{
   phys_addr paddr;

   // Translate all virtual addresses to physical before performing IPC request
   switch (request->request.command) {
   case ios::Command::Open:
      if (!cpu::virtualToPhysicalAddress(virt_cast<virt_addr>(request->buffer1), paddr)) {
         return ios::Error::InvalidArg;
      } else {
         request->request.args.open.name = phys_cast<const char *>(paddr);
      }
      break;
   case ios::Command::Ioctl:
      if (request->buffer1 && !cpu::virtualToPhysicalAddress(virt_cast<virt_addr>(request->buffer1), paddr)) {
         return ios::Error::InvalidArg;
      } else {
         request->request.args.ioctl.inputBuffer = phys_cast<const void *>(paddr);
      }

      if (request->buffer2 && !cpu::virtualToPhysicalAddress(virt_cast<virt_addr>(request->buffer2), paddr)) {
         return ios::Error::InvalidArg;
      } else {
         request->request.args.ioctl.outputBuffer = phys_cast<void *>(paddr);
      }
      break;
   case ios::Command::Ioctlv:
   {
      auto &ioctlv = request->request.args.ioctlv;
      if (request->buffer1 && !cpu::virtualToPhysicalAddress(virt_cast<virt_addr>(request->buffer1), paddr)) {
         return ios::Error::InvalidArg;
      } else {
         ioctlv.vecs = phys_cast<ios::IoctlVec *>(paddr);
      }

      for (auto i = 0u; i < ioctlv.numVecIn + ioctlv.numVecOut; ++i) {
         if (!ioctlv.vecs[i].vaddr) {
            continue;
         }

         if (!cpu::virtualToPhysicalAddress(ioctlv.vecs[i].vaddr, paddr)) {
            return ios::Error::InvalidArg;
         }

         ioctlv.vecs[i].paddr = paddr;
      }
      break;
   }
   default:
      break;
   }

   switch (cpu::this_core::id()) {
   case 0:
      request->request.cpuId = ios::CpuId::PPC0;
      break;
   case 1:
      request->request.cpuId = ios::CpuId::PPC1;
      break;
   case 2:
      request->request.cpuId = ios::CpuId::PPC2;
      break;
   default:
      decaf_abort("Unexpected core id");
   }

   auto ipcRequest = virt_addrof(request->request);
   if (!cpu::virtualToPhysicalAddress(virt_cast<virt_addr>(ipcRequest), paddr)) {
      return ios::Error::InvalidArg;
   }

   ios::kernel::submitIpcRequest(phys_cast<ios::IpcRequest *>(paddr));

   sIpcMutex.lock();
   sPendingRequests.push_back({ request, phys_cast<ios::IpcRequest *>(paddr) });
   sIpcMutex.unlock();
   return ios::Error::OK;
}


/**
 * Submit a reply to the PPC side IPC queue.
 */
void
ipcDriverKernelSubmitReply(phys_ptr<ios::IpcRequest> reply)
{
   auto coreId = reply->cpuId - ios::CpuId::PPC0;
   auto &responses = sIpcResponses[coreId];

   // Find the matching pending request
   auto request = virt_ptr<IPCKDriverRequest> { nullptr };

   sIpcMutex.lock();
   for (auto itr = sPendingRequests.begin(); itr != sPendingRequests.end(); ++itr) {
      if (itr->second == reply) {
         request = itr->first;
         sPendingRequests.erase(itr);
         break;
      }
   }

   responses.push(request);
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
      driver->responses[driver->numResponses] = responses.front();
      driver->numResponses++;
      responses.pop();
   }

   sIpcMutex.unlock();

   // Call userland IPCDriver callback
   coreinit::internal::ipcDriverProcessResponses();
}

} // namespace cafe::kernel
