#include "ios_acp_enum.h"
#include "ios_acp_log.h"
#include "ios_acp_nnsm.h"

#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/mcp/ios_mcp_ipc.h"

namespace ios::acp::internal
{

using namespace kernel;

constexpr auto NnsmNumMessages = 0x10u;
constexpr auto NnsmThreadStackSize = 0x2000u;
constexpr auto NnsmThreadPriority = 50u;

struct StaticNnsmData
{
   be2_val<BOOL> running;
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, NnsmNumMessages> messageBuffer;
   be2_array<uint8_t, NnsmThreadStackSize> threadStack;
};

static phys_ptr<StaticNnsmData>
sNnsmData = nullptr;

static Error
nnsmWaitForResume()
{
   StackObject<Message> message;

   // Read Open
   auto error = IOS_ReceiveMessage(sNnsmData->messageQueueId,
                                   message,
                                   MessageFlags::None);
   if (error < Error::OK) {
      internal::acpLog->error(
         "nnsmWaitForResume: IOS_ReceiveMessage failed for Open with error = {}", error);
      return error;
   }

   auto request = parseMessage<ResourceRequest>(message);
   if (request->requestData.command != Command::Open ||
       request->requestData.processId != ProcessId::MCP) {
      internal::acpLog->error(
         "nnsmWaitForResume: Received unexpected message, expected Open from MCP, command = {}, processId = {}",
         request->requestData.command, request->requestData.processId);
      return Error::FailInternal;
   }

   if (error = IOS_ResourceReply(request, Error::OK); error < Error::OK) {
      internal::acpLog->error(
         "nnsmWaitForResume: IOS_ResourceReply failed for Open with error = {}", error);
      return error;
   }

   // Read Resume
   error = IOS_ReceiveMessage(sNnsmData->messageQueueId,
                              message,
                              MessageFlags::None);
   if (error < Error::OK) {
      internal::acpLog->error(
         "nnsmWaitForResume: IOS_ReceiveMessage failed for Resume with error = {}", error);
      return error;
   }

   request = parseMessage<ResourceRequest>(message);
   if (request->requestData.command != Command::Resume ||
       request->requestData.processId != ProcessId::MCP) {
      internal::acpLog->error(
         "nnsmWaitForResume: Received unexpected message, expected Resume from MCP, command = {}, processId = {}",
         request->requestData.command, request->requestData.processId);
      return Error::FailInternal;
   }

   // TODO: Send resume message to /dev/nnmisc
   if (error = IOS_ResourceReply(request, Error::OK); error < Error::OK) {
      internal::acpLog->error(
         "nnsmWaitForResume: IOS_ResourceReply failed for Resume with error = {}", error);
      return error;
   }

   sNnsmData->running = TRUE;
   return Error::OK;
}

static void
nnsmIoctl(phys_ptr<ResourceRequest> resourceRequest)
{
   switch (static_cast<NssmCommand>(resourceRequest->requestData.args.ioctl.request)) {
   case NssmCommand::RegisterService:
   case NssmCommand::UnregisterService:
      // TODO: Do something with registered / unregistered services.
      IOS_ResourceReply(resourceRequest, Error::OK);
      return;
   default:
      IOS_ResourceReply(resourceRequest, Error::InvalidArg);
   }
}

static Error
nnsmThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;
   if (auto error = nnsmWaitForResume(); error < Error::OK) {
      internal::acpLog->error(
         "nnsmThreadEntry: nnsmWaitForResume failed with error = {}", error);
      return error;
   }

   while (sNnsmData->running) {
      auto error = IOS_ReceiveMessage(sNnsmData->messageQueueId,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto resourceRequest = parseMessage<ResourceRequest>(message);
      switch (resourceRequest->requestData.command) {
      case Command::Open:
      case Command::Close:
         IOS_ResourceReply(resourceRequest, Error::OK);
         break;
      case Command::Ioctl:
         nnsmIoctl(resourceRequest);
         break;
      case Command::Suspend:
         sNnsmData->running = FALSE;
         IOS_ResourceReply(resourceRequest, Error::OK);
         break;
      default:
         IOS_ResourceReply(resourceRequest, Error::InvalidArg);
      }
   }

   return Error::OK;
}

Error
startNnsm()
{
   // Create message queue
   auto error = IOS_CreateMessageQueue(phys_addrof(sNnsmData->messageBuffer),
                                       static_cast<uint32_t>(sNnsmData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }
   sNnsmData->messageQueueId = static_cast<MessageQueueId>(error);

   // Register the device
   error = mcp::MCP_RegisterResourceManager("/dev/nnsm", sNnsmData->messageQueueId);
   if (error < Error::OK) {
      IOS_DestroyMessageQueue(sNnsmData->messageQueueId);
      return error;
   }

   // Create thread
   error = IOS_CreateThread(&nnsmThreadEntry, nullptr,
                            phys_addrof(sNnsmData->threadStack) + sNnsmData->threadStack.size(),
                            static_cast<uint32_t>(sNnsmData->threadStack.size()),
                            NnsmThreadPriority,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      IOS_DestroyMessageQueue(sNnsmData->messageQueueId);
      return error;
   }

   sNnsmData->threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sNnsmData->threadId, "NnsmThread");

   return IOS_StartThread(sNnsmData->threadId);
}

void
initialiseStaticNnsmData()
{
   sNnsmData = allocProcessStatic<StaticNnsmData>();
}

} // namespace ios::acp::internal
