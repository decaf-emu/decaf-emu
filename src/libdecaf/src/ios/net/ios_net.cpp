#include "ios_net.h"
#include "ios_net_subsys.h"
#include "ios_net_socket.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>

namespace ios::net
{

using namespace kernel;
using namespace mcp;

constexpr auto LocalHeapSize = 0x40000u;
constexpr auto CrossHeapSize = 0xC0000u;

constexpr auto NumNetworkMessages = 10u;

struct StaticData
{
   be2_val<bool> subsysStarted;
   be2_array<std::byte, LocalHeapSize> localHeapBuffer;
};

static phys_ptr<StaticData>
sData = nullptr;

namespace internal
{

static void
initialiseStaticData()
{
   sData = kernel::allocProcessStatic<StaticData>();
}

static Error
networkLoop()
{
   StackArray<Message, NumNetworkMessages> messageBuffer;
   StackObject<kernel::Message> message;

   // Create message queue
   auto error = IOS_CreateMessageQueue(messageBuffer, messageBuffer.size());
   if (error < Error::OK) {
      gLog->error("NET: Failed to create message queue, error = {}.", error);
      return error;
   }

   auto messageQueueId = static_cast<MessageQueueId>(error);

   // Register resource manager
   error = MCP_RegisterResourceManager("/dev/network", messageQueueId);
   if (error < Error::OK) {
      gLog->error("NET: Failed to register resource manager for /dev/network, error = {}.", error);
      return error;
   }

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(messageQueueId,
                                              message,
                                              kernel::MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      auto request = kernel::parseMessage<kernel::ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      case Command::Close:
         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Resume:
         if (!sData->subsysStarted) {
            error = internal::startSubsys();
         } else {
            error = Error::OK;
         }

         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Suspend:
         if (sData->subsysStarted) {
            error = internal::stopSubsys();
         } else {
            error = Error::OK;
         }

         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      default:
         kernel::IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   IOS_DestroyMessageQueue(messageQueueId);
   return Error::OK;
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticSocketData();
   internal::initialiseStaticSubsysData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(phys_addrof(sData->localHeapBuffer),
                                                   static_cast<uint32_t>(sData->localHeapBuffer.size()));
   if (error < Error::OK) {
      gLog->error("NET: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("NET: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: bspGetClockInfo
   // TODO: initIoBuf
   // TODO: startWifi24Thread (/dev/wifi24)
   // TODO: startUdsThreads (/dev/ifuds /dev/udscntrl)
   // TODO: startSomeThreads (/dev/ac_main /dev/ndm /dev/dlp)

   error = internal::initSubsys();
   if (error < Error::OK) {
      gLog->error("NET: Failed to initialise subsystem, error = {}.", error);
      return error;
   }

   return internal::networkLoop();
}

} // namespace ios::net
