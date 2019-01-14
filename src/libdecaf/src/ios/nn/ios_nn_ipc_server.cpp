#include "ios_nn_ipc_server.h"
#include "ios_nn_tls.h"

#include "ios/ios_stackobject.h"
#include "ios/acp/ios_acp_nnsm_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_timer.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "nn/ios/nn_ios_error.h"
#include "nn/ipc/nn_ipc_format.h"

using namespace ios;
using namespace ios::acp;
using namespace ios::kernel;
using namespace ios::mcp;

namespace nn::ipc
{

Result
Server::initialise(std::string_view deviceName,
                   phys_ptr<Message> messageBuffer,
                   uint32_t numMessages)
{
   auto error = IOS_CreateMessageQueue(messageBuffer, numMessages);
   if (error < ::ios::Error::OK) {
      return ::nn::ios::convertError(error);
   }
   mQueueId = static_cast<MessageQueueId>(error);

   error = IOS_CreateTimer(std::chrono::microseconds(0),
                           std::chrono::microseconds(0),
                           mQueueId,
                           mTimerMessage);
   if (error < ::ios::Error::OK) {
      return ::nn::ios::convertError(error);
   }
   mTimer = static_cast<TimerId>(error);

   if (mIsMcpResourceManager) {
      error = MCP_RegisterResourceManager(deviceName, mQueueId);
   } else {
      error = IOS_RegisterResourceManager(deviceName, mQueueId);
   }

   mDeviceName = deviceName;
   return ::nn::ios::convertError(error);
}

void
Server::registerService(ServiceId serviceId,
                        CommandHandler commandHandler)
{
   for (auto &service : mServices) {
      if (!service.handler) {
         service.id = serviceId;
         service.handler = commandHandler;
         break;
      }
   }
}

Error
Server::threadEntryWrapper(phys_ptr<void> ptr)
{
   auto self = phys_cast<Server *>(ptr);
   return static_cast<Error>(self->threadEntry().code);
}

Result
Server::waitForResume()
{
   StackObject<Message> message;

   // Read Open
   auto error = IOS_ReceiveMessage(mQueueId, message, MessageFlags::None);
   if (error < Error::OK) {
      return error;
   }

   auto request = parseMessage<ResourceRequest>(message);
   if (request->requestData.command != ::ios::Command::Open) {
      return Error::FailInternal;
   }

   if (error = IOS_ResourceReply(request, Error::OK); error < Error::OK) {
      return error;
   }

   // Read Resume
   error = IOS_ReceiveMessage(mQueueId, message, MessageFlags::None);
   if (error < Error::OK) {
      return error;
   }

   request = parseMessage<ResourceRequest>(message);
   if (request->requestData.command != ::ios::Command::Resume) {
      return Error::FailInternal;
   }

   mResumeArgs = request->ipcRequest->args.resume;

   if (error = IOS_ResourceReply(request, Error::OK); error < Error::OK) {
      return error;
   }

   return Error::OK;
}

Error
Server::openDeviceHandle(uint64_t caps,
                             ProcessId processId)
{
   for (auto &deviceHandle : mDeviceHandles) {
      if (deviceHandle.open) {
         continue;
      }

      deviceHandle.open = true;
      deviceHandle.caps = caps;
      deviceHandle.processId = processId;

      auto index = &deviceHandle - &mDeviceHandles[0];
      return static_cast<Error>(index);
   }

   return Error::Max;
}

Error
Server::closeDeviceHandle(DeviceHandleId handleId)
{
   if (handleId >= mDeviceHandles.size()) {
      return Error::InvalidHandle;
   }

   auto &deviceHandle = mDeviceHandles[handleId];
   if (!deviceHandle.open) {
      return Error::InvalidHandle;
   }

   deviceHandle.open = false;
   deviceHandle.processId = ProcessId::Invalid;
   deviceHandle.caps = 0ull;
   return Error::OK;
}

Error
Server::handleMessage(phys_ptr<ResourceRequest> request)
{
   auto &ioctlv = request->requestData.args.ioctlv;

   if (!ioctlv.numVecOut ||
       ioctlv.vecs[ioctlv.numVecIn].len < sizeof(RequestHeader) ||
       ioctlv.vecs[0].len < sizeof(ResponseHeader)) {
      return Error::InvalidArg;
   }

   auto requestHeader = phys_cast<RequestHeader *>(ioctlv.vecs[ioctlv.numVecIn].paddr);
   auto responseHeader = phys_cast<ResponseHeader *>(ioctlv.vecs[0].paddr);

   CommandHandlerArgs args;
   args.resourceRequest = request;
   args.requestBuffer = requestHeader + 1;
   args.requestBufferSize = ioctlv.vecs[ioctlv.numVecIn].len - sizeof(RequestHeader);

   args.responseBuffer = responseHeader + 1;
   args.responseBufferSize = ioctlv.vecs[0].len - sizeof(ResponseHeader);

   args.numVecsIn = ioctlv.numVecIn;
   args.numVecsOut = ioctlv.numVecOut;
   args.vecs = ioctlv.vecs;

   for (auto &service : mServices) {
      if (service.id != requestHeader->service) {
         continue;
      }

      auto result = service.handler(requestHeader->unk0x08, requestHeader->command, args);
      responseHeader->result = result.code;
      return Error::OK;
   }

   return Error::InvalidArg;
}

Result
Server::runMessageLoop()
{
   StackObject<Message> message;
   auto error = Error::OK;

   while (true) {
      mMutex.lock();

      error = IOS_ReceiveMessage(mQueueId, message, MessageFlags::None);
      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case ::ios::Command::Open:
      {
         error = openDeviceHandle(request->requestData.args.open.caps,
                                  request->requestData.processId);
         IOS_ResourceReply(request, error);
         break;
      }
      case ::ios::Command::Close:
         error = closeDeviceHandle(static_cast<DeviceHandleId>(request->requestData.handle));
         IOS_ResourceReply(request, error);
         break;
      case ::ios::Command::Ioctlv:
         error = handleMessage(request);
         IOS_ResourceReply(request, error);
         break;
      default:
         IOS_ResourceReply(request, Error::Invalid);
      }

      mMutex.unlock();
   }

   return error;
}

Result
Server::resolvePendingMessages()
{
   return Error::OK;
}

Result
Server::threadEntry()
{
   if (mResourcePermissionGroup != ResourcePermissionGroup::None) {
      IOS_AssociateResourceManager(mDeviceName, mResourcePermissionGroup);
   }

   if (auto result = waitForResume(); result.failed()) {
      return result;
   }

   NNSM_RegisterServer(mDeviceName);
   intialiseServer();
   auto result = runMessageLoop();

   finaliseServer();
   resolvePendingMessages();
   NNSM_UnregisterServer(mDeviceName);
   return result;
}

Result
Server::start(phys_ptr<uint8_t> stackTop,
                  uint32_t stackSize,
                  kernel::ThreadPriority priority)
{
   return mThread.start(Server::threadEntryWrapper,
                        phys_this(this),
                        stackTop,
                        stackSize,
                        priority);
}

} // namespace nn::ipc
