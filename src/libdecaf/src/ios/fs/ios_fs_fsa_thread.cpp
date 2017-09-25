#include "ios_fs_fsa.h"
#include "ios_fs_mutex.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include "ios/ios_stackobject.h"

namespace ios::fs
{

using FSADeviceHandle = int32_t;
constexpr auto FSAMaxClients = 0x270;

struct RequestOrigin
{
   be2_val<TitleId> titleId;
   be2_val<ProcessId> processId;
   be2_val<GroupId> groupId;
};

struct FsaPerProcessData
{
   be2_val<BOOL> initialised;
   be2_struct<RequestOrigin> origin;
   UNKNOWN(4);
   be2_val<kernel::ClientCapabilityMask> clientCapabilityMask;
   UNKNOWN(0x34 - 0x20);
   be2_val<uint32_t> numFilesOpen;
   UNKNOWN(0x4538 - 0x38);
   be2_array<char, FSAPathLength> unkPath0x4538;
   be2_array<char, FSAPathLength> unkPath0x47B8;
   be2_struct<Mutex> mutex;
};

struct FsaDeviceData
{
   be2_val<BOOL> initialised;
   be2_phys_ptr<FsaPerProcessData> perProcessData;
   UNKNOWN(0xAC - 0x08);
   be2_array<char, FSAPathLength> workingPath;
   UNKNOWN(0x4);
};

struct FsaData
{
   be2_val<kernel::MessageQueueId> fsaMessageQueue;
   be2_array<kernel::Message, 0x160> fsaMessageBuffer;

   be2_val<kernel::ThreadId> fsaThread;
   be2_array<uint8_t, 0x4000> fsaThreadStack;
};

static phys_ptr<FsaData>
sData = nullptr;

static std::vector<std::unique_ptr<FSADevice>>
sDevices;

FSADevice *
getDevice(FSADeviceHandle handle)
{
   if (handle < 0 || handle >= sDevices.size()) {
      return nullptr;
   }

   return sDevices[handle].get();
}

static FSAStatus
fsaDeviceOpen(phys_ptr<RequestOrigin> origin,
              FSADeviceHandle *outFsaHandle,
              kernel::ClientCapabilityMask clientCapabilityMask)
{
   auto handle = FSADeviceHandle { -1 };

   for (auto i = 0u; i < sDevices.size(); ++i) {
      if (!sDevices[i]) {
         handle = static_cast<FSADeviceHandle>(i);
         break;
      }
   }

   if (handle < 0) {
      if (sDevices.size() >= FSAMaxClients) {
         return FSAStatus::MaxClients;
      }

      handle = static_cast<FSADeviceHandle>(sDevices.size());
      sDevices.emplace_back(std::make_unique<FSADevice>());
   } else {
      sDevices[handle] = std::make_unique<FSADevice>();
   }

   *outFsaHandle = handle;
   return FSAStatus::OK;
}

static FSAStatus
fsaDeviceIoctl(phys_ptr<kernel::ResourceRequest> resourceRequest,
               FSACommand command,
               be2_phys_ptr<const void> inputBuffer,
               be2_phys_ptr<void> outputBuffer)
{
   auto status = FSAStatus::OK;
   auto device = getDevice(resourceRequest->requestData.handle);
   auto request = phys_cast<const FSARequest>(inputBuffer);
   auto response = phys_cast<FSAResponse>(outputBuffer);

   if (!device) {
      status = FSAStatus::InvalidClientHandle;
   } else if (!inputBuffer && !outputBuffer) {
      status = FSAStatus::InvalidParam;
   } else {
      switch (command) {
      case FSACommand::GetCwd:
         status = device->getCwd(response->getCwd);
         break;
      default:

      }
   }

   kernel::IOS_ResourceReply(resourceRequest, static_cast<Error>(status));
}

Error
fsaThreadMain(phys_ptr<void> /*context*/)
{
   StackObject<kernel::Message> message;
   StackObject<RequestOrigin> origin;

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
      {
         origin->titleId = request->requestData.titleId;
         origin->processId = request->requestData.processId;
         origin->groupId = request->requestData.groupId;

         FSADeviceHandle fsaHandle;
         auto error = fsaDeviceOpen(origin,
                                    &fsaHandle,
                                    request->requestData.args.open.caps);
         if (error >= FSAStatus::OK) {
            error = static_cast<FSAStatus>(fsaHandle);
         }

         kernel::IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Close:
      {
         // Close an FSADevice
         break;
      }

      case Command::Ioctl:
      {
         auto error = fsaDeviceIoctl(request,
                                     static_cast<FSACommand>(request->requestData.args.ioctl.request.value()),
                                     request->requestData.args.ioctl.inputBuffer,
                                     request->requestData.args.ioctl.outputBuffer);
         break;
      }

      case Command::Ioctlv:
      {
         // FSADevice::ioctlv
         break;
      }

      default:
         kernel::IOS_ResourceReply(request, Error::Invalid);
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

   auto queueId = static_cast<kernel::MessageQueueId>(error);
   sData->fsaMessageQueue = queueId;

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

   auto threadId = static_cast<kernel::ThreadId>(error);
   sData->fsaThread = threadId;
   if (!kernel::IOS_StartThread(sData->fsaThread)) {
      return false;
   }

   return true;
}

} // namespace ios::fs
