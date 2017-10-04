#include "ios_mcp_enum.h"
#include "ios_mcp_pm_thread.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_timer.h"

#include "ios/ios_stackobject.h"

#include <array>
#include <common/log.h>
#include <memory>

namespace ios::mcp::internal
{

using namespace kernel;

using RegisteredResourceManagerId = int32_t;

constexpr auto MaxNumRmQueueMessages = 0x80u;
constexpr auto MaxNumResourceManagers = 0x56u;

constexpr auto PmThreadStackSize = 0x2000u;
constexpr auto PmThreadPriority = 123u;

struct ResourceManagerRegistration
{
   struct Data // Why another struct? Who knows...
   {
      be2_val<BOOL> isDummyRM;
      be2_val<ResourceHandleId> resourceHandle;
      be2_val<Error> error;
      be2_phys_ptr<IpcRequest> messageBuffer;
      be2_val<ResourceManagerRegistrationState> state;
      be2_val<uint32_t> systemModeFlags;
      be2_val<ProcessId> processId;
      be2_val<uint32_t> unk0x1C;
      be2_val<TimeMicroseconds64> timeResumeStart;
      be2_val<TimeMicroseconds64> timeResumeFinished;
      be2_val<TimeMicroseconds64> timeSuspendStart;
      be2_val<TimeMicroseconds64> timeSuspendFinished;
      be2_val<TimeMicroseconds64> timeOpenStart;
      be2_val<TimeMicroseconds64> timeOpenFinished;
   };

   be2_phys_ptr<char> name;
   be2_val<uint32_t> unk0x04;
   Data data;
};
CHECK_OFFSET(ResourceManagerRegistration, 0x00, name);
CHECK_OFFSET(ResourceManagerRegistration, 0x04, unk0x04);
CHECK_OFFSET(ResourceManagerRegistration, 0x08, data);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x00, isDummyRM);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x04, resourceHandle);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x08, error);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x0C, messageBuffer);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x10, state);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x14, systemModeFlags);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x18, processId);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x1C, unk0x1C);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x20, timeResumeStart);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x28, timeResumeFinished);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x30, timeSuspendStart);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x38, timeSuspendFinished);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x40, timeOpenStart);
CHECK_OFFSET(ResourceManagerRegistration::Data, 0x48, timeOpenFinished);
CHECK_SIZE(ResourceManagerRegistration::Data, 0x50);
CHECK_SIZE(ResourceManagerRegistration, 0x58);

struct StaticPmThreadData
{
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, PmThreadStackSize> threadStack;

   be2_array<ResourceManagerRegistration, MaxNumResourceManagers> resourceManagers;

   be2_val<MessageQueueId> resourceManagerMessageQueueId;
   be2_val<TimerId> resourceManagerTimerId;
   be2_struct<IpcRequest> resourceManagerTimeoutMessage;
   be2_array<Message, MaxNumRmQueueMessages> resourceManagerMessageBuffer;
};

static phys_ptr<StaticPmThreadData>
sData;

static Error
getResourceManagerId(std::string_view name)
{
   for (auto i = 0u; i < sData->resourceManagers.size(); ++i) {
      auto &resourceManager = sData->resourceManagers[i];
      if (resourceManager.data.isDummyRM || !resourceManager.name) {
         continue;
      }

      if (name.compare(resourceManager.name.getRawPointer()) == 0) {
         return static_cast<Error>(i);
      }
   }

   return Error::NoResource;
}

static Error
sendRegisterResourceManagerMessage(RegisteredResourceManagerId id)
{
   if (id < 0 || id >= sData->resourceManagers.size()) {
      return Error::InvalidArg;
   }

   auto &resourceManager = sData->resourceManagers[id];
   if (resourceManager.data.isDummyRM) {
      return Error::InvalidArg;
   }

   if (resourceManager.data.state != ResourceManagerRegistrationState::NotRegistered) {
      return Error::InvalidArg;
   }

   resourceManager.data.messageBuffer->command = static_cast<Command>(ResourceManagerCommand::Register);
   resourceManager.data.messageBuffer->handle = id;

   return IOS_SendMessage(sData->resourceManagerMessageQueueId,
                          makeMessage(resourceManager.data.messageBuffer),
                          MessageFlags::None);
}

