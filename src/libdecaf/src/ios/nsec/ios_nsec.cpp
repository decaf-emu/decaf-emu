#include "ios_nsec.h"
#include "ios_nsec_log.h"
#include "ios_nsec_nssl_thread.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/ios_enum.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>
#include <libcpu/be2_struct.h>

using namespace ios::kernel;

namespace ios::nsec
{

constexpr auto LocalHeapSize = 0x140000u;
constexpr auto CrossHeapSize = 0x200000u;
constexpr auto InitThreadStackSize = 0x1000u;
constexpr auto InitThreadPriority = 50u;


struct StaticNsecData
{
   be2_val<BOOL> started;
   be2_array<uint8_t, InitThreadStackSize> threadStack;
   be2_val<ThreadId> initThreadId;
};

static phys_ptr<StaticNsecData>
sNsecData = nullptr;

static phys_ptr<void>
sLocalHeapBuffer = nullptr;

namespace internal
{

Logger nsecLog = { };

static void
initialiseStaticData()
{
   sNsecData = allocProcessStatic<StaticNsecData>();
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

static Error
initThread(phys_ptr<void> /*context*/)
{
   // Some FSA stuff

   // Some cert stuff

   // Start nssl thread
   auto error = startNsslThread();
   if (error < Error::OK) {
      return error;
   }

   // Start nss thread

   return Error::OK;
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> context)
{
   StackArray<Message, 10> messageBuffer;
   StackObject<Message> message;

   // Initialise logger
   internal::nsecLog = decaf::makeLogger("IOS_NSEC");

   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticNsslData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::nsecLog->error("NSEC: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::nsecLog->error("NSEC: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   error = internal::registerNsslResourceManager();
   if (error < Error::OK) {
      internal::nsecLog->error("NSEC: Failed to register NSSL resource manager, error = {}.", error);
      return error;
   }
   // TODO: registerNssResourceManager

   // Setup nsec
   error = IOS_CreateMessageQueue(messageBuffer, messageBuffer.size());
   if (error < Error::OK) {
      internal::nsecLog->error("NSEC: Failed to create nsec proc message queue, error = {}.", error);
      return error;
   }
   auto messageQueueId = static_cast<MessageQueueId>(error);

   error = mcp::MCP_RegisterResourceManager("/dev/nsec", messageQueueId);
   if (error < Error::OK) {
      internal::nsecLog->error("NSEC: Failed to register /dev/nsec, error = {}.", error);
      return error;
   }

   // Run nsec
   while (true) {
      auto error = IOS_ReceiveMessage(messageQueueId,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Suspend:
      {
         if (sNsecData->started) {
            internal::stopNsslThread();
            // internal::stopNssThread();
            sNsecData->started = FALSE;
         }
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Resume:
      {
         if (sNsecData->started) {
            IOS_ResourceReply(request, Error::OK);
         } else {
            auto error =
               IOS_CreateThread(&internal::initThread,
                                nullptr,
                                phys_addrof(sNsecData->threadStack) + sNsecData->threadStack.size(),
                                sNsecData->threadStack.size(),
                                InitThreadPriority,
                                ThreadFlags::Detached);
            if (error >= 0) {
               sNsecData->initThreadId = static_cast<ThreadId>(error);
               error = IOS_StartThread(sNsecData->initThreadId);
            }

            if (error >= 0) {
               IOS_ResourceReply(request, Error::OK);
               sNsecData->started = TRUE;
            } else {
               IOS_ResourceReply(request, Error::Access);
            }
         }
         break;
      }

      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

} // namespace ios::nsec
