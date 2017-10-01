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
                                          internal::getCurrentProcessId(),
                                          CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_OpenAsync(std::string_view device,
              OpenMode mode,
              MessageQueueId asyncNotifyQueue,
              phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosOpen(device,
                                    mode,
                                    queue,
                                    asyncNotifyRequest,
                                    internal::getCurrentProcessId(),
                                    CpuId::ARM);
}

Error
IOS_Close(ResourceHandleId handle)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosClose(handle,
                                           queue,
                                           request,
                                           0,
                                           internal::getCurrentProcessId(),
                                           CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_CloseAsync(ResourceHandleId handle,
               MessageQueueId asyncNotifyQueue,
               phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosClose(handle,
                                     queue,
                                     asyncNotifyRequest,
                                     0,
                                     internal::getCurrentProcessId(),
                                     CpuId::ARM);
}

Error
IOS_Read(ResourceHandleId handle,
         phys_ptr<void> buffer,
         uint32_t length)
{
   // TODO: Implement IOS_Read
   return Error::Invalid;
}

Error
IOS_ReadAsync(ResourceHandleId handle,
              phys_ptr<void> buffer,
              uint32_t length,
              MessageQueueId asyncNotifyQueue,
              phys_ptr<IpcRequest> asyncNotifyRequest)
{
   // TODO: Implement IOS_Read
   return Error::Invalid;
}

Error
IOS_Write(ResourceHandleId handle,
          phys_ptr<const void> buffer,
          uint32_t length)
{
   // TODO: Implement IOS_Write
   return Error::Invalid;
}

Error
IOS_WriteAsync(ResourceHandleId handle,
               phys_ptr<const void> buffer,
               uint32_t length,
               MessageQueueId asyncNotifyQueue,
               phys_ptr<IpcRequest> asyncNotifyRequest)
{
   // TODO: Implement IOS_WriteAsync
   return Error::Invalid;
}

Error
IOS_Seek(ResourceHandleId handle,
         uint32_t offset,
         uint32_t origin)
{
   // TODO: Implement IOS_Seek
   return Error::Invalid;
}

Error
IOS_SeekAsync(ResourceHandleId handle,
              uint32_t offset,
              uint32_t origin,
              MessageQueueId asyncNotifyQueue,
              phys_ptr<IpcRequest> asyncNotifyRequest)
{
   // TODO: Implement IOS_SeekAsync
   return Error::Invalid;
}

Error
IOS_Ioctl(ResourceHandleId handle,
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
                                           internal::getCurrentProcessId(),
                                           CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_IoctlAsync(ResourceHandleId handle,
               uint32_t ioctlRequest,
               phys_ptr<const void> inputBuffer,
               uint32_t inputBufferLength,
               phys_ptr<void> outputBuffer,
               uint32_t outputBufferLength,
               MessageQueueId asyncNotifyQueue,
               phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosIoctl(handle,
                                     ioctlRequest,
                                     inputBuffer,
                                     inputBufferLength,
                                     outputBuffer,
                                     outputBufferLength,
                                     queue,
                                     asyncNotifyRequest,
                                     internal::getCurrentProcessId(),
                                     CpuId::ARM);
}

Error
IOS_Ioctlv(ResourceHandleId handle,
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
                                            internal::getCurrentProcessId(),
                                            CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_IoctlvAsync(ResourceHandleId handle,
                uint32_t ioctlRequest,
                uint32_t numVecIn,
                uint32_t numVecOut,
                phys_ptr<IoctlVec> vecs,
                MessageQueueId asyncNotifyQueue,
                phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosIoctlv(handle,
                                      ioctlRequest,
                                      numVecIn,
                                      numVecOut,
                                      vecs,
                                      queue,
                                      asyncNotifyRequest,
                                      internal::getCurrentProcessId(),
                                      CpuId::ARM);
}

Error
IOS_Resume(ResourceHandleId handle,
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
                                            internal::getCurrentProcessId(),
                                            CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_ResumeAsync(ResourceHandleId handle,
                uint32_t unkArg0,
                uint32_t unkArg1,
                MessageQueueId asyncNotifyQueue,
                phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosResume(handle,
                                      unkArg0,
                                      unkArg1,
                                      queue,
                                      asyncNotifyRequest,
                                      internal::getCurrentProcessId(),
                                      CpuId::ARM);
}

Error
IOS_Suspend(ResourceHandleId handle,
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
                                             internal::getCurrentProcessId(),
                                             CpuId::ARM);
   if (error < Error::OK) {
      return error;
   }

   return internal::waitRequestReply(queue, request);
}

Error
IOS_SuspendAsync(ResourceHandleId handle,
                 uint32_t unkArg0,
                 uint32_t unkArg1,
                 MessageQueueId asyncNotifyQueue,
                 phys_ptr<IpcRequest> asyncNotifyRequest)
{
   phys_ptr<MessageQueue> queue;

   auto error = internal::getMessageQueue(asyncNotifyQueue, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::dispatchIosSuspend(handle,
                                       unkArg0,
                                       unkArg1,
                                       queue,
                                       asyncNotifyRequest,
                                       internal::getCurrentProcessId(),
                                       CpuId::ARM);
}

Error
IOS_SvcMsg(ResourceHandleId handle,
           uint32_t command,
           uint32_t unkArg1,
           uint32_t unkArg2,
           uint32_t unkArg3)
{
   StackObject<IpcRequest> request;
   std::memset(request.getRawPointer(), 0, sizeof(IpcRequest));

   auto queue = internal::getCurrentThreadMessageQueue();
   auto error = internal::dispatchIosSvcMsg(handle,
                                            command,
                                            unkArg1,
                                            unkArg2,
                                            unkArg3,
                                            queue,
                                            request,
                                            internal::getCurrentProcessId(),
                                            CpuId::ARM);
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

   auto receivedRequest = kernel::parseMessage<IpcRequest>(message);
   if (receivedRequest != request) {
      return Error::FailInternal;
   }

   return static_cast<Error>(request->reply);
}

} // namespace internal

} // namespace ios::kernel
