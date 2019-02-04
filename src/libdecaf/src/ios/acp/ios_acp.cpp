#include "ios_acp.h"
#include "ios_acp_client_save.h"
#include "ios_acp_log.h"
#include "ios_acp_main_server.h"
#include "ios_acp_nnsm.h"
#include "ios_acp_pdm_server.h"

#include "decaf_log.h"
#include "ios/ios_stackobject.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/nn/ios_nn.h"

#include <common/log.h>

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

static phys_ptr<StaticAcpData> sAcpData = nullptr;
static phys_ptr<void> sLocalHeapBuffer = nullptr;

namespace internal
{

std::shared_ptr<spdlog::logger> acpLog = nullptr;

void
initialiseStaticData()
{
   sAcpData = allocProcessStatic<StaticAcpData>();
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

} // namespace internal

static void
updateSaveAndBossQuota()
{
   // TODO: Update boss and save quota.

   // Make quota for Mii database in all regions
   // Note: Real IOS only does it for current system region
   FSAMakeQuota(sAcpData->fsaHandle,
                "/vol/storage_mlc01/usr/save/00050010/1004a000/user/common/db",
                0x660, 24 * 1024 * 1024);
   FSAMakeQuota(sAcpData->fsaHandle,
                "/vol/storage_mlc01/usr/save/00050010/1004a100/user/common/db",
                0x660, 24 * 1024 * 1024);
   FSAMakeQuota(sAcpData->fsaHandle,
                "/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/db",
                0x660, 24 * 1024 * 1024);
}

static Error
acpSaveStart()
{
   auto error = client::save::start();
   if (error < Error::OK) {
      internal::acpLog->error("acpSaveStart: client::save::start failed with error = {}", error);
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
      internal::acpLog->error("acpSystemStart: FSAOpen failed with error = {}", error);
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

   // Initialise logger
   if (!internal::acpLog) {
      internal::acpLog = decaf::makeLogger("IOS_ACP");
   }

   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticMainServerData();
   internal::initialiseStaticNnsmData();
   internal::initialiseStaticPdmServerData();

   // Initialise nn for current process
   nn::initialiseProcess();

   auto error = IOS_SetThreadPriority(CurrentThread, 50);
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: IOS_SetThreadPriority failed with error = {}", error);
      return error;
   }

   // Initialise process heaps
   error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: IOS_CreateLocalProcessHeap failed with error = {}", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: IOS_CreateCrossProcessHeap failed with error = {}", error);
      return error;
   }

   // Setup /dev/acpproc
   error = IOS_CreateMessageQueue(messages, messages.size());
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: IOS_CreateMessageQueue failed with error = {}", error);
      return error;
   }
   messageQueueId = static_cast<MessageQueueId>(error);

   error = MCP_RegisterResourceManager("/dev/acpproc", messageQueueId);
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: MCP_RegisterResourceManager failed for /dev/acpproc with error = {}", error);
      return error;
   }

   // Start nnsm
   error = internal::startNnsm();
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: startNnsm failed with error = {}", error);
      return error;
   }

   // Start nn::ipc::Server for /dev/acp_main
   error = internal::startMainServer();
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: startMainServer failed with error = {}", error);
      return error;
   }

   // Start nn::ipc::Server for /dev/pdm
   error = internal::startPdmServer();
   if (error < Error::OK) {
      internal::acpLog->error(
         "processEntryPoint: startPdmServer failed with error = {}", error);
      return error;
   }

   // Run acpproc
   while (true) {
      error = IOS_ReceiveMessage(messageQueueId, message, MessageFlags::None);
      if (error < Error::OK) {
         internal::acpLog->error(
            "processEntryPoint: IOS_ReceiveMessage failed with error = {}", error);
         break;
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

   // Uninitialise nn for current process
   nn::uninitialiseProcess();
   return error;
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
