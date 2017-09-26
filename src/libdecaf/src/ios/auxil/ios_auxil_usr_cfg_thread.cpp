#include "ios_auxil_enum.h"
#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

namespace ios::auxil::internal
{

constexpr auto NumMessages = 10u;
constexpr auto ThreadStackSize = 0x2000u;
constexpr auto ThreadPriority = 70u;

struct UsrCfgData
{
   be2_val<kernel::ThreadId> threadId;
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, NumMessages> messageBuffer;
   be2_array<uint8_t, ThreadStackSize> threadStack;
};

static phys_ptr<UsrCfgData>
sData = nullptr;

static Error
usrCfgThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<kernel::Message> message;

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(sData->messageQueueId,
                                              message,
                                              kernel::MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      UCDevice *device = nullptr;
      auto request = phys_ptr<kernel::ResourceRequest>(phys_addr { static_cast<kernel::Message>(*message) });
      auto handle = static_cast<UCDeviceHandle>(request->requestData.handle);
      error = getUCDevice(handle, &device);

      if (error >= Error::OK) {
         if (request->requestData.command != Command::Ioctlv) {
            error = Error::InvalidArg;
         } else {
            auto command = static_cast<UCCommand>(request->requestData.args.ioctlv.request);
            switch (command) {
            case UCCommand::ReadSysConfig:
               break;
            case UCCommand::WriteSysConfig:
               break;
            case UCCommand::DeleteSysConfig:
               break;
            default:
               error = Error::InvalidArg;
            }
         }
      }

      kernel::IOS_ResourceReply(request, error);
      device->decrementRefCount();
   }
}

Error
startUsrCfgThread()
{
   // Create message queue
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer), sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<kernel::MessageQueueId>(error);

   // Create thread
   error = kernel::IOS_CreateThread(&usrCfgThreadEntry, nullptr,
                                    phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                    sData->threadStack.size(),
                                    ThreadPriority,
                                    kernel::ThreadFlags::Detached);
   if (error < Error::OK) {
      kernel::IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }

   sData->threadId = static_cast<kernel::ThreadId>(error);
   return kernel::IOS_StartThread(sData->threadId);
}

kernel::MessageQueueId
getUsrCfgMessageQueueId()
{
   return sData->messageQueueId;
}

} // namespace ios::auxil::internal
