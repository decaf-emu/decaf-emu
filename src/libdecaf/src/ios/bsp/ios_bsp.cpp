#include "ios_bsp.h"
#include "ios_bsp_enum.h"
#include "ios_bsp_bsp_request.h"
#include "ios_bsp_bsp_response.h"
#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>

namespace ios::bsp
{

using namespace kernel;

constexpr auto CrossHeapSize = 0x10000u;

struct StaticBspData
{
   be2_array<Message, 0x40> messageBuffer;
   be2_val<MessageQueueId> messageQueue;
};

static phys_ptr<StaticBspData>
sBspData = nullptr;

static void
bspIoctl(phys_ptr<ResourceRequest> resourceRequest,
         BSPCommand command,
         be2_phys_ptr<const void> inputBuffer,
         be2_phys_ptr<void> outputBuffer)
{
   auto request = phys_cast<const BSPRequest *>(inputBuffer);
   auto response = phys_cast<BSPResponse *>(outputBuffer);
   auto error = Error::OK;

   switch (command) {
   case BSPCommand::GetHardwareVersion:
      response->getHardwareVersion.hardwareVersion = HardwareVersion::LATTE_B1X_CAFE;
      break;
   default:
      error = Error::Invalid;
   }

   IOS_ResourceReply(resourceRequest, error);
}

static void
initialiseStaticBspData()
{
   sBspData = allocProcessStatic<StaticBspData>();
}

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Startup bsp process
   initialiseStaticBspData();

   auto error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("BSP: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // Initialise /dev/bsp
   error = IOS_CreateMessageQueue(phys_addrof(sBspData->messageBuffer),
                                  static_cast<uint32_t>(sBspData->messageBuffer.size()));
   if (error < Error::OK) {
      gLog->error("BSP: Failed to create message queue, error = {}.", error);
      return error;
   }

   sBspData->messageQueue = static_cast<MessageQueueId>(error);

   error = IOS_RegisterResourceManager("/dev/bsp", sBspData->messageQueue);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_AssociateResourceManager("/dev/bsp",
                                        ResourcePermissionGroup::BSP);
   if (error < Error::OK) {
      return error;
   }

   IOS_SetBspReady();

   while (true) {
      StackObject<Message> message;
      auto error = IOS_ReceiveMessage(sBspData->messageQueue,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         IOS_ResourceReply(request, static_cast<Error>(1));
         break;
      }

      case Command::Close:
      {
         IOS_ResourceReply(request, Error::OK);
         break;
      }

      case Command::Ioctl:
      {
         bspIoctl(request,
                  static_cast<BSPCommand>(request->requestData.args.ioctl.request),
                  request->requestData.args.ioctl.inputBuffer,
                  request->requestData.args.ioctl.outputBuffer);
         break;
      }

      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }

   return Error::OK;
}

} // namespace ios::bsp
