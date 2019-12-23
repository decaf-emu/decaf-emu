#include "ios_crypto.h"
#include "ios_crypto_log.h"

#include "decaf_log.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_debug.h"
#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/mcp/ios_mcp_ipc.h"

using namespace ios::kernel;
using namespace ios::mcp;

namespace ios::crypto
{

constexpr auto CrossHeapSize = 0x10000u;
constexpr auto MainThreadNumMessages = 260u;

struct CryptoHandle
{
   be2_val<BOOL> used = FALSE;
   be2_val<ProcessId> processId;
};

struct StaticCryptoData
{
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, MainThreadNumMessages> messages;
   be2_array<CryptoHandle, 32> handles;
};

static phys_ptr<StaticCryptoData> sCryptoData = nullptr;

namespace internal
{

Logger cryptoLog = { };

static void
initialiseStaticData()
{
   sCryptoData = allocProcessStatic<StaticCryptoData>();
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   StackObject<Message> message;

   // Initialise process static data
   internal::initialiseStaticData();

   // Initialise logger
   internal::cryptoLog = decaf::makeLogger("IOS_CRYPTO");

   // Initialise process heaps
   auto error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: Failed to create cross process heap, error = {}.",
         error);
      return error;
   }

   // Setup /dev/acpproc
   error = IOS_CreateMessageQueue(phys_addrof(sCryptoData->messages),
                                  sCryptoData->messages.size());
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: IOS_CreateMessageQueue failed with error = {}",
         error);
      return error;
   }
   sCryptoData->messageQueueId = static_cast<MessageQueueId>(error);

   error = MCP_RegisterResourceManager("/dev/crypto",
                                       sCryptoData->messageQueueId);
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: MCP_RegisterResourceManager failed for /dev/crypto with error = {}",
         error);
      return error;
   }

   // Run /dev/crypto
   while (true) {
      error = IOS_ReceiveMessage(sCryptoData->messageQueueId, message, MessageFlags::None);
      if (error < Error::OK) {
         internal::cryptoLog->error(
            "processEntryPoint: IOS_ReceiveMessage failed with error = {}", error);
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
         error = Error::NoResource;
         for (auto i = 0u; i < sCryptoData->handles.size(); ++i) {
            if (!sCryptoData->handles[i].used) {
               sCryptoData->handles[i].used = TRUE;
               sCryptoData->handles[i].processId = request->requestData.processId;
               error = static_cast<Error>(i);
               break;
            }
         }

         IOS_ResourceReply(request, error);
         break;
      case Command::Close:
         if (request->requestData.handle < sCryptoData->handles.size()) {
            sCryptoData->handles[request->requestData.handle].used = FALSE;
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Suspend:
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Resume:
         IOS_ResourceReply(request, Error::OK);
         break;
      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
   return Error::OK;
}

} // namespace ios::crypto
