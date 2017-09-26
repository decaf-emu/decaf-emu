#include "ios_auxil_im_device.h"
#include "ios_auxil_im_thread.h"

#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"

#include <memory>

namespace ios::auxil::internal
{

using IMDeviceHandle = uint32_t;

constexpr auto MaxNumIMDevices = 96u;
constexpr auto NumMessages = 20u;
constexpr auto ThreadStackSize = 0x800u;
constexpr auto ThreadPriority = 69u;

struct ImThreadData
{
   be2_val<kernel::ThreadId> threadId;
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, NumMessages> messageBuffer;
   be2_array<uint8_t, ThreadStackSize> threadStack;
};

static phys_ptr<ImThreadData>
sData;

static HandleManager<IMDevice, IMDeviceHandle, MaxNumIMDevices>
sDevices;

static Error
imDeviceIoctlv(IMDeviceHandle handle,
               IMCommand command,
               phys_ptr<IoctlVec> vecs)
{
   IMDevice *device = nullptr;
   auto error = sDevices.get(handle, &device);
   if (error < Error::OK) {
      return error;
   }

   auto request = phys_ptr<IMRequest> { vecs[0].paddr };
   auto response = phys_ptr<IMResponse> { vecs[1].paddr };

   switch (command) {
   case IMCommand::CopyParameterFromNv:
      error = device->copyParameterFromNv();
      break;
   case IMCommand::SetNvParameter:
      error = device->setNvParameter(request->setNvParameter.parameter,
                                     phys_addrof(request->setNvParameter.value));
      break;
   case IMCommand::SetParameter:
      error = device->setParameter(request->setParameter.parameter,
                                   phys_addrof(request->setParameter.value));
      break;
   case IMCommand::GetParameter:
      error = device->getParameter(request->getParameter.parameter,
                                   phys_addrof(response->getParameter.value));
      break;
   case IMCommand::GetHomeButtonParams:
      error = device->getHomeButtonParams(phys_addrof(response->getHomeButtonParam));
      break;
   case IMCommand::GetTimerRemaining:
      error = device->getTimerRemaining(request->getTimerRemaining.timer,
                                        phys_addrof(response->getTimerRemaining.value));
      break;
   case IMCommand::GetNvParameter:
      error = device->getNvParameter(request->getNvParameter.parameter,
                                     phys_addrof(response->getNvParameter.value));
      break;
   default:
      error = Error::InvalidArg;
   }

   return error;
}

static Error
imThreadEntry(phys_ptr<void> /*context*/)
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
         error = sDevices.close(request->requestData.handle);
         kernel::IOS_ResourceReply(request, error);
         break;
      }

      case Command::Ioctlv:
      {
         auto handle = static_cast<IMDeviceHandle>(request->requestData.handle);
         auto command = static_cast<IMCommand>(request->requestData.args.ioctlv.request);
         error = imDeviceIoctlv(handle,
                                command,
                                request->requestData.args.ioctlv.vecs);
         kernel::IOS_ResourceReply(request, error);
         break;
      }

      default:
         kernel::IOS_ResourceReply(request, Error::InvalidArg);
      }
   }
}

Error
startImThread()
{
   // Create message queue
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer), sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<kernel::MessageQueueId>(error);

   // Create thread
   error = kernel::IOS_CreateThread(&imThreadEntry, nullptr,
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

} // namespace ios::auxil::internal
