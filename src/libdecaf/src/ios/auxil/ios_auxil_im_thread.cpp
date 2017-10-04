#include "ios_auxil_im_device.h"
#include "ios_auxil_im_thread.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_timer.h"

#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"

#include <memory>

namespace ios::auxil::internal
{

using namespace kernel;

using IMDeviceHandle = uint32_t;

constexpr auto MaxNumIMDevices = 96u;
constexpr auto ImNumMessages = 20u;
constexpr auto ImThreadStackSize = 0x800u;
constexpr auto ImThreadPriority = 69u;

struct StaticImThreadData
{
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, ImNumMessages> messageBuffer;
   be2_array<uint8_t, ImThreadStackSize> threadStack;
   be2_val<Command> stopMessageBuffer;
};

static phys_ptr<StaticImThreadData>
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
   StackObject<Message> message;
   auto error = Error::OK;

   initialiseImParameters();

   while (true) {
      error = IOS_ReceiveMessage(sData->messageQueueId,
                                 message,
                                 MessageFlags::None);
      if (error < Error::OK) {
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      if (request->requestData.command == Command::IpcMsg2) {
         // Shutdown message from stopImThread
         error = Error::OK;
         break;
      }

      switch (request->requestData.command) {
      case Command::Open:
      {
         error = sDevices.open();
         IOS_ResourceReply(request, error);
         break;
      }

      case Command::Close:
      {
         error = sDevices.close(request->requestData.handle);
         IOS_ResourceReply(request, error);
         break;
      }

      case Command::Ioctlv:
      {
         auto handle = static_cast<IMDeviceHandle>(request->requestData.handle);
         auto command = static_cast<IMCommand>(request->requestData.args.ioctlv.request);
         error = imDeviceIoctlv(handle,
                                command,
                                request->requestData.args.ioctlv.vecs);
         IOS_ResourceReply(request, error);
         break;
      }

      case Command::IpcMsg1:
      {
         // Timer message
         break;
      }

      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   sDevices.closeAll();
   // TODO: Stop timer
   // TODO: Close UsrCfg handle
   return error;
}

Error
startImThread()
{
   // Create message queue
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<MessageQueueId>(error);

   // Register /dev/im
   error = IOS_RegisterResourceManager("/dev/im", sData->messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   // Create thread
   error = IOS_CreateThread(&imThreadEntry, nullptr,
                            phys_addrof(sData->threadStack) + sData->threadStack.size(),
                            static_cast<uint32_t>(sData->threadStack.size()),
                            ImThreadPriority,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }

   sData->threadId = static_cast<ThreadId>(error);
   return IOS_StartThread(sData->threadId);
}

Error
stopImThread()
{
   sData->stopMessageBuffer = Command::IpcMsg2;

   auto message = makeMessage(phys_addrof(sData->stopMessageBuffer));
   auto error = IOS_SendMessage(sData->messageQueueId, message, MessageFlags::None);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_JoinThread(sData->threadId, nullptr);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_DestroyMessageQueue(sData->messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

void
initialiseStaticImThreadData()
{
   sData = phys_cast<StaticImThreadData>(allocProcessStatic(sizeof(StaticImThreadData)));
   sDevices.closeAll();
}

} // namespace ios::auxil::internal
