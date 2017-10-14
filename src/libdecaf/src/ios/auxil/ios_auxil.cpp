#include "ios_auxil.h"
#include "ios_auxil_im_device.h"
#include "ios_auxil_im_thread.h"
#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"

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

namespace ios::auxil
{

constexpr auto LocalHeapSize = 0x20000u;
constexpr auto CrossHeapSize = 0x80000u;

constexpr auto NumMessages = 10u;

struct StaticAuxilData
{
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, NumMessages> messageBuffer;
};

static phys_ptr<StaticAuxilData>
sData = nullptr;

static phys_ptr<void>
sLocalHeapBuffer = nullptr;

namespace internal
{

static void
initialiseStaticData()
{
   sData = phys_cast<StaticAuxilData *>(allocProcessStatic(sizeof(StaticAuxilData)));
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> context)
{
   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticImDeviceData();
   internal::initialiseStaticUsrCfgThreadData();
   internal::initialiseStaticUsrCfgServiceThreadData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // Start usr_cfg threads
   error = internal::startUsrCfgServiceThread();
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to start usr_cfg service thread, error = {}.", error);
      return error;
   }

   error = internal::startUsrCfgThread();
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to start usr_cfg thread, error = {}.", error);
      return error;
   }

   // Setup auxilproc
   error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                  static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to create auxil proc message queue, error = {}.", error);
      return error;
   }
   sData->messageQueueId = static_cast<MessageQueueId>(error);

   error = mcp::MCP_RegisterResourceManager("/dev/auxilproc", sData->messageQueueId);
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to register /dev/auxilproc, error = {}.", error);
      return error;
   }

   // Run auxilproc thread
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
      case Command::Suspend:
      {
         internal::stopImThread();
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Resume:
      {
         internal::startImThread();
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::SvcMsg:
      {
         if (request->requestData.args.svcMsg.command == 1u) {
            // TODO: "fast relaunch"
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      }

      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

} // namespace ios::auxil