static Error
pmIoctl(PMCommand command,
        phys_ptr<const void> inputBuffer,
        uint32_t inputLength,
        phys_ptr<void> outputBuffer,
        uint32_t outputLength)
{
   auto error = Error::OK;

   switch (command) {
   case PMCommand::GetResourceManagerId:
      error = getResourceManagerId(phys_cast<const char>(inputBuffer).getRawPointer());
      break;
   case PMCommand::RegisterResourceManager:
      error = sendRegisterResourceManagerMessage(*phys_cast<const RegisteredResourceManagerId>(inputBuffer));
      break;
   default:
      error = Error::InvalidArg;
   }

   return error;
}

static Error
pmThreadEntry(phys_ptr<void> /*context*/)
{
   StackArray<Message, 0x80u> messageBuffer;
   StackObject<Message> message;

   // Create message queue
   auto error = IOS_CreateMessageQueue(messageBuffer, 0x80u);
   if (error < Error::OK) {
      return error;
   }
   auto messageQueueId = static_cast<MessageQueueId>(error);

   error = IOS_RegisterResourceManager("/dev/pm", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(messageQueueId,
                                 message,
                                 MessageFlags::None);
      if (error < Error::OK) {
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
      case Command::Close:
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Ioctl:
      {
         error = pmIoctl(static_cast<PMCommand>(request->requestData.args.ioctl.request),
                         request->requestData.args.ioctl.inputBuffer,
                         request->requestData.args.ioctl.inputLength,
                         request->requestData.args.ioctl.outputBuffer,
                         request->requestData.args.ioctl.outputLength);
         break;
      }
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   return error;
}

Error
registerResourceManager(std::string_view device,
                        MessageQueueId queue)
{
   auto error = getResourceManagerId(device);
   if (error < Error::OK) {
      return error;
   }

   auto resourceManagerId = static_cast<RegisteredResourceManagerId>(error);
   error = IOS_RegisterResourceManager(device, queue);
   if (error < Error::OK) {
      return error;
   }

   return sendRegisterResourceManagerMessage(resourceManagerId);
}

Error
startPmThread()
{
   // Create resource manager message queue
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->resourceManagerMessageBuffer),
                                       sData->resourceManagerMessageBuffer.size());
   if (error < Error::OK) {
      return error;
   }

   sData->resourceManagerMessageQueueId = static_cast<MessageQueueId>(error);

   // Create resource manager timeout timer
   error = IOS_CreateTimer(std::chrono::microseconds { 0 },
                           std::chrono::microseconds { 0 },
                           sData->resourceManagerMessageQueueId,
                           makeMessage(phys_addrof(sData->resourceManagerTimeoutMessage)));
   if (error < Error::OK) {
      return error;
   }

   sData->resourceManagerTimerId = static_cast<TimerId>(error);

   // Initialise resource manager registrations
   for (auto i = 0u; i < sData->resourceManagers.size(); ++i) {
      auto &rm = sData->resourceManagers[i];
      rm.data.state = ResourceManagerRegistrationState::NotRegistered;
      rm.data.resourceHandle = static_cast<ResourceHandleId>(Error::Invalid);
      rm.data.error = Error::Invalid;

      if (!rm.data.isDummyRM) {
         auto buffer = IOS_HeapAlloc(CrossProcessHeapId,
                                     static_cast<uint32_t>(sizeof(IpcRequest)));
         if (!buffer) {
            return error;
         }

         rm.data.messageBuffer = phys_cast<IpcRequest>(buffer);
      }
   }

   // Create thread
   error = IOS_CreateThread(&pmThreadEntry, nullptr,
                            phys_addrof(sData->threadStack) + sData->threadStack.size(),
                            static_cast<uint32_t>(sData->threadStack.size()),
                            PmThreadPriority,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   sData->threadId = static_cast<ThreadId>(error);
   return IOS_StartThread(sData->threadId);
}

static Error
waitAsyncReplyWithTimeout(MessageQueueId queue,
                          phys_ptr<Message> message,
                          TimeMicroseconds32 timeout)
{
   if (timeout == -1) {
      return IOS_ReceiveMessage(queue, message, MessageFlags::None);
   }

   auto error = IOS_RestartTimer(sData->resourceManagerTimerId,
                                 std::chrono::microseconds { timeout },
                                 std::chrono::microseconds { 0 });
   if (error < Error::OK) {
      return error;
   }

   error = IOS_ReceiveMessage(queue, message, MessageFlags::None);
   IOS_StopTimer(sData->resourceManagerTimerId);
   return error;
}

/**
 * Handle all pending resource manager registrations.
 *
 * This code is needlessly complex, thanks Nint*ndo. I think I know what you
 * were trying to do - not overfill the message queue with pending messages
 * whilst at the same time trying to have multiple devices registering
 * asynchronously. Still - it's not great a way to do that is it.
 *
 * A basic summary is that we try to transition states for the RM as:
 * NotRegistered -> Registered -> Pending -> Resumed.
 *
 * The non-trivial logic is to handle all incoming messages on the message
 * queue whilst at the same time ensuring every resource manager gets
 * registered and resumed.
 */
Error
handleResourceManagerRegistrations(uint32_t systemModeFlags,
                                   uint32_t bootFlags)
{
   StackObject<Message> message;
   auto id = 0u;
   auto numPendingResumes = 0;
   auto error = Error::OK;

   while (id < sData->resourceManagers.size()) {
      if (sData->resourceManagers[id].data.isDummyRM) {
         auto &dummyRm = sData->resourceManagers[id];

         if (numPendingResumes == 0) {
            // We've processed all our resumes, on to the next resource manager id!
            IOS_GetUpTime64(phys_addrof(dummyRm.data.timeResumeStart));
            ++id;
            continue;
         }
      } else {
         if (!sData->resourceManagers[id].name) {
            // Skip this unimplemented resource.
            ++id;
            continue;
         }

         if ((sData->resourceManagers[id].data.systemModeFlags & systemModeFlags) == 0) {
            // Skip this device if it is not enable for the current system mode.
            ++id;
            continue;
         }

         if (sData->resourceManagers[id].data.state == ResourceManagerRegistrationState::Registered) {
            auto &rm = sData->resourceManagers[id];

            // Send a resume request - transition from Registered to Pending.
            IOS_GetUpTime64(phys_addrof(rm.data.timeResumeStart));
            rm.data.state = ResourceManagerRegistrationState::Pending;
            rm.data.error = Error::Invalid;

            error = IOS_ResumeAsync(rm.data.resourceHandle,
                                    systemModeFlags,
                                    bootFlags,
                                    sData->resourceManagerMessageQueueId,
                                    rm.data.messageBuffer);
            if (error < Error::OK) {
               gLog->error("Unexpected error for IOS_ResumeAsync on resource manager {}, error = {}",
                           rm.name.getRawPointer(), error);
               return error;
            }

            // Increase the number of pending resumes so we wait for the reply on
            // the next dummy manager.
            ++numPendingResumes;

            // Move onto the next resource manager
            ++id;
            continue;
         }
      }

      // Check for any pending messages
      error = waitAsyncReplyWithTimeout(sData->resourceManagerMessageQueueId, message, 10000);
      if (error < Error::OK) {
         gLog->error("Unexpected error for waitAsyncReplyWithTimeout, error = {}", error);
         return error;
      }

      auto request = parseMessage<IpcRequest>(message);
      auto command = static_cast<ResourceManagerCommand>(request->command);
      if (command == ResourceManagerCommand::Timeout) {
         gLog->error("Unexpected timeout whilst waiting for resource manager message");
         return Error::Timeout;
      } else if (command == ResourceManagerCommand::Register) {
         auto &rm = sData->resourceManagers[request->handle];

         // Open a resource handle
         IOS_GetUpTime64(phys_addrof(rm.data.timeOpenStart));
         error = IOS_Open(rm.name.getRawPointer(), static_cast<OpenMode>(0x80000000));
         IOS_GetUpTime64(phys_addrof(rm.data.timeOpenFinished));

         if (error < Error::OK) {
            gLog->error("Unexpected error for IOS_Open on resource manager {}, error = {}",
                        rm.name.getRawPointer(), error);
            return error;
         }

         // Transition from NotRegistered to Registered.
         decaf_check(rm.data.state == ResourceManagerRegistrationState::NotRegistered);
         rm.data.state = ResourceManagerRegistrationState::Registered;
         rm.data.resourceHandle = static_cast<ResourceHandleId>(error);
      } else if (command == ResourceManagerCommand::ResumeReply) {
         // This is a reply to our resume request - transition from Pending to Resumed.
         auto resumeId = request->handle;
         auto &resumeRm = sData->resourceManagers[resumeId];

         decaf_check(resumeRm.data.state == ResourceManagerRegistrationState::Pending);
         resumeRm.data.error = request->reply;
         resumeRm.data.state = ResourceManagerRegistrationState::Resumed;
         IOS_GetUpTime64(phys_addrof(resumeRm.data.timeResumeFinished));

         if (request->reply < Error::OK) {
            resumeRm.data.state = ResourceManagerRegistrationState::Failed;
            gLog->error("Unexpected reply from IOS_ResumeAsync for resource manager {}, error = {}",
                        resumeRm.name.getRawPointer(), request->reply);
            return request->reply;
         }

         --numPendingResumes;
         decaf_check(numPendingResumes >= 0);
      }
   }

   return Error::OK;
}

void
initialiseStaticPmThreadData()
{
   sData = phys_cast<StaticPmThreadData>(allocProcessStatic(sizeof(StaticPmThreadData)));
   sData->resourceManagerTimeoutMessage.command = static_cast<Command>(Error::Timeout);

   auto dummyRM = ResourceManagerRegistration {
         nullptr, 0,
         {
            TRUE, 0u, Error::OK, nullptr, static_cast<ResourceManagerRegistrationState>(0),
            0u, static_cast<ProcessId>(0), 0u, 0ull, 0ull, 0ull, 0ull, 0ull, 0ull
         }
      };

   auto ss = [](const char *str)
      {
         return allocProcessStatic(str);
      };
   auto rm = [](uint32_t systemModeFlags, ProcessId pid, uint32_t unk0x1C)
      {
         return ResourceManagerRegistration::Data
            {
               FALSE, 0, Error::OK, nullptr, static_cast<ResourceManagerRegistrationState>(0),
               systemModeFlags, pid, unk0x1C, 0, 0, 0, 0, 0, 0
            };
      };

   // This table was taken from firmware 5.5.1
   sData->resourceManagers = std::array<ResourceManagerRegistration, 86>
      {
         dummyRM,
         //{ ss("/dev/crypto"),          1, rm(0x1E8000, ProcessId::CRYPTO, 0) },
         { ss("/dev/ahcimgr"),         1, rm(0x1E8000, ProcessId::FS, 0) },
         //{ ss("/dev/usbproc1"),        1, rm(0x1C0000, ProcessId::USB, 0) },
         dummyRM,
         //{ ss("/dev/usb_cdc"),         1, rm(0x1C0000, ProcessId::USB, 0) },
         dummyRM,
         //{ ss("/dev/testproc1"),       1, rm(0x1C0000, ProcessId::TEST, 0) },
         dummyRM,
         //{ ss("/dev/usb_syslog"),      0, rm(0x1E8000, ProcessId::MCP, 0) },
         { ss("/dev/mmc"),             1, rm(0x1E8000, ProcessId::FS, 0) },
         //{ ss("/dev/odm"),             1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/shdd"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/fla"),             1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         //{ ss("/dev/dk"),              1, rm(0x1E8000, ProcessId::FS, 0) },
         //{ ss("/dev/ramdisk_svc"),     1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         //{ ss("/dev/dk_syslog"),       0, rm(0x1E8000, ProcessId::MCP, 0) },
         { ss("/dev/df"),              1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         { ss("/dev/atfs"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/isfs"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/wfs"),             1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/fat"),             1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         { ss("/dev/rbfs"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         { ss("/dev/scfm"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         { ss("/dev/md"),              1, rm(0x1E8000, ProcessId::FS, 0) },
         { ss("/dev/pcfs"),            1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         { ss("/dev/mcp"),             1, rm(0x1A8000, ProcessId::MCP, 0) },
         //{ ss("/dev/mcp_recovery"),    0, rm( 0x40000, ProcessId::MCP, 0) },
         dummyRM,
         //{ ss("/dev/usbproc2"),        1, rm(0x1C0000, ProcessId::USB, 0) },
         { ss("/dev/usr_cfg"),         1, rm(0x180000, ProcessId::AUXIL, 0) },
         //{ ss("/dev/usb_hid"),         1, rm(0x100000, ProcessId::USB, 0) },
         //{ ss("/dev/usb_uac"),         1, rm(0x100000, ProcessId::USB, 0) },
         //{ ss("/dev/usb_midi"),        1, rm(0x100000, ProcessId::USB, 0) },
         dummyRM,
         //{ ss("/dev/ppc_kernel"),      1, rm(0x180000, ProcessId::MCP, 0) },
         //{ ss("/dev/ccr_io"),          1, rm(0x1C8000, ProcessId::PAD, 0) },
         //{ ss("/dev/usb/early_btrm"),  0, rm(0x1C0000, ProcessId::PAD, 3) },
         //{ ss("/dev/testproc2"),       1, rm(0x1C0000, ProcessId::TEST, 0) },
         dummyRM,
         { ss("/dev/ums"),             1, rm(0x1C0000, ProcessId::USB, 0) }, //  WTF?? Should be FS surely?
         //{ ss("/dev/wifi24"),          0, rm(0x188000, ProcessId::PAD, 0) }, // WTF?? Should be NET surely?
         dummyRM,
         { ss("/dev/auxilproc"),       1, rm(0x100000, ProcessId::AUXIL, 1) },
         { ss("/dev/network"),         1, rm(0x180000, ProcessId::NET, 0) },
         dummyRM,
         //{ ss("/dev/nsec"),            1, rm(0x180000, ProcessId::NET, 0) },
         //{ ss("/dev/usb/btrm"),        0, rm(0x1C0000, ProcessId::PAD, 1) },
         //{ ss("/dev/acpproc"),         1, rm(0x188000, ProcessId::ACP, 0) },
         dummyRM,
         //{ ss("/dev/ifuds"),           1, rm(0x100000, ProcessId::PAD, 0) }, // WTF?? Should be NET surely?
         //{ ss("/dev/udscntrl"),        1, rm(0x100000, ProcessId::PAD, 0) }, // WTF?? Should be NET surely?
         dummyRM,
         //{ ss("/dev/nnsm"),            1, rm(0x180000, ProcessId::ACP, 0) },
         dummyRM,
         //{ ss("/dev/dlp"),             1, rm(0x100000, ProcessId::NET, 0) },
         dummyRM,
         //{ ss("/dev/ac_main"),         1, rm(0x180000, ProcessId::NET, 1) },
         dummyRM,
         { ss("/dev/tcp_pcfs"),        1, rm(0x1E8000, ProcessId::FS, 0) },
         dummyRM,
         //{ ss("/dev/act"),             1, rm(0x180000, ProcessId::FPD, 1) },
         dummyRM,
         //{ ss("/dev/fpd"),             1, rm(0x180000, ProcessId::FPD, 1) },
         dummyRM,
         //{ ss("/dev/acp_main"),        1, rm(0x180000, ProcessId::ACP, 1) },
         dummyRM,
         //{ ss("/dev/pdm"),             1, rm(0x180000, ProcessId::ACP, 1) },
         dummyRM,
         //{ ss("/dev/boss"),            1, rm(0x180000, ProcessId::NIM, 1) },
         dummyRM,
         //{ ss("/dev/nim"),             1, rm(0x180000, ProcessId::NIM, 1) },
         dummyRM,
         //{ ss("/dev/ndm"),             1, rm(0x180000, ProcessId::NET, 1) },
         dummyRM,
         //{ ss("/dev/emd"),             1, rm(0x180000, ProcessId::ACP, 1) },
         dummyRM,
         //{ ss("/dev/ppc_app"),         1, rm(0x180000, ProcessId::MCP, 2) },
         dummyRM,
      };
}

} // namespace ios::mcp::internal
