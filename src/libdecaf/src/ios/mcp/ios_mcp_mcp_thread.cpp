#include "ios_mcp_mcp_thread.h"
#include "ios_mcp_pm_thread.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

namespace ios::mcp::internal
{

constexpr auto McpThreadStackSize = 0x8000u;
constexpr auto McpThreadPriority = 123u;
constexpr auto MaxNumMessages = 0xC8u;

using namespace kernel;

struct StaticData
{
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, McpThreadStackSize> threadStack;
   be2_array<Message, MaxNumMessages> messageBuffer;
};

static phys_ptr<StaticData>
sData;

static Error
mcpThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }

   auto messageQueueId = static_cast<MessageQueueId>(error);
   error = registerResourceManager("/dev/mcp", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_AssociateResourceManager("/dev/mcp", ResourcePermissionGroup::MCP);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(messageQueueId,
                                 message,
                                 MessageFlags::NonBlocking);
      if (error < Error::OK) {
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   return error;
}

Error
startMcpThread()
{
   auto error = IOS_CreateThread(&mcpThreadEntry,
                                 nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 sData->threadStack.size(),
                                 McpThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   return IOS_StartThread(error);
}

void
initialiseStaticMcpThreadData()
{
   sData = allocProcessStatic<StaticData>();
}

} // namespace ios::mcp::internal
