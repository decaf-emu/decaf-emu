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

Error
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

   return error;
}

Error
IOS_Close(ResourceHandleID handle)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosClose(handle,
                                           queue,
                                           request,
                                           0,
                                           internal::getCurrentProcessID(),
                                           CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_Read(ResourceHandleID handle,
         phys_ptr<void> buffer,
         uint32_t length)
{
   return Error::Invalid;
}

Error
IOS_Write(ResourceHandleID handle,
          phys_ptr<const void> buffer,
          uint32_t length)
{
   return Error::Invalid;
}

Error
IOS_Seek(ResourceHandleID handle,
         uint32_t offset,
         uint32_t origin)
{
   return Error::Invalid;
}

Error
IOS_Ioctl(ResourceHandleID handle,
          uint32_t ioctlRequest,
          phys_ptr<const void> inputBuffer,
          uint32_t inputBufferLength,
          phys_ptr<void> outputBuffer,
          uint32_t outputBufferLength)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosIoctl(handle,
                                           ioctlRequest,
                                           inputBuffer,
                                           inputBufferLength,
                                           outputBuffer,
                                           outputBufferLength,
                                           queue,
                                           request,
                                           internal::getCurrentProcessID(),
                                           CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_Ioctlv(ResourceHandleID handle,
           uint32_t ioctlRequest,
           uint32_t numVecIn,
           uint32_t numVecOut,
           phys_ptr<IoctlVec> vecs)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosIoctlv(handle,
                                            ioctlRequest,
                                            numVecIn,
                                            numVecOut,
                                            vecs,
                                            queue,
                                            request,
                                            internal::getCurrentProcessID(),
                                            CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_Resume(ResourceHandleID handle,
           uint32_t unkArg0,
           uint32_t unkArg1)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosResume(handle,
                                            unkArg0,
                                            unkArg1,
                                            queue,
                                            request,
                                            internal::getCurrentProcessID(),
                                            CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_Suspend(ResourceHandleID handle,
            uint32_t unkArg0,
            uint32_t unkArg1)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosSuspend(handle,
                                             unkArg0,
                                             unkArg1,
                                             queue,
                                             request,
                                             internal::getCurrentProcessID(),
                                             CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_SvcMsg(ResourceHandleID handle,
           uint32_t unkArg0,
           uint32_t unkArg1,
           uint32_t unkArg2,
           uint32_t unkArg3)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosSvcMsg(handle,
                                            unkArg0,
                                            unkArg1,
                                            unkArg2,
                                            unkArg3,
                                            queue,
                                            request,
                                            internal::getCurrentProcessID(),
                                            CpuID::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
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
