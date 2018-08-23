#include "ios_fs_fsa_device.h"
#include "ios_fs_mutex.h"

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
#include <vector>

using namespace ios::kernel;
using ios::kernel::internal::setInterruptAhbAll;

namespace ios::fs::internal
{

struct CompletedTask
{
   phys_ptr<ResourceRequest> resourceRequest;
   FSAStatus status;
};

struct StaticFsaAsyncData
{
   be2_val<MessageQueueId> messageQueue;
   be2_array<Message, 64> messageBuffer;

   be2_val<ThreadId> thread;
   be2_array<uint8_t, 0x1000> threadStack;
};

static std::mutex sCompletedTasksMutex;
static std::vector<CompletedTask> sCompletedTasks;
static phys_ptr<StaticFsaAsyncData> sFsaAsyncData = nullptr;

static void
fsaTaskCompleteHandler()
{
   auto completedTasks = std::vector<CompletedTask> { };

   sCompletedTasksMutex.lock();
   completedTasks.swap(sCompletedTasks);
   IOS_ClearAndEnable(DeviceId::Sata);
   sCompletedTasksMutex.unlock();

   for (auto &task : completedTasks) {
      IOS_ResourceReply(task.resourceRequest,
                        static_cast<Error>(task.status));
   }
}

void
fsaAsyncTaskComplete(phys_ptr<ResourceRequest> resourceRequest,
                     FSAStatus result)
{
   sCompletedTasksMutex.lock();
   sCompletedTasks.push_back({ resourceRequest, result });
   sCompletedTasksMutex.unlock();

   setInterruptAhbAll(AHBALL::get(0).Sata(true));
}

static Error
fsaAsyncTaskThread(phys_ptr<void> /*unused*/)
{
   StackObject<Message> message;

   auto error = IOS_HandleEvent(DeviceId::Sata,
                                sFsaAsyncData->messageQueue,
                                Message { 1 });
   if (error < Error::OK) {
      return error;
   }

   error = IOS_ClearAndEnable(DeviceId::Sata);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      auto error = IOS_ReceiveMessage(sFsaAsyncData->messageQueue,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      switch (*message) {
      case 1:
         fsaTaskCompleteHandler();
         break;
      }
   }
}

Error
startFsaAsyncTaskThread()
{
   auto error = IOS_CreateMessageQueue(phys_addrof(sFsaAsyncData->messageBuffer),
                                       static_cast<uint32_t>(sFsaAsyncData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }

   sFsaAsyncData->messageQueue = static_cast<MessageQueueId>(error);


   error = IOS_CreateThread(fsaAsyncTaskThread,
                            nullptr,
                            phys_addrof(sFsaAsyncData->threadStack) + sFsaAsyncData->threadStack.size(),
                            static_cast<uint32_t>(sFsaAsyncData->threadStack.size()),
                            79,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sFsaAsyncData->thread = static_cast<ThreadId>(error);

   error = IOS_StartThread(sFsaAsyncData->thread);
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

void
initialiseStaticFsaAsyncTaskData()
{
   sFsaAsyncData = phys_cast<StaticFsaAsyncData *>(
      allocProcessStatic(sizeof(StaticFsaAsyncData)));
}

} // namespace ios::fs
