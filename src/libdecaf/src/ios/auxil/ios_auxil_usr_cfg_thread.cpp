#include "ios_auxil_enum.h"
#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

namespace ios::auxil::internal
{

constexpr auto UsrCfgThreadNumMessages = 10u;
constexpr auto UsrCfgThreadStackSize = 0x2000u;
constexpr auto UsrCfgThreadPriority = 70u;

struct StaticData
{
   be2_val<kernel::ThreadId> threadId;
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, UsrCfgThreadNumMessages> messageBuffer;
   be2_array<uint8_t, UsrCfgThreadStackSize> threadStack;
};

static phys_ptr<StaticData>
sData = nullptr;

static Error
usrCfgThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<kernel::Message> message;

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(sData->messageQueueId,
                                              message,
                                              kernel::MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      UCDevice *device = nullptr;
      auto request = kernel::parseMessage<kernel::ResourceRequest>(message);
      auto handle = static_cast<UCDeviceHandle>(request->requestData.handle);
      error = getUCDevice(handle, &device);

      if (error >= Error::OK) {
         if (request->requestData.command != Command::Ioctlv) {
            error = Error::InvalidArg;
         } else {
            auto command = static_cast<UCCommand>(request->requestData.args.ioctlv.request);
            switch (command) {
            case UCCommand::ReadSysConfig:
               error = static_cast<Error>(device->readSysConfig(request->requestData.args.ioctlv.numVecIn,
                                                                request->requestData.args.ioctlv.vecs));
               break;
            case UCCommand::WriteSysConfig:
               error = static_cast<Error>(device->writeSysConfig(request->requestData.args.ioctlv.numVecIn,
                                                                 request->requestData.args.ioctlv.vecs));
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
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                               static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<kernel::MessageQueueId>(error);

   // Create thread
   error = kernel::IOS_CreateThread(&usrCfgThreadEntry, nullptr,
                                    phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                    static_cast<uint32_t>(sData->threadStack.size()),
                                    UsrCfgThreadPriority,
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

void
initialiseStaticUsrCfgThreadData()
{
   sData = kernel::allocProcessStatic<StaticData>();
}

} // namespace ios::auxil::internal
