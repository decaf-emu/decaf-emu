#include "ios_auxil.h"
#include "ios_auxil_im_device.h"
#include "ios_auxil_usr_cfg_thread.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

#include "ios/ios_enum.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>
#include <libcpu/be2_struct.h>

namespace ios::auxil
{

constexpr auto LocalHeapSize = 0x20000u;
constexpr auto CrossHeapSize = 0x80000u;

constexpr auto NumMessages = 10u;

struct StaticData
{
   be2_val<kernel::MessageQueueId> messageQueueId;
   be2_array<kernel::Message, NumMessages> messageBuffer;

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
   auto error = kernel::IOS_CreateLocalProcessHeap(phys_addrof(sData->localHeapBuffer),
                                                   sData->localHeapBuffer.size());
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = kernel::IOS_CreateCrossProcessHeap(CrossHeapSize);
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
   error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer), sData->messageBuffer.size());
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to create auxil proc message queue, error = {}.", error);
      return error;
   }
   sData->messageQueueId = static_cast<kernel::MessageQueueId>(error);

   error = kernel::IOS_RegisterResourceManager("/dev/auxilproc", sData->messageQueueId);
   if (error < Error::OK) {
      gLog->error("AUXIL: Failed to register /dev/auxilproc, error = {}.", error);
      return error;
   }

   // Run auxilproc thread
   StackObject<kernel::Message> message;

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(sData->messageQueueId,
                                              message,
                                              kernel::MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      auto request = phys_ptr<kernel::ResourceRequest>(phys_addr { static_cast<kernel::Message>(*message) });
      switch (request->requestData.command) {
      case Command::Suspend:
      {
         // TODO: Stop /dev/im thread
         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Resume:
      {
         // TODO: Start /dev/im thread
         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::SvcMsg:
      {
         if (request->requestData.args.svcMsg.command == 1u) {
            // TODO: "fast relaunch"
         }

         kernel::IOS_ResourceReply(request, Error::OK);
         break;
      }

      default:
         kernel::IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

} // namespace ios::auxil
