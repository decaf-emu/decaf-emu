#include "ios_pad.h"
#include "ios_pad_btrm_device.h"
#include "ios_pad_btrm_request.h"
#include "ios_pad_btrm_response.h"
#include "ios_pad_log.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/mcp/ios_mcp_ipc.h"
#include "ios/ios_stackobject.h"

#include <cstring>

using namespace ios::kernel;
using namespace ios::mcp;

namespace ios::pad::internal
{

struct StaticBtrmData
{
   be2_array<Message, 0x40> messageBuffer;
   be2_val<MessageQueueId> messageQueue;

   be2_array<Message, 0x10> transMessageBuffer;
   be2_val<MessageQueueId> transMessageQueue;

   be2_val<ThreadId> threadId;
   be2_array<uint8_t, 0x4000> threadStack;
};

static phys_ptr<StaticBtrmData> sBtrmData = nullptr;
static phys_ptr<void> sLocalHeapBuffer = nullptr;

static Error
btrmIoctlv(phys_ptr<ResourceRequest> resourceRequest)
{
   if (resourceRequest->requestData.args.ioctlv.numVecIn != 1 ||
       resourceRequest->requestData.args.ioctlv.numVecOut != 1 ||
       !resourceRequest->requestData.args.ioctlv.vecs[0].paddr ||
       resourceRequest->requestData.args.ioctlv.vecs[0].len != sizeof(BtrmRequest) ||
       !resourceRequest->requestData.args.ioctlv.vecs[1].paddr ||
       resourceRequest->requestData.args.ioctlv.vecs[1].len != sizeof(BtrmResponse)) {
      return Error::Invalid;
   }

   auto request = phys_cast<BtrmRequest *>(resourceRequest->requestData.args.ioctlv.vecs[0].paddr);
   auto response = phys_cast<BtrmResponse *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr);
   auto result = Error::OK;

   switch (request->command) {
   case BtrmCommand::PpcInitDone:
      std::memset(phys_addrof(response->ppcInitDone).get(), 0, sizeof(response->ppcInitDone));
      response->ppcInitDone.btChipId = uint8_t { 63 };
      response->ppcInitDone.btChipBuildNumber = uint16_t { 517 };
      result = Error::OK;
      break;
   case BtrmCommand::Wud:
   {
      switch (request->subcommand) {
      case BtrmSubCommand::UpdateBTDevSize:
         result = Error::OK;
         break;
      default:
         result = Error::Invalid;
      }
   }
   default:
      result = Error::Invalid;
   }

   return result;
}

static Error
btrmThreadMain(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sBtrmData->messageQueue,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         auto device = std::string_view { phys_addrof(request->openNameBuffer).get() };
         if (device == "/dev/usb/btrm") {
            IOS_ResourceReply(request, static_cast<Error>(1));
         } else if (device == "/dev/usb/early_btrm") {
            IOS_ResourceReply(request, static_cast<Error>(2));
         } else {
            IOS_ResourceReply(request, static_cast<Error>(3));
         }
         break;
      }
      case Command::Close:
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Ioctlv:
      {
         IOS_ResourceReply(request, btrmIoctlv(request));
         break;
      }
      case Command::Resume:
      case Command::Suspend:
         IOS_ResourceReply(request, Error::OK);
         break;
      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

Error
startBtrmDeviceThread()
{
   auto error = Error::OK;

   error = IOS_CreateMessageQueue(phys_addrof(sBtrmData->messageBuffer),
                                  static_cast<uint32_t>(sBtrmData->messageBuffer.size()));
   if (error < Error::OK) {
      padLog->error("Failed to create message queue, error = {}.", error);
      return error;
   }

   sBtrmData->messageQueue = static_cast<MessageQueueId>(error);

   error = MCP_RegisterResourceManager("/dev/usb/btrm", sBtrmData->messageQueue);
   if (error < Error::OK) {
      padLog->error("Failed to register resource manager for btrm, error = {}.", error);
      return error;
   }

   error = MCP_RegisterResourceManager("/dev/usb/early_btrm", sBtrmData->messageQueue);
   if (error < Error::OK) {
      padLog->error("Failed to register resource manager for early_btrm, error = {}.", error);
      return error;
   }

   error = IOS_AssociateResourceManager("/dev/usb/early_btrm", ResourcePermissionGroup::PAD);
   if (error < Error::OK) {
      padLog->error("Failed to associate resource manager for early_btrm, error = {}.", error);
      return error;
   }

   error = IOS_CreateMessageQueue(phys_addrof(sBtrmData->transMessageBuffer),
                                  static_cast<uint32_t>(sBtrmData->transMessageBuffer.size()));
   if (error < Error::OK) {
      padLog->error("Failed to create message queue, error = {}.", error);
      return error;
   }

   sBtrmData->transMessageQueue = static_cast<MessageQueueId>(error);

   error = IOS_CreateThread(btrmThreadMain,
                            nullptr,
                            phys_addrof(sBtrmData->threadStack) + sBtrmData->threadStack.size(),
                            static_cast<uint32_t>(sBtrmData->threadStack.size()),
                            IOS_GetThreadPriority(IOS_GetCurrentThreadId()),
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      padLog->error("Failed to create btrm thread, error = {}.", error);
      return error;
   }

   sBtrmData->threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sBtrmData->threadId, "BtrmThread");

   error = IOS_StartThread(sBtrmData->threadId);
   if (error < Error::OK) {
      padLog->error("Failed to start btrm thread, error = {}.", error);
      return error;
   }

   return Error::OK;
}

void
initialiseStaticBtrmData()
{
   sBtrmData = allocProcessStatic<StaticBtrmData>();
}

} // namespace ios::pad
