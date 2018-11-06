#include "ios_fs_service_thread.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/mcp/ios_mcp_ipc.h"

#include "ios/ios_enum.h"
#include "ios/ios_handlemanager.h"
#include "ios/ios_ipc.h"
#include "ios/ios_stackobject.h"

using namespace ios::kernel;
using namespace ios::mcp;

namespace ios::fs::internal
{

constexpr auto ServiceThreadStackSize = 0x4000u;
constexpr auto ServiceThreadPriority = 85u;

struct StaticServiceThreadData
{
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, ServiceThreadStackSize> threadStack;

   be2_val<MessageQueueId> messageQueue;
   be2_array<Message, 0x160> messageBuffer;
};

static phys_ptr<StaticServiceThreadData>
sData = nullptr;

struct ServiceDevice
{
   std::string_view name;
   bool open = false;
   bool resumed = false;
};

static std::array<ServiceDevice, 16>
sServiceDevices =
{
   ServiceDevice { "/dev/df" },
   ServiceDevice { "/dev/atfs" },
   ServiceDevice { "/dev/isfs" },
   ServiceDevice { "/dev/wfs" },
   ServiceDevice { "/dev/pcfs" },
   ServiceDevice { "/dev/rbfs" },
   ServiceDevice { "/dev/fat" },
   ServiceDevice { "/dev/fla" },
   ServiceDevice { "/dev/ums" },
   ServiceDevice { "/dev/ahcimgr" },
   ServiceDevice { "/dev/shdd" },
   ServiceDevice { "/dev/md" },
   ServiceDevice { "/dev/scfm" },
   ServiceDevice { "/dev/mmc" },
   ServiceDevice { "/dev/timetrace" },
   ServiceDevice { "/dev/tcp_pcfs" },
};

static Error
serviceThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }

   auto queueId = static_cast<MessageQueueId>(error);
   sData->messageQueue = queueId;

   for (auto &device : sServiceDevices) {
      error = MCP_RegisterResourceManager(device.name, sData->messageQueue);
      if (error >= Error::OK) {
         IOS_AssociateResourceManager(device.name, ResourcePermissionGroup::FS);
      }
   }

   while (true) {
      auto error = IOS_ReceiveMessage(sData->messageQueue,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         error = Error::Invalid;

         for (auto i = 0u; i < sServiceDevices.size(); ++i) {
            auto &device = sServiceDevices[i];
            if (device.name.compare(request->requestData.args.open.name.get()) == 0) {
               device.open = true;
               error = static_cast<Error>(i);
               break;
            }
         }

         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Close:
      {
         auto deviceIdx = request->requestData.handle;
         if (deviceIdx < 0 || deviceIdx >= sServiceDevices.size()) {
            error = Error::Invalid;
         } else {
            sServiceDevices[deviceIdx].open = false;
         }

         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Resume:
      {
         auto deviceIdx = request->requestData.handle;
         if (deviceIdx < 0 || deviceIdx >= sServiceDevices.size()) {
            error = Error::Invalid;
         } else {
            sServiceDevices[deviceIdx].resumed = true;
         }

         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Suspend:
      {
         auto deviceIdx = request->requestData.handle;
         if (deviceIdx < 0 || deviceIdx >= sServiceDevices.size()) {
            error = Error::Invalid;
         } else {
            sServiceDevices[deviceIdx].resumed = false;
         }

         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

Error
startServiceThread()
{
   auto error = IOS_CreateThread(&serviceThreadEntry,
                                 nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 static_cast<uint32_t>(sData->threadStack.size()),
                                 ServiceThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }
   sData->threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sData->threadId, "FsServiceThread");

   error = IOS_StartThread(sData->threadId);
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

void
initialiseStaticServiceThreadData()
{
   sData = allocProcessStatic<StaticServiceThreadData>();
   for (auto &device : sServiceDevices) {
      device.open = false;
      device.resumed = false;
   }
}

} // namespace ios::fs::internal
