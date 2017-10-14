#include "ios_net_socket_device.h"
#include "ios_net_socket_thread.h"
#include "ios_net_socket_request.h"
#include "ios_net_socket_response.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

#include <array>

namespace ios::net::internal
{

using SocketDeviceHandle = int32_t;
using namespace kernel;

constexpr auto NumSocketMessages = 40u;
constexpr auto SocketThreadStackSize = 0x4000u;
constexpr auto SocketThreadPriority = 69u;

struct StaticSocketThreadData
{
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_struct<IpcRequest> stopMessage;
   be2_array<Message, NumSocketMessages> messageBuffer;
   be2_array<uint8_t, SocketThreadStackSize> threadStack;
};

static phys_ptr<StaticSocketThreadData>
sData = nullptr;

static std::array<std::unique_ptr<SocketDevice>, ProcessId::Max>
sDevices;

static SocketDevice *
getDevice(SocketDeviceHandle handle)
{
   if (handle < 0 || handle >= sDevices.size()) {
      return nullptr;
   }

   return sDevices[handle].get();
}

static Error
socketOpen(phys_ptr<ResourceRequest> request)
{
   // There is one socket device per process
   auto pid = request->requestData.clientPid;
   auto idx = static_cast<size_t>(pid);

   if (!sDevices[idx]) {
      sDevices[idx] = std::make_unique<SocketDevice>();
   }

   return static_cast<Error>(idx);
}

static Error
socketClose(phys_ptr<ResourceRequest> request)
{
   auto pid = request->requestData.clientPid;
   auto idx = static_cast<size_t>(pid);

   if (idx != request->requestData.handle) {
      return Error::Exists;
   }

   sDevices[idx] = nullptr;
   return Error::OK;
}

static Error
socketIoctl(phys_ptr<ResourceRequest> resourceRequest)
{
   auto error = Error::OK;
   auto device = getDevice(resourceRequest->requestData.handle);

   if (!device) {
      return Error::InvalidHandle;
   }

   auto request = phys_cast<SocketRequest *>(resourceRequest->requestData.args.ioctl.inputBuffer);

   switch (static_cast<SocketCommand>(resourceRequest->requestData.command)) {
   case SocketCommand::Close:
      error = device->closeSocket(request->close.fd);
      break;
   case SocketCommand::Socket:
      error = device->createSocket(request->socket.family,
                                   request->socket.type,
                                   request->socket.proto);
      break;
   default:
      error = Error::InvalidArg;
   }

   return error;
}

static Error
socketIoctlv(phys_ptr<ResourceRequest> request)
{
   auto error = Error::OK;
   auto device = getDevice(request->requestData.handle);

   if (!device) {
      return Error::InvalidHandle;
   }

   switch (static_cast<SocketCommand>(request->requestData.command)) {
   default:
      error = Error::InvalidArg;
   }

   return error;
}

static Error
socketThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sData->messageQueueId,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
         IOS_ResourceReply(request, socketOpen(request));
         break;
      case Command::Close:
         IOS_ResourceReply(request, socketClose(request));
         break;
      case Command::Ioctl:
         IOS_ResourceReply(request, socketIoctl(request));
         break;
      case Command::Ioctlv:
         IOS_ResourceReply(request, socketIoctlv(request));
         break;
      case Command::Suspend:
         // TODO: Do any necessary cleanup!
         return Error::OK;
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }
}

Error
registerSocketResourceManager()
{
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }

   sData->messageQueueId = static_cast<MessageQueueId>(error);
   return IOS_RegisterResourceManager("/dev/socket", sData->messageQueueId);
}

Error
startSocketThread()
{
   auto error = IOS_CreateThread(&socketThreadEntry,
                                 nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 sData->threadStack.size(),
                                 SocketThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sData->threadId = static_cast<ThreadId>(error);
   return IOS_StartThread(sData->threadId);
}

Error
stopSocketThread()
{
   return IOS_JamMessage(sData->messageQueueId,
                         makeMessage(phys_addrof(sData->stopMessage)),
                         MessageFlags::NonBlocking);
}

void
initialiseStaticSocketData()
{
   sData = phys_cast<StaticSocketThreadData *>(allocProcessStatic(sizeof(StaticSocketThreadData)));
   sData->stopMessage.command = Command::Suspend;
}

} // namespace ios::net::internal
