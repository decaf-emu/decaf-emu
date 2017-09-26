#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"

namespace ios::auxil::internal
{

constexpr auto NumMessages = 10u;
constexpr auto ThreadStackSize = 0x2000u;
constexpr auto ThreadPriority = 70u;
constexpr auto MaxNumDevices = 96u;

struct UsrCfgDevice
{
   be2_val<BOOL> allocated;
   be2_val<int32_t> refCount;
   be2_phys_ptr<kernel::ResourceRequest> closeRequest;
};

struct UsrCfgServiceData
{
   be2_val<bool> serviceRunning;
   be2_val<kernel::ThreadId> threadId;
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, NumMessages> messageBuffer;
   be2_array<uint8_t, ThreadStackSize> threadStack;
};

static phys_ptr<UsrCfgServiceData>
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
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer), sData->messageBuffer.size());
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
getUsrCfgServiceMessageQueueId()
{
   return sData->messageQueueId;
}

} // namespace ios::auxil::internal
