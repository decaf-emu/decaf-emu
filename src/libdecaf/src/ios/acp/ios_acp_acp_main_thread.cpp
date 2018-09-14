#include "ios_acp_acp_main_thread.h"

#include "ios/ios_handlemanager.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/mcp/ios_mcp_ipc.h"

namespace ios::acp::internal
{

using namespace kernel;

constexpr auto AcpMainNumMessages = 100u;
constexpr auto AcpMainThreadStackSize = 0x10000u;
constexpr auto AcpMainThreadPriority = 50u;

struct StaticAcpMainThreadData
{
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, AcpMainNumMessages> messageBuffer;
   be2_array<uint8_t, AcpMainThreadStackSize> threadStack;
};

static phys_ptr<StaticAcpMainThreadData>
sAcpMainThreadData = nullptr;

static Error
acpMainThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sAcpMainThreadData->messageQueueId,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      case Command::Close:
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Resume:
      case Command::Suspend:
         IOS_ResourceReply(request, Error::OK);
         break;
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }
}

Error
startAcpMainThread()
{
   // Create message queue
   auto error = IOS_CreateMessageQueue(phys_addrof(sAcpMainThreadData->messageBuffer),
                                       static_cast<uint32_t>(sAcpMainThreadData->messageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }
   sAcpMainThreadData->messageQueueId = static_cast<MessageQueueId>(error);

   // Register the device
   error = mcp::MCP_RegisterResourceManager("/dev/acp_main", sAcpMainThreadData->messageQueueId);
   if (error < Error::OK) {
      IOS_DestroyMessageQueue(sAcpMainThreadData->messageQueueId);
      return error;
   }

   // Create thread
   error = IOS_CreateThread(&acpMainThreadEntry, nullptr,
                            phys_addrof(sAcpMainThreadData->threadStack) + sAcpMainThreadData->threadStack.size(),
                            static_cast<uint32_t>(sAcpMainThreadData->threadStack.size()),
                            AcpMainThreadPriority,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      IOS_DestroyMessageQueue(sAcpMainThreadData->messageQueueId);
      return error;
   }

   sAcpMainThreadData->threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sAcpMainThreadData->threadId, "AcpMainThread");

   return IOS_StartThread(sAcpMainThreadData->threadId);
}

void
initialiseStaticAcpMainThreadData()
{
   sAcpMainThreadData = allocProcessStatic<StaticAcpMainThreadData>();
}

} // namespace ios::auxil::internal
