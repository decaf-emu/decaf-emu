#include "ios_kernel_hardware.h"
#include "ios_kernel_ipc_thread.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_process.h"
#include "ios_kernel_resourcemanager.h"
#include "ios/ios_stackobject.h"
#include "kernel/kernel_ipc.h"

#include <common/atomicqueue.h>
#include <common/log.h>
#include <mutex>
#include <queue>

namespace ios::kernel
{

constexpr auto IpcThreadNumMessages = 0x100u;
constexpr auto IpcThreadStackSize = 0x800u;
constexpr auto IpcThreadPriority = 95u;

struct StaticData
{
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, IpcThreadNumMessages> messageBuffer;
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, IpcThreadStackSize> threadStack;
};

static phys_ptr<StaticData>
sData;

static MessageQueueId
sIpcMessageQueueId;

static AtomicQueue<phys_ptr<IpcRequest>, 128>
sIpcRequestQueue;

void
submitIpcRequest(phys_ptr<IpcRequest> request)
{
   auto x = sizeof(sIpcRequestQueue);
   decaf_check(!sIpcRequestQueue.wasFull());
   sIpcRequestQueue.push(request);

   switch (request->cpuId) {
   case CpuId::PPC0:
      internal::setInterruptAhbLt(AHBLT::get(0).IpcStarbuckCore0(true));
      break;
   case CpuId::PPC1:
      internal::setInterruptAhbLt(AHBLT::get(0).IpcStarbuckCore1(true));
      break;
   case CpuId::PPC2:
      internal::setInterruptAhbLt(AHBLT::get(0).IpcStarbuckCore2(true));
      break;
   }
}

namespace internal
{

static Error
ipcThreadEntry(phys_ptr<void> context)
{
   StackObject<Message> message;
   phys_ptr<MessageQueue> queue;

   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }

   // Register interrupt handlers and enable the interrupts.
   auto queueId = static_cast<MessageQueueId>(error);
   error = internal::getMessageQueue(queueId, &queue);
   sData->messageQueueId = queueId;

   IOS_HandleEvent(DeviceId::IpcStarbuckCore0, queueId, Message { Command::IpcMsg0 });
   IOS_ClearAndEnable(DeviceId::IpcStarbuckCore0);

   IOS_HandleEvent(DeviceId::IpcStarbuckCore1, queueId, Message { Command::IpcMsg1 });
   IOS_ClearAndEnable(DeviceId::IpcStarbuckCore1);

   IOS_HandleEvent(DeviceId::IpcStarbuckCore2, queueId, Message { Command::IpcMsg2 });
   IOS_ClearAndEnable(DeviceId::IpcStarbuckCore2);

   IOS_HandleEvent(DeviceId::IpcStarbuckCompat, queueId, Message { Command::IpcMsg1 });
   IOS_ClearAndEnable(DeviceId::IpcStarbuckCompat);

   while (true) {
      auto error = IOS_ReceiveMessage(queueId, message, MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      /*
       * We're kind of cheating here, we have combined all 3 IPC queues into a
       * single queue in order to make it easier to reduce chances of race
       * conditions. Also we attempt to fully empty the ipc request queue
       * rather than processing a single message per interrupt like on hardware.
       */
      while (!sIpcRequestQueue.wasEmpty()) {
         auto request = sIpcRequestQueue.pop();

         if (request->clientPid > 7) {
            gLog->error("Received IPC request with invalid clientPid of {}", request->clientPid);
            error = Error::Invalid;
         } else {
            auto pid = static_cast<ProcessId>(request->clientPid + ProcessId::COSKERNEL);
            auto error = Error::OK;

            switch (request->command) {
            case Command::Open:
               error = internal::dispatchIosOpen(request->args.open.name.getRawPointer(),
                                                 request->args.open.mode,
                                                 queue,
                                                 request,
                                                 pid,
                                                 request->cpuId);
               break;
            case Command::Close:
               error = internal::dispatchIosClose(request->handle,
                                                  queue,
                                                  request,
                                                  request->args.close.unkArg0,
                                                  pid,
                                                  request->cpuId);
               break;
            case Command::Read:
               error = internal::dispatchIosRead(request->handle,
                                                 request->args.read.data,
                                                 request->args.read.length,
                                                 queue,
                                                 request,
                                                 pid,
                                                 request->cpuId);
               break;
            case Command::Write:
               error = internal::dispatchIosWrite(request->handle,
                                                  request->args.write.data,
                                                  request->args.write.length,
                                                  queue,
                                                  request,
                                                  pid,
                                                  request->cpuId);
               break;
            case Command::Seek:
               error = internal::dispatchIosSeek(request->handle,
                                                 request->args.seek.offset,
                                                 request->args.seek.origin,
                                                 queue,
                                                 request,
                                                 pid,
                                                 request->cpuId);
               break;
            case Command::Ioctl:
               error = internal::dispatchIosIoctl(request->handle,
                                                  request->args.ioctl.request,
                                                  request->args.ioctl.inputBuffer,
                                                  request->args.ioctl.inputLength,
                                                  request->args.ioctl.outputBuffer,
                                                  request->args.ioctl.outputLength,
                                                  queue,
                                                  request,
                                                  pid,
                                                  request->cpuId);
               break;
            case Command::Ioctlv:
               error = internal::dispatchIosIoctlv(request->handle,
                                                   request->args.ioctlv.request,
                                                   request->args.ioctlv.numVecIn,
                                                   request->args.ioctlv.numVecOut,
                                                   request->args.ioctlv.vecs,
                                                   queue,
                                                   request,
                                                   pid,
                                                   request->cpuId);
               break;
            default:
               error = Error::Invalid;
            }
         }

         if (error < Error::OK) {
            // Reply with error!
            request->command = Command::Reply;
            request->reply = error;
            ::kernel::ipcDriverKernelSubmitReply(request.getRawPointer());
            continue;
         }
      }

      IOS_ClearAndEnable(DeviceId::IpcStarbuckCore0);
      IOS_ClearAndEnable(DeviceId::IpcStarbuckCore1);
      IOS_ClearAndEnable(DeviceId::IpcStarbuckCore2);
   }
}

MessageQueueId
getIpcMessageQueueId()
{
   return sIpcMessageQueueId;
}

Error
startIpcThread()
{
   sData = allocProcessStatic<StaticData>();

   // Create thread
   auto error = IOS_CreateThread(&ipcThreadEntry, nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 static_cast<uint32_t>(sData->threadStack.size()),
                                 IpcThreadPriority,
                                 kernel::ThreadFlags::Detached);
   if (error < Error::OK) {
      kernel::IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }

   sData->threadId = static_cast<kernel::ThreadId>(error);
   return kernel::IOS_StartThread(sData->threadId);
}

} // namespace internal

} // namespace ios::kernel
