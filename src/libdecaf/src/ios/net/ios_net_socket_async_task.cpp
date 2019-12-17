#include "ios_net_socket_async_task.h"

#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_enum.h"
#include "ios/ios_handlemanager.h"
#include "ios/ios_ipc.h"
#include "ios/ios_stackobject.h"

#include <mutex>
#include <optional>
#include <vector>

using namespace ios::kernel;
using ios::kernel::internal::setInterruptAhbAll;

namespace ios::net::internal
{

struct CompletedTask
{
   phys_ptr<ResourceRequest> resourceRequest;
   Error status;
};

struct StaticSocketAsyncTaskData
{
   be2_val<MessageQueueId> messageQueue;
   be2_array<Message, 64> messageBuffer;

   be2_val<ThreadId> thread;
   be2_array<uint8_t, 0x1000> threadStack;
};

static std::mutex sCompletedTasksMutex;
static std::vector<CompletedTask> sCompletedTasks;
static phys_ptr<StaticSocketAsyncTaskData> sSocketAsyncTaskData = nullptr;

void
completeSocketTask(phys_ptr<ResourceRequest> resourceRequest,
                   std::optional<Error> result)
{
   if (result.has_value()) {
      sCompletedTasksMutex.lock();
      sCompletedTasks.push_back({ resourceRequest, result.value() });
      sCompletedTasksMutex.unlock();
      setInterruptAhbAll(AHBALL::get(0).Wireless80211(true));
   }
}

static Error
socketAsyncTaskThread(phys_ptr<void> /*unused*/)
{
   StackObject<Message> message;
   auto error = IOS_HandleEvent(DeviceId::Wireless80211,
                                sSocketAsyncTaskData->messageQueue,
                                Message { 1 });
   if (error < Error::OK) {
      return error;
   }

   error = IOS_ClearAndEnable(DeviceId::Wireless80211);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(sSocketAsyncTaskData->messageQueue,
                                 message, MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      if (*message == 1) {
         auto completedTasks = std::vector<CompletedTask> { };

         sCompletedTasksMutex.lock();
         completedTasks.swap(sCompletedTasks);
         sCompletedTasksMutex.unlock();

         for (auto &task : completedTasks) {
            IOS_ResourceReply(task.resourceRequest,
                              static_cast<Error>(task.status));
         }

         IOS_ClearAndEnable(DeviceId::Wireless80211);
      }
   }
}

Error
startSocketAsyncTaskThread()
{
   auto error = IOS_CreateMessageQueue(phys_addrof(sSocketAsyncTaskData->messageBuffer),
                                       static_cast<uint32_t>(sSocketAsyncTaskData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }

   sSocketAsyncTaskData->messageQueue = static_cast<MessageQueueId>(error);

   error = IOS_CreateThread(socketAsyncTaskThread,
                            nullptr,
                            phys_addrof(sSocketAsyncTaskData->threadStack) + sSocketAsyncTaskData->threadStack.size(),
                            static_cast<uint32_t>(sSocketAsyncTaskData->threadStack.size()),
                            80,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sSocketAsyncTaskData->thread = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sSocketAsyncTaskData->thread, "SocketAsyncTaskThread");

   error = IOS_StartThread(sSocketAsyncTaskData->thread);
   if (error < Error::OK) {
      return error;
   }

   IOS_YieldCurrentThread();
   return Error::OK;
}

void
initialiseStaticSocketAsyncTaskData()
{
   sSocketAsyncTaskData = allocProcessStatic<StaticSocketAsyncTaskData>();
}

} // namespace ios::net
