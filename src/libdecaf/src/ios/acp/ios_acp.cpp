#include "ios_acp.h"
#include "ios_acp_client_save.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/ios_stackobject.h"

#include "decaf_config.h"

namespace ios::acp
{

using namespace ios::fs;
using namespace ios::kernel;
using namespace ios::mcp;

constexpr auto LocalHeapSize = 0x20000u;
constexpr auto CrossHeapSize = 0x80000u;

constexpr auto AcpProcNumMessages = 10u;
constexpr auto AcpProcThreadStackSize = 0x2000u;
constexpr auto AcpProcThreadPriority = 50u;

struct StaticAcpData
{
   be2_val<Handle> fsaHandle;
};

static phys_ptr<StaticAcpData>
sAcpData = nullptr;

static phys_ptr<void>
sLocalHeapBuffer = nullptr;

namespace internal
{

void
initialiseStaticData()
{
   sAcpData = phys_cast<StaticAcpData *>(allocProcessStatic(sizeof(StaticAcpData)));
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

} // namespace internal

static void
updateSaveAndBossQuota()
{
   // TODO: Update boss and save quota.

   // Make quota for Mii database
   auto path = "/vol/storage_mlc01/usr/save/00050010/1004a000/user/common/db";

   if (decaf::config::system::region == decaf::config::system::Region::USA) {
      path = "/vol/storage_mlc01/usr/save/00050010/1004a100/user/common/db";
   } else if (decaf::config::system::region == decaf::config::system::Region::Europe) {
      path = "/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/db";
   }

   FSAMakeQuota(sAcpData->fsaHandle, path, 0x660, 24 * 1024 * 1024);
}

static Error
acpSaveStart()
{
   auto error = client::save::start();
   if (error < Error::OK) {
      return error;
   }

   // TODO: acp::client::save::bossStorage::start();

   // TODO: This is not actually called here, but not sure where yet.
   updateSaveAndBossQuota();
   return Error::OK;
}

static Error
acpSaveStop()
{
   return Error::OK;
}

static Error
acpSystemStart()
{
   auto error = FSAOpen();
   if (error < Error::OK) {
      return error;
   }

   sAcpData->fsaHandle = static_cast<Handle>(error);
   return Error::OK;
}

static Error
acpSystemStop()
{
   if (sAcpData->fsaHandle > 0) {
      FSAClose(sAcpData->fsaHandle);
   }

   return Error::OK;
}

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   StackArray<Message, 10> messages;
   StackObject<Message> message;
   MessageQueueId messageQueueId;

   // Initialise static memory
   internal::initialiseStaticData();

   auto error = IOS_SetThreadPriority(CurrentThread, 50);
   if (error < Error::OK) {
      return error;
   }

   // Initialise process heaps
   error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      return error;
   }

   // Setup /dev/acpproc
   error = IOS_CreateMessageQueue(messages, messages.size());
   if (error < Error::OK) {
      return error;
   }
   messageQueueId = static_cast<MessageQueueId>(error);

   error = MCP_RegisterResourceManager("/dev/acpproc", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   // Run acpproc
   while (true) {
      auto error = IOS_ReceiveMessage(messageQueueId, message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      case Command::Close:
         error = Error::OK;
         break;
      case Command::Suspend:
         error = acpSaveStop();
         if (error >= Error::OK) {
            error = acpSystemStop();
         }
         break;
      case Command::Resume:
         error = acpSystemStart();
         if (error >= Error::OK) {
            error = acpSaveStart();
         }
         break;
      default:
         error = Error::Invalid;
      }

      IOS_ResourceReply(request, error);
   }
}

namespace internal
{

Handle
getFsaHandle()
{
   return sAcpData->fsaHandle;
}

} // namespace internal

} // namespace ios::acp
