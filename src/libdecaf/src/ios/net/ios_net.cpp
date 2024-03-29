#include "ios_net.h"
#include "ios_net_ac_main_server.h"
#include "ios_net_log.h"
#include "ios_net_ndm_server.h"
#include "ios_net_subsys.h"
#include "ios_net_socket_async_task.h"
#include "ios_net_socket_thread.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>

using namespace ios::kernel;

namespace ios::net
{

using namespace kernel;
using namespace mcp;

constexpr auto LocalHeapSize = 0x40000u;
constexpr auto CrossHeapSize = 0xC0000u;

constexpr auto NumNetworkMessages = 10u;

struct StaticNetData
{
   be2_val<bool> subsysStarted;
};

static phys_ptr<StaticNetData>
sData = nullptr;

static phys_ptr<void>
sLocalHeapBuffer = nullptr;

namespace internal
{

Logger netLog = { };

static void
initialiseStaticData()
{
   sData = allocProcessStatic<StaticNetData>();
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

static Error
networkLoop()
{
   StackArray<Message, NumNetworkMessages> messageBuffer;
   StackObject<Message> message;

   // Create message queue
   auto error = IOS_CreateMessageQueue(messageBuffer, messageBuffer.size());
   if (error < Error::OK) {
      netLog->error("NET: Failed to create message queue, error = {}.", error);
      return error;
   }

   auto messageQueueId = static_cast<MessageQueueId>(error);

   // Register resource manager
   error = MCP_RegisterResourceManager("/dev/network", messageQueueId);
   if (error < Error::OK) {
      netLog->error("NET: Failed to register resource manager for /dev/network, error = {}.", error);
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(messageQueueId, message, MessageFlags::None);
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
         if (!sData->subsysStarted) {
            error = internal::startSubsys();
         } else {
            error = Error::OK;
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Suspend:
         if (sData->subsysStarted) {
            error = internal::stopSubsys();
         } else {
            error = Error::OK;
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   IOS_DestroyMessageQueue(messageQueueId);
   return Error::OK;
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Initialise logger
   internal::netLog = decaf::makeLogger("IOS_NET");

   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticAcMainServerData();
   internal::initialiseStaticNdmServerData();
   internal::initialiseStaticSocketData();
   internal::initialiseStaticSocketAsyncTaskData();
   internal::initialiseStaticSubsysData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: bspGetClockInfo
   // TODO: initIoBuf
   // TODO: start wifi24 thread (/dev/wifi24)
   // TODO: start uds threads (/dev/ifuds /dev/udscntrl)
   // TODO: start nn servers for /dev/dlp

   error = internal::startNdmServer();
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to start ndm server, error = {}.", error);
      return error;
   }

   error = internal::startAcMainServer();
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to start ac_main server, error = {}.", error);
      return error;
   }

   error = internal::startSocketAsyncTaskThread();
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to initialise socket async task thread, error = {}.", error);
      return error;
   }

   error = internal::initSubsys();
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to initialise subsystem, error = {}.", error);
      return error;
   }

   error = internal::networkLoop();
   if (error < Error::OK) {
      internal::netLog->error("NET: networkLoop returned error = {}.", error);
      return error;
   }

   error = internal::joinAcMainServer();
   if (error < Error::OK) {
      internal::netLog->error("NET: Failed to join ac_main server, error = {}.", error);
      return error;
   }

   return IOS_SuspendThread(IOS_GetCurrentThreadId());
}

} // namespace ios::net
