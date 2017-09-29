#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"

namespace ios::auxil::internal
{

constexpr auto UCServiceNumMessages = 10u;
constexpr auto UCServiceThreadStackSize = 0x2000u;
constexpr auto UCServiceThreadPriority = 70u;
constexpr auto MaxNumDevices = 96u;

struct StaticData
{
   be2_val<bool> serviceRunning;
   be2_val<kernel::ThreadId> threadId;
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, UCServiceNumMessages> messageBuffer;
   be2_array<uint8_t, UCServiceThreadStackSize> threadStack;
};

static phys_ptr<StaticData>
sData = nullptr;

static HandleManager<UCDevice, UCDeviceHandle, MaxNumDevices>
sDevices;

void
destroyUCDevice(UCDevice *device)
{
   sDevices.close(device);
}

Error
getUCDevice(UCDeviceHandle handle,
            UCDevice **outDevice)
{
   return sDevices.get(handle, outDevice);
}

static Error
usrCfgServiceThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<kernel::Message> message;

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(sData->messageQueueId,
                                              message,
                                              kernel::MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      auto request = phys_ptr<kernel::ResourceRequest>(phys_addr { static_cast<kernel::Message>(*message) });
      switch (request->requestData.command) {
      case Command::Open:
      {
         error = sDevices.open();
         kernel::IOS_ResourceReply(request, error);
         break;
      }

      case Command::Close:
      {
         UCDevice *device = nullptr;
         auto handle = static_cast<UCDeviceHandle>(request->requestData.handle);
         error = sDevices.get(handle, &device);
         if (error < Error::OK) {
            kernel::IOS_ResourceReply(request, error);
            continue;
         }

         device->setCloseRequest(request);
         device->decrementRefCount();
         break;
      }

      case Command::Ioctlv:
      {
         UCDevice *device = nullptr;

         if (!sData->serviceRunning) {
            kernel::IOS_ResourceReply(request, Error::NotReady);
            continue;
         }

         auto handle = static_cast<UCDeviceHandle>(request->requestData.handle);
         error = sDevices.get(handle, &device);
         if (error < Error::OK) {
            kernel::IOS_ResourceReply(request, error);
            continue;
         }

         device->incrementRefCount();
         kernel::IOS_SendMessage(getUsrCfgMessageQueueId(),
                                 *message,
                                 kernel::MessageFlags::None);
         break;
      }

      case Command::Suspend:
      {
         // TODO: Close FSA Handle
         sData->serviceRunning = false;
         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Resume:
      {
         // TODO: Open FSA Handle
         sData->serviceRunning = true;
         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      }

      default:
         kernel::IOS_ResourceReply(request, Error::InvalidArg);
      }
   }
}

Error
startUsrCfgServiceThread()
{
   // Create message queue
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                               static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<kernel::MessageQueueId>(error);

   // Register the device
   error = kernel::IOS_RegisterResourceManager("/dev/usr_cfg",
                                               sData->messageQueueId);
   if (error < Error::OK) {
      kernel::IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }

   // Create thread
   error = kernel::IOS_CreateThread(&usrCfgServiceThreadEntry, nullptr,
                                    phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                    static_cast<uint32_t>(sData->threadStack.size()),
                                    UCServiceThreadPriority,
                                    kernel::ThreadFlags::Detached);
   if (error < Error::OK) {
      kernel::IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }
   sData->threadId = static_cast<kernel::ThreadId>(error);
   return kernel::IOS_StartThread(sData->threadId);
}

kernel::MessageQueueId
getUsrCfgServiceMessageQueueId()
{
   return sData->messageQueueId;
}

void
initialiseStaticUsrCfgServiceThreadData()
{
   sData = kernel::allocProcessStatic<StaticData>();
   sDevices.closeAll();
}

} // namespace ios::auxil::internal
