#include "ios_net_socket_async_task.h"
#include "ios_net_socket_device.h"
#include "ios_net_socket_response.h"
#include "ios_net_socket_request.h"
#include "ios_net_socket_thread.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_error.h"
#include "ios/ios_stackobject.h"
#include "ios/ios_network_thread.h"

#include <array>
#include <optional>

using ios::internal::submitNetworkTask;

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

static std::optional<Error>
socketIoctl(phys_ptr<ResourceRequest> resourceRequest)
{
   auto device = getDevice(resourceRequest->requestData.handle);
   if (!device) {
      return Error::InvalidHandle;
   }

   auto request = phys_cast<const SocketRequest *>(resourceRequest->requestData.args.ioctl.inputBuffer);
   auto response = phys_cast<SocketResponse *>(resourceRequest->requestData.args.ioctl.outputBuffer);

   switch (static_cast<SocketCommand>(resourceRequest->requestData.args.ioctl.request)) {
   case SocketCommand::Accept:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketAcceptRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->accept(resourceRequest,
                           request->accept.fd,
                           phys_addrof(request->accept.addr),
                           request->accept.addrlen));
      });
      break;
   case SocketCommand::Bind:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketBindRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->bind(resourceRequest,
                         request->bind.fd,
                         phys_addrof(request->bind.addr),
                         request->bind.addrlen));
      });
      break;
   case SocketCommand::Close:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketCloseRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->closeSocket(resourceRequest,
                                request->close.fd));
      });
      break;
   case SocketCommand::Connect:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketConnectRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->connect(resourceRequest,
                            request->connect.fd,
                            phys_addrof(request->connect.addr),
                            request->connect.addrlen));
      });
      break;
   case SocketCommand::GetPeerName:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketGetPeerNameRequest) ||
          resourceRequest->requestData.args.ioctl.outputLength != sizeof(SocketGetPeerNameResponse)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->getpeername(resourceRequest,
                                phys_addrof(request->getpeername),
                                phys_addrof(response->getpeername)));
      });
      break;
   case SocketCommand::Listen:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketListenRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->listen(resourceRequest,
                           request->listen.fd,
                           request->listen.backlog));
      });
      break;
   case SocketCommand::Select:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketSelectRequest) ||
          resourceRequest->requestData.args.ioctl.outputLength != sizeof(SocketSelectResponse)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->select(resourceRequest,
                           phys_addrof(request->select),
                           phys_addrof(response->select)));
      });
      break;
   case SocketCommand::Socket:
      if (resourceRequest->requestData.args.ioctl.inputLength != sizeof(SocketSocketRequest)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      submitNetworkTask([=]() {
         completeSocketTask(
            resourceRequest,
            device->createSocket(resourceRequest,
                                 request->socket.family,
                                 request->socket.type,
                                 request->socket.proto));
      });
      break;
   case SocketCommand::GetProcessSocketHandle:
      if (static_cast<size_t>(request->getProcessSocketHandle.processId) < sDevices.size() &&
          sDevices[request->getProcessSocketHandle.processId]) {
         return static_cast<Error>(request->getProcessSocketHandle.processId);
      } else {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }
      break;
   default:
      return Error::Invalid;
   }

   return {}; // async request
}

static std::optional<Error>
socketIoctlv(phys_ptr<ResourceRequest> resourceRequest)
{
   auto device = getDevice(resourceRequest->requestData.handle);

   if (!device) {
      return Error::InvalidHandle;
   }

   switch (static_cast<SocketCommand>(resourceRequest->requestData.args.ioctlv.request)) {
   case SocketCommand::DnsQuery:
      if (resourceRequest->requestData.args.ioctlv.numVecIn == 1 &&
          resourceRequest->requestData.args.ioctlv.numVecOut == 1 &&
          resourceRequest->requestData.args.ioctlv.vecs[0].len == sizeof(SocketDnsQueryRequest) &&
          resourceRequest->requestData.args.ioctlv.vecs[1].len == sizeof(SocketDnsQueryResponse)) {
         submitNetworkTask([=]() {
            completeSocketTask(
               resourceRequest,
               device->dnsQuery(resourceRequest,
                                phys_cast<SocketDnsQueryRequest *>(resourceRequest->requestData.args.ioctlv.vecs[0].paddr),
                                phys_cast<SocketDnsQueryResponse *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr)));
         });
      } else {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }
      break;
   case SocketCommand::Recv:
      if (resourceRequest->requestData.args.ioctlv.numVecIn == 1 &&
          resourceRequest->requestData.args.ioctlv.numVecOut == 3 &&
          resourceRequest->requestData.args.ioctlv.vecs[0].len == sizeof(SocketRecvRequest)) {
         submitNetworkTask([=]() {
            completeSocketTask(
               resourceRequest,
               device->recv(resourceRequest,
                            phys_cast<const SocketRecvRequest *>(resourceRequest->requestData.args.ioctlv.vecs[0].paddr),
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[1].len,
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[2].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[2].len,
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[3].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[3].len));
         });
      } else {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }
      break;
   case SocketCommand::SetSockOpt:
      if (resourceRequest->requestData.args.ioctlv.numVecIn == 2 &&
          resourceRequest->requestData.args.ioctlv.numVecOut == 0 &&
          resourceRequest->requestData.args.ioctlv.vecs[1].len == sizeof(SocketSetSockOptRequest)) {
         submitNetworkTask([=]() {
            completeSocketTask(
               resourceRequest,
               device->setsockopt(resourceRequest,
                                  phys_cast<SocketSetSockOptRequest *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr),
                                  phys_cast<void *>(resourceRequest->requestData.args.ioctlv.vecs[0].paddr),
                                  resourceRequest->requestData.args.ioctlv.vecs[0].len));
         });
      } else {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }
      break;
   case SocketCommand::Send:
      if (resourceRequest->requestData.args.ioctlv.numVecIn == 4 &&
          resourceRequest->requestData.args.ioctlv.numVecOut == 0 &&
          resourceRequest->requestData.args.ioctlv.vecs[0].len == sizeof(SocketSendRequest)) {
         submitNetworkTask([=]() {
            completeSocketTask(
               resourceRequest,
               device->send(resourceRequest,
                            phys_cast<const SocketSendRequest *>(resourceRequest->requestData.args.ioctlv.vecs[0].paddr),
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[1].len,
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[2].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[2].len,
                            phys_cast<char *>(resourceRequest->requestData.args.ioctlv.vecs[3].paddr),
                            resourceRequest->requestData.args.ioctlv.vecs[3].len));
         });
      } else {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }
      break;
   default:
      return Error::Invalid;
   }

   return {}; // async request
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
         if (auto result = socketIoctl(request); result.has_value()) {
            IOS_ResourceReply(request, result.value());
         }
         break;
      case Command::Ioctlv:
         if (auto result = socketIoctlv(request); result.has_value()) {
            IOS_ResourceReply(request, result.value());
         }
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
   kernel::internal::setThreadName(sData->threadId, "SocketThread");

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
   sData = allocProcessStatic<StaticSocketThreadData>();
   sData->stopMessage.command = Command::Suspend;
}

} // namespace ios::net::internal
