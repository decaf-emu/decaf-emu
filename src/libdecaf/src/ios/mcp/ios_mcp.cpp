#include "ios_mcp.h"
#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_thread.h"
#include "ios_mcp_pm_thread.h"
#include "ios_mcp_ppc_thread.h"
#include "ios_mcp_title.h"

#include "ios/kernel/ios_kernel_debug.h"
#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

#include "ios/ios_stackobject.h"

#include <common/log.h>

using namespace ios::kernel;

namespace ios::mcp
{

constexpr auto LocalHeapSize = 0x8000u;
constexpr auto CrossHeapSize = 0x31E000u;

constexpr auto MainThreadNumMessages = 30u;

struct StaticMcpData
{
   be2_val<uint32_t> bootFlags;
   be2_val<uint32_t> systemModeFlags;
   be2_val<SystemFileSys> systemFileSys;
   be2_struct<IpcRequest> sysEventMsg;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, MainThreadNumMessages> messageBuffer;
};

static phys_ptr<StaticMcpData>
sData = nullptr;

static phys_ptr<void>
sLocalHeapBuffer = nullptr;

namespace internal
{

static void
initialiseStaticData()
{
   sData = phys_cast<StaticMcpData *>(allocProcessStatic(sizeof(StaticMcpData)));
   sData->sysEventMsg.command = static_cast<Command>(MainThreadCommand::SysEvent);

   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

static void
initialiseClientCaps()
{
   StackObject<uint64_t> mask;

   struct
   {
      ProcessId pid;
      FeatureId fid;
      uint64_t mask;
   } caps[] = {
      { ProcessId::MCP,    0x7FFFFFFF, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::CRYPTO,          1,               0xFF },
      { ProcessId::USB,             1,                0xF },
      { ProcessId::USB,           0xC, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,             9, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,           0xB,          0x3300300 },
      { ProcessId::FS,              1,                0xF },
      { ProcessId::FS,              3,                  3 },
      { ProcessId::FS,              9,                  1 },
      { ProcessId::FS,            0xB, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::FS,              2, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::FS,            0xC,                  1 },
      { ProcessId::PAD,           0xB,           0x101000 },
      { ProcessId::PAD,             1,                0xF },
      { ProcessId::PAD,           0xD,                  1 },
      { ProcessId::PAD,          0x18, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::NET,             1,                0xF },
      { ProcessId::NET,             8,                  1 },
      { ProcessId::NET,           0xC,                  1 },
      { ProcessId::NET,          0x1A, 0xFFFFFFFFFFFFFFFF },
   };

   for (auto &cap : caps) {
      *mask = cap.mask;
      IOS_SetClientCapabilities(cap.pid, cap.fid, mask);
   }
}

static Error
mainThreadLoop()
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sData->messageQueueId, message, MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      /* TODO: Handle commands
      switch (request->requestData.command) {
      default:
      }
      */
      IOS_ResourceReply(request, Error::InvalidArg);
   }
}

uint32_t
getBootFlags()
{
   return sData->bootFlags;
}

uint32_t
getSystemModeFlags()
{
   return sData->systemModeFlags;
}

SystemFileSys
getSystemFileSys()
{
   return sData->systemFileSys;
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Initialise process static data
   internal::initialiseStaticData();
   internal::initialiseStaticConfigData();
   internal::initialiseStaticMcpThreadData();
   internal::initialiseStaticPmThreadData();
   internal::initialiseStaticPpcThreadData();
   internal::initialiseTitleStaticData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      gLog->error("Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // Set normal system flags
   sData->bootFlags = 0xC200000u;
   sData->systemModeFlags = 0x100000u;
   sData->systemFileSys = SystemFileSys::Nand;
   IOS_SetSecurityLevel(SecurityLevel::Normal);

   // Start pm thread
   error = internal::startPmThread();
   if (error < Error::OK) {
      gLog->error("Failed to start pm thread, error = {}.", error);
      return error;
   }

   // Create main thread message queue
   error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                  sData->messageBuffer.size());
   if (error < Error::OK) {
      gLog->error("Failed to create main thread message buffer, error = {}.", error);
      return error;
   }

   sData->messageQueueId = static_cast<MessageQueueId>(error);

   // Set process client caps
   internal::initialiseClientCaps();

   // Stat mcp thread
   internal::startMcpThread();

   // Start PPC thread
   internal::startPpcThread();

   // Register main thread as SysEvent handler
   error = IOS_HandleEvent(DeviceId::SysEvent,
                           sData->messageQueueId,
                           makeMessage(phys_addrof(sData->sysEventMsg)));
   if (error < Error::OK) {
      gLog->error("Failed to register SysEvent event handler, error = {}.", error);
      return error;
   }

   // Handle all pending resource manager registrations
   error = internal::handleResourceManagerRegistrations(sData->systemModeFlags,
                                                        sData->bootFlags);
   if (error < Error::OK) {
      gLog->error("Failed to handle resource manager registrations, error = {}.", error);
      return error;
   }

   return internal::mainThreadLoop();
}

} // namespace ios::mcp
