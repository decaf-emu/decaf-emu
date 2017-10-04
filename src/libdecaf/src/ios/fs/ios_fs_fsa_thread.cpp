#include "ios_fs_fsa_device.h"
#include "ios_fs_mutex.h"

#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_enum.h"
#include "ios/ios_handlemanager.h"
#include "ios/ios_ipc.h"
#include "ios/ios_stackobject.h"

using namespace ios::kernel;

namespace ios::fs::internal
{

using FSADeviceHandle = int32_t;
constexpr auto FSAMaxClients = 0x270;

struct RequestOrigin
{
   be2_val<TitleId> titleId;
   be2_val<ProcessId> processId;
   be2_val<GroupId> groupId;
};

struct StaticFsaThreadData
{
   be2_val<MessageQueueId> fsaMessageQueue;
   be2_array<Message, 0x160> fsaMessageBuffer;

   be2_val<ThreadId> fsaThread;
   be2_array<uint8_t, 0x4000> fsaThreadStack;
};

static phys_ptr<StaticFsaThreadData>
sData = nullptr;

static HandleManager<FSADevice, FSADeviceHandle, FSAMaxClients>
sDevices;

FSAStatus
getDevice(FSADeviceHandle handle,
          FSADevice **outDevice)
{
   auto error = sDevices.get(handle, outDevice);
   if (error < Error::OK) {
      return FSAStatus::InvalidClientHandle;
   }

   return FSAStatus::OK;
}

static FSAStatus
fsaDeviceOpen(phys_ptr<RequestOrigin> origin,
              FSADeviceHandle *outHandle,
              ClientCapabilityMask clientCapabilityMask)
{
   auto error = sDevices.open();
   if(error < Error::OK) {
      return FSAStatus::MaxClients;
   }

   *outHandle = static_cast<FSADeviceHandle>(error);
   return FSAStatus::OK;
}

static FSAStatus
fsaDeviceClose(FSADeviceHandle handle,
               phys_ptr<ResourceRequest> resourceRequest)
{
   FSADevice *device = nullptr;
   auto status = getDevice(handle, &device);
   if (status < FSAStatus::OK) {
      return status;
   }

   // TODO: Handle cleanup of async operations

   sDevices.close(device);
   return FSAStatus::OK;
}

static FSAStatus
fsaDeviceIoctl(phys_ptr<ResourceRequest> resourceRequest,
               FSACommand command,
               be2_phys_ptr<const void> inputBuffer,
               be2_phys_ptr<void> outputBuffer)
{
   FSADevice *device = nullptr;
   auto status = getDevice(resourceRequest->requestData.handle, &device);
   if (status < FSAStatus::OK) {
      return status;
   }

   auto request = phys_cast<const FSARequest>(inputBuffer);
   auto response = phys_cast<FSAResponse>(outputBuffer);

   if (!device) {
     status = FSAStatus::InvalidClientHandle;
   } else if (!inputBuffer && !outputBuffer) {
     status = FSAStatus::InvalidParam;
   } else if (request->emulatedError < 0) {
      return request->emulatedError;
   } else {
      switch (command) {
      case FSACommand::ChangeDir:
         status = device->changeDir(phys_addrof(request->changeDir));
         break;
      case FSACommand::CloseDir:
         status = device->closeDir(phys_addrof(request->closeDir));
         break;
      case FSACommand::CloseFile:
         status = device->closeFile(phys_addrof(request->closeFile));
         break;
      case FSACommand::FlushFile:
         status = device->flushFile(phys_addrof(request->flushFile));
         break;
      case FSACommand::FlushQuota:
         status = device->flushQuota(phys_addrof(request->flushQuota));
         break;
      case FSACommand::GetCwd:
         status = device->getCwd(phys_addrof(response->getCwd));
         break;
      case FSACommand::GetInfoByQuery:
         status = device->getInfoByQuery(phys_addrof(request->getInfoByQuery), phys_addrof(response->getInfoByQuery));
         break;
      case FSACommand::GetPosFile:
         status = device->getPosFile(phys_addrof(request->getPosFile), phys_addrof(response->getPosFile));
         break;
      case FSACommand::IsEof:
         status = device->isEof(phys_addrof(request->isEof));
         break;
      case FSACommand::MakeDir:
         status = device->makeDir(phys_addrof(request->makeDir));
         break;
      case FSACommand::OpenDir:
         status = device->openDir(phys_addrof(request->openDir), phys_addrof(response->openDir));
         break;
      case FSACommand::OpenFile:
         status = device->openFile(phys_addrof(request->openFile), phys_addrof(response->openFile));
         break;
      case FSACommand::ReadDir:
         status = device->readDir(phys_addrof(request->readDir), phys_addrof(response->readDir));
         break;
      case FSACommand::Remove:
         status = device->remove(phys_addrof(request->remove));
         break;
      case FSACommand::Rename:
         status = device->rename(phys_addrof(request->rename));
         break;
      case FSACommand::RewindDir:
         status = device->rewindDir(phys_addrof(request->rewindDir));
         break;
      case FSACommand::SetPosFile:
         status = device->setPosFile(phys_addrof(request->setPosFile));
         break;
      case FSACommand::StatFile:
         status = device->statFile(phys_addrof(request->statFile), phys_addrof(response->statFile));
         break;
      case FSACommand::TruncateFile:
         status = device->truncateFile(phys_addrof(request->truncateFile));
         break;
      default:
         status = FSAStatus::UnsupportedCmd;
      }
   }

   // TODO: Move this reply once we do asynchronous filesystem commands
   IOS_ResourceReply(resourceRequest, static_cast<Error>(status));
   return status;
}

static FSAStatus
fsaDeviceIoctlv(phys_ptr<ResourceRequest> resourceRequest,
                FSACommand command,
                be2_phys_ptr<IoctlVec> vecs)
{
   auto request = phys_ptr<FSARequest> { vecs[0].paddr };
   auto status = FSAStatus::OK;

   if (request->emulatedError < FSAStatus::OK) {
      status = request->emulatedError;
   } else {
      FSADevice *device = nullptr;
      status = getDevice(resourceRequest->requestData.handle, &device);

      if (status >= FSAStatus::OK) {
         switch (command) {
         case FSACommand::ReadFile:
         {
            auto buffer = phys_ptr<uint8_t> { vecs[1].paddr };
            auto length = vecs[1].len;
            status = device->readFile(phys_addrof(request->readFile), buffer, length);
            break;
         }
         case FSACommand::WriteFile:
         {
            auto buffer = phys_ptr<uint8_t> { vecs[1].paddr };
            auto length = vecs[1].len;
            status = device->writeFile(phys_addrof(request->writeFile), buffer, length);
            break;
         }
         case FSACommand::Mount:
         {
            status = device->mount(phys_addrof(request->mount));
            break;
         }
         default:
            status = FSAStatus::UnsupportedCmd;
         }
      }
   }

   // TODO: Move this reply once we do asynchronous filesystem commands
   IOS_ResourceReply(resourceRequest, static_cast<Error>(status));
   return status;
}

static Error
fsaThreadMain(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;
   StackObject<RequestOrigin> origin;

   while (true) {
      auto error = IOS_ReceiveMessage(sData->fsaMessageQueue,
                                      message,
                                      MessageFlags::None);
      if (error < Error::OK) {
         return error;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      {
         FSADeviceHandle fsaHandle;
         origin->titleId = request->requestData.titleId;
         origin->processId = request->requestData.processId;
         origin->groupId = request->requestData.groupId;

         auto error = fsaDeviceOpen(origin,
                                    &fsaHandle,
                                    request->requestData.args.open.caps);
         if (error >= FSAStatus::OK) {
            error = static_cast<FSAStatus>(fsaHandle);
         }

         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Close:
      {
         auto error = fsaDeviceClose(request->requestData.handle, request);
         IOS_ResourceReply(request, static_cast<Error>(error));
         break;
      }

      case Command::Ioctl:
      {
         fsaDeviceIoctl(request,
                        static_cast<FSACommand>(request->requestData.args.ioctl.request),
                        request->requestData.args.ioctl.inputBuffer,
                        request->requestData.args.ioctl.outputBuffer);
         break;
      }

      case Command::Ioctlv:
      {
         fsaDeviceIoctlv(request,
                         static_cast<FSACommand>(request->requestData.args.ioctlv.request),
                         request->requestData.args.ioctlv.vecs);
         break;
      }

      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
}

Error
startFsaThread()
{
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->fsaMessageBuffer),
                                       static_cast<uint32_t>(sData->fsaMessageBuffer.size()));
   if (error < Error::OK) {
      return error;
   }

   auto queueId = static_cast<MessageQueueId>(error);
   sData->fsaMessageQueue = queueId;

   error = IOS_RegisterResourceManager("/dev/fsa", sData->fsaMessageQueue);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_AssociateResourceManager("/dev/fsa", ResourcePermissionGroup::FS);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_CreateThread(fsaThreadMain,
                            nullptr,
                            phys_addrof(sData->fsaThreadStack) + sData->fsaThreadStack.size(),
                            static_cast<uint32_t>(sData->fsaThreadStack.size()),
                            78,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   auto threadId = static_cast<ThreadId>(error);
   sData->fsaThread = threadId;

   error = IOS_StartThread(sData->fsaThread);
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

void
initialiseStaticFsaThreadData()
{
   sData = phys_cast<StaticFsaThreadData>(allocProcessStatic(sizeof(StaticFsaThreadData)));
   sDevices.closeAll();
}

} // namespace ios::fs::internal
