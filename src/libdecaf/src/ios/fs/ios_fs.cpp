#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/ios_stackobject.h"

namespace ios
{

struct FsaData
{
   be2_val<kernel::MessageQueueID> fsaMessageQueue;
   be2_array<kernel::Message, 0x160> fsaMessageBuffer;

   be2_val<kernel::ThreadID> fsaThread;
   be2_array<uint8_t, 0x4000> fsaThreadStack;
};

static phys_ptr<FsaData>
sData = nullptr;

Error
fsaThreadMain(phys_ptr<void> /*context*/)
{
   StackObject<kernel::Message> message;

   while (true) {
      auto error = kernel::IOS_ReceiveMessage(sData->fsaMessageQueue,
                                              message,
                                              kernel::MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      auto request = phys_ptr<kernel::ResourceRequest>(phys_addr { static_cast<kernel::Message>(*message) });
      switch (request->requestData.command) {
      case Command::Open:
         break;
      case Command::Close:
         break;
      case Command::Ioctl:
         break;
      case Command::Ioctlv:
         break;
      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

bool
startFsaThread()
{
   auto error = kernel::IOS_CreateMessageQueue(phys_addrof(sData->fsaMessageBuffer),
                                               static_cast<uint32_t>(sData->fsaMessageBuffer.size()));
   if (error < Error::OK) {
      return false;
   }

   auto queueID = static_cast<kernel::MessageQueueID>(error);
   sData->fsaMessageQueue = queueID;

   if (!kernel::IOS_RegisterResourceManager("/dev/fsa", sData->fsaMessageQueue)) {
      return false;
   }

   if (!kernel::IOS_SetResourcePermissionGroup("/dev/fsa", kernel::ResourcePermissionGroup::FS)) {
      return false;
   }

   error = kernel::IOS_CreateThread(fsaThreadMain,
                                    nullptr,
                                    sData->fsaThreadStack.phys_data() + sData->fsaThreadStack.size(),
                                    static_cast<uint32_t>(sData->fsaThreadStack.size()),
                                    78,
                                    kernel::ThreadFlags::Detached);
   if (error < Error::OK) {
      return false;
   }

   auto threadID = static_cast<kernel::ThreadID>(error);
   sData->fsaThread = threadID;
   if (!kernel::IOS_StartThread(sData->fsaThread)) {
      return false;
   }

   return true;
}

void
fs_svc_thread()
{
   /*
   /dev/df
   /dev/atfs
   /dev/isfs
   /dev/wfs
   /dev/pcfs
   /dev/rbfs
   /dev/fat
   /dev/fla
   /dev/ums
   /dev/ahcimgr
   /dev/shdd
   /dev/md
   /dev/scfm
   /dev/mmc
   /dev/timetrace
   /dev/tcp_pcfs
   */
}

} // namespace ios