#include "ios_net_socket.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

namespace ios::net::internal
{

using namespace kernel;

constexpr auto NumSocketMessages = 40u;
constexpr auto SocketThreadStackSize = 0x4000u;
constexpr auto SocketThreadPriority = 69u;

struct StaticData
{
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_struct<IpcRequest> stopMessage;
   be2_array<Message, NumSocketMessages> messageBuffer;
   be2_array<uint8_t, SocketThreadStackSize> threadStack;
};

static phys_ptr<StaticData>
sData = nullptr;

static Error
socketThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sData->messageQueueId,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
         // Seems like one /dev/socket per title id x process id
         break;
      case Command::Suspend:
         // TODO: Do any necessary cleanup!
         return Error::OK;
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }
}

Error
registerSocketResourceManager()
{
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }

   sData->messageQueueId = static_cast<MessageQueueId>(error);
   return IOS_RegisterResourceManager("/dev/socket", sData->messageQueueId);
}

Error
startSocketThread()
{
   auto error = IOS_CreateThread(&socketThreadEntry,
                                 nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 sData->threadStack.size(),
                                 SocketThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sData->threadId = static_cast<ThreadId>(error);
   return IOS_StartThread(sData->threadId);
}

Error
stopSocketThread()
{
   return IOS_JamMessage(sData->messageQueueId,
                         makeMessage(phys_addrof(sData->stopMessage)),
                         MessageFlags::NonBlocking);
}

void
initialiseStaticSocketData()
{
   sData = allocProcessStatic<StaticData>();
   sData->stopMessage.command = Command::Suspend;
}

} // namespace ios::net::internal
