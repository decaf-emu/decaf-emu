#include "ios_fs_fsa_async_task.h"
#include "ios_fs_fsa_device.h"
#include "ios_fs_mutex.h"

#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_enum.h"
#include "ios/ios_handlemanager.h"
#include "ios/ios_ipc.h"
#include "ios/ios_stackobject.h"
#include "ios/ios_worker_thread.h"

using namespace ios::kernel;
using ios::internal::submitWorkerTask;

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

static void
fsaDeviceIoctl(phys_ptr<ResourceRequest> resourceRequest,
               FSACommand command,
               be2_phys_ptr<const void> inputBuffer,
               be2_phys_ptr<void> outputBuffer)
{
   FSADevice *device = nullptr;
   auto status = getDevice(resourceRequest->requestData.handle, &device);
   if (status < FSAStatus::OK) {
      IOS_ResourceReply(resourceRequest, static_cast<Error>(status));
      return;
   }

   auto request = phys_cast<const FSARequest *>(inputBuffer);
   auto response = phys_cast<FSAResponse *>(outputBuffer);

   if (!device) {
     IOS_ResourceReply(resourceRequest,
                       static_cast<Error>(FSAStatus::InvalidClientHandle));
     return;
   }

   if (!inputBuffer && !outputBuffer) {
     IOS_ResourceReply(resourceRequest,
                       static_cast<Error>(FSAStatus::InvalidParam));
     return;
   }

   if (request->emulatedError < 0) {
      IOS_ResourceReply(resourceRequest,
                        static_cast<Error>(request->emulatedError));
      return;
   }

   auto user = vfs::User { };
   user.id = static_cast<vfs::OwnerId>(resourceRequest->requestData.titleId & 0xFFFFFFFF);
   user.group = static_cast<vfs::GroupId>(resourceRequest->requestData.groupId);

   switch (command) {
   case FSACommand::ChangeDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->changeDir(user, phys_addrof(request->changeDir)));
         });
      break;
   case FSACommand::CloseDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->closeDir(user, phys_addrof(request->closeDir)));
         });
      break;
   case FSACommand::CloseFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->closeFile(user, phys_addrof(request->closeFile)));
         });
      break;
   case FSACommand::FlushFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->flushFile(user, phys_addrof(request->flushFile)));
         });
      break;
   case FSACommand::FlushQuota:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->flushQuota(user, phys_addrof(request->flushQuota)));
         });
      break;
   case FSACommand::GetCwd:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->getCwd(user, phys_addrof(response->getCwd)));
         });
      break;
   case FSACommand::GetInfoByQuery:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->getInfoByQuery(user,
                                      phys_addrof(request->getInfoByQuery),
                                      phys_addrof(response->getInfoByQuery)));
         });
      break;
   case FSACommand::GetPosFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->getPosFile(user,
                                  phys_addrof(request->getPosFile),
                                  phys_addrof(response->getPosFile)));
         });
      break;
   case FSACommand::IsEof:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->isEof(user, phys_addrof(request->isEof)));
         });
      break;
   case FSACommand::MakeDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->makeDir(user, phys_addrof(request->makeDir)));
         });
      break;
   case FSACommand::MakeQuota:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->makeQuota(user, phys_addrof(request->makeQuota)));
         });
      break;
   case FSACommand::OpenDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->openDir(user,
                               phys_addrof(request->openDir),
                               phys_addrof(response->openDir)));
         });
      break;
   case FSACommand::OpenFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->openFile(user,
                                phys_addrof(request->openFile),
                                phys_addrof(response->openFile)));
         });
      break;
   case FSACommand::ReadDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->readDir(user,
                               phys_addrof(request->readDir),
                               phys_addrof(response->readDir)));
         });
      break;
   case FSACommand::Remove:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->remove(user, phys_addrof(request->remove)));
         });
      break;
   case FSACommand::Rename:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->rename(user, phys_addrof(request->rename)));
         });
      break;
   case FSACommand::RewindDir:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->rewindDir(user, phys_addrof(request->rewindDir)));
         });
      break;
   case FSACommand::SetPosFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->setPosFile(user, phys_addrof(request->setPosFile)));
         });
      break;
   case FSACommand::StatFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->statFile(user,
                                phys_addrof(request->statFile),
                                phys_addrof(response->statFile)));
         });
      break;
   case FSACommand::TruncateFile:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->truncateFile(user, phys_addrof(request->truncateFile)));
         });
      break;
   case FSACommand::Unmount:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->unmount(user, phys_addrof(request->unmount)));
         });
      break;
   case FSACommand::UnmountWithProcess:
      submitWorkerTask([=]() {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->unmountWithProcess(user,
                                          phys_addrof(request->unmountWithProcess)));
         });
      break;
   default:
      IOS_ResourceReply(resourceRequest,
                        static_cast<Error>(FSAStatus::UnsupportedCmd));
   }
}

static void
fsaDeviceIoctlv(phys_ptr<ResourceRequest> resourceRequest,
                FSACommand command,
                be2_phys_ptr<IoctlVec> vecs)
{
   auto request = phys_cast<FSARequest *>(vecs[0].paddr);

   if (request->emulatedError < FSAStatus::OK) {
      IOS_ResourceReply(resourceRequest,
                        static_cast<Error>(request->emulatedError));
      return;
   }

   FSADevice *device = nullptr;
   auto status = getDevice(resourceRequest->requestData.handle, &device);
   if (status < FSAStatus::OK) {
      IOS_ResourceReply(resourceRequest, static_cast<Error>(status));
      return;
   }

   auto user = vfs::User{ };
   user.id = static_cast<vfs::OwnerId>(resourceRequest->requestData.titleId & 0xFFFFFFFF);
   user.group = static_cast<vfs::GroupId>(resourceRequest->requestData.groupId);

   switch (command) {
   case FSACommand::ReadFile:
   {
      submitWorkerTask(
         [=]() {
            auto buffer = phys_cast<uint8_t *>(vecs[1].paddr);
            auto length = vecs[1].len;

            fsaAsyncTaskComplete(
               resourceRequest,
               device->readFile(user, phys_addrof(request->readFile),
                                buffer, length));
         });
      break;
   }
   case FSACommand::WriteFile:
   {
      submitWorkerTask(
         [=]()
         {
            auto buffer = phys_cast<uint8_t *>(vecs[1].paddr);
            auto length = vecs[1].len;

            fsaAsyncTaskComplete(
               resourceRequest,
               device->writeFile(user, phys_addrof(request->writeFile),
                                 buffer, length));
         });
      break;
   }
   case FSACommand::Mount:
   {
      submitWorkerTask(
         [=]()
         {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->mount(user, phys_addrof(request->mount)));
         });
      break;
   }
   case FSACommand::MountWithProcess:
   {
      submitWorkerTask(
         [=]()
         {
            fsaAsyncTaskComplete(
               resourceRequest,
               device->mountWithProcess(user, phys_addrof(request->mountWithProcess)));
         });
      break;
   }
   default:
      IOS_ResourceReply(resourceRequest,
                        static_cast<Error>(FSAStatus::UnsupportedCmd));
   }
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

   sData->fsaThread = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(sData->fsaThread, "FsaThread");

   error = IOS_StartThread(sData->fsaThread);
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

void
initialiseStaticFsaThreadData()
{
   sData = allocProcessStatic<StaticFsaThreadData>();
   sDevices.closeAll();
}

} // namespace ios::fs::internal
