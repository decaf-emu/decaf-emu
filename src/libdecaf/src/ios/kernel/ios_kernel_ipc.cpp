#include "ios_kernel_ipc.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_process.h"
#include "ios_kernel_resourcemanager.h"
#include "ios/ios_stackobject.h"

namespace ios::kernel
{

namespace internal
{

static Error
waitRequestReply(phys_ptr<MessageQueue> queue,
                 phys_ptr<IpcRequest> request);

} // namespace internal

Result<IpcHandle>
IOS_Open(std::string_view device,
         OpenMode mode)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosOpen(device,
                                          mode,
                                          queue,
                                          request,
                                          internal::getCurrentProcessID(),
                                          CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   error = internal::waitRequestReply(queue, request);
   if (error < Error::OK) {
      return error;
   }

   return static_cast<IpcHandle>(error);
}

namespace internal
{

static Error
waitRequestReply(phys_ptr<MessageQueue> queue,
                 phys_ptr<IpcRequest> request)
{
   StackObject<Message> message;
   auto error = internal::receiveMessage(queue, message, MessageFlags::None);
   if (error < Error::OK) {
      return error;
   }

   auto receivedRequest = phys_ptr<IpcRequest> { static_cast<phys_addr>(static_cast<Message>(*message)) };
   if (receivedRequest != request) {
      return Error::FailInternal;
   }

   return static_cast<Error>(request->reply);
}

} // namespace internal

} // namespace ios::kernel
