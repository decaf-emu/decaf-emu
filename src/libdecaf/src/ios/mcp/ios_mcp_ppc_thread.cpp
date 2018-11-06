#include "ios_mcp_enum.h"
#include "ios_mcp_pm_thread.h"
#include "ios_mcp_ppc_thread.h"

#include "cafe/kernel/cafe_kernel.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

namespace ios::mcp::internal
{

using namespace kernel;

using RegisteredResourceManagerId = int32_t;

constexpr auto MaxNumPpcMessages = 10u;

constexpr auto PpcThreadStackSize = 0x2000u;
constexpr auto PpcThreadPriority = 123u;

constexpr auto PpcAppHandle = 0x707061; // 'ppa'
constexpr auto PpcKernelHandle = 0x6E726B; // 'nrk'

struct StaticPpcThreadData
{
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, PpcThreadStackSize> threadStack;
};

static phys_ptr<StaticPpcThreadData>
sPpcThreadData;

static Error
ppcThreadEntry(phys_ptr<void> /*context*/)
{
   StackArray<Message, MaxNumPpcMessages> messageBuffer;
   StackObject<Message> message;

   // Create message queue
   auto error = IOS_CreateMessageQueue(messageBuffer, messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }
   auto messageQueueId = static_cast<MessageQueueId>(error);

   // Register devices
   error = registerResourceManager("/dev/ppc_app", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   error = registerResourceManager("/dev/ppc_kernel", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(messageQueueId,
                                 message,
                                 MessageFlags::None);
      if (error < Error::OK) {
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         auto name = std::string_view { request->requestData.args.open.name.get() };
         auto error = Error::InvalidArg;

         if (name.compare("/dev/ppc_kernel") == 0) {
            error = static_cast<Error>(PpcKernelHandle);
         } else if (name.compare("/dev/ppc_app") == 0) {
            error = static_cast<Error>(PpcAppHandle);
         }

         IOS_ResourceReply(request, error);
         break;
      }

      case Command::Close:
      {
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Resume:
      {
         if (request->requestData.handle == PpcKernelHandle) {
            // TODO: Until we have proper permission initialisation in IOS for
            // CafeOS kernel let's just force all permission
            StackObject<uint64_t> mask;
            *mask = 0xFFFFFFFFFFFFFFFFull;
            IOS_SetClientCapabilities(ProcessId::COSKERNEL,
                                      ResourcePermissionGroup::All,
                                      mask);

            // Boot the PPC kernel!
            cafe::kernel::start();
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      }

      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   return error;
}

Error
startPpcThread()
{
   // Create thread
   auto error = IOS_CreateThread(&ppcThreadEntry, nullptr,
                                 phys_addrof(sPpcThreadData->threadStack) + sPpcThreadData->threadStack.size(),
                                 static_cast<uint32_t>(sPpcThreadData->threadStack.size()),
                                 PpcThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sPpcThreadData->threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sPpcThreadData->threadId, "PpcThread");

   return IOS_StartThread(sPpcThreadData->threadId);
}

void
initialiseStaticPpcThreadData()
{
   sPpcThreadData = allocProcessStatic<StaticPpcThreadData>();
}

} // namespace ios::mcp::internal
