#pragma once
#include "ios_nn_recursivemutex.h"
#include "ios_nn_thread.h"

#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_timer.h"
#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"
#include "nn/nn_result.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nn::ipc
{

struct CommandHandlerArgs
{
   phys_ptr<ios::kernel::ResourceRequest> resourceRequest;
   phys_ptr<void> requestBuffer;
   uint32_t requestBufferSize;
   phys_ptr<void> responseBuffer;
   uint32_t responseBufferSize;
   uint32_t numVecsIn;
   uint32_t numVecsOut;
   phys_ptr<::ios::IoctlVec> vecs;
};

using DeviceHandleId = uint32_t;

using CommandHandler = Result(*)(uint32_t unk1,
                                 CommandId command,
                                 CommandHandlerArgs &args);

class Server
{
   static constexpr auto MaxNumDeviceHandles = 32u;
   static constexpr auto MaxNumServices = 32u;

public:
   struct DeviceHandle
   {
      bool open = false;
      uint64_t caps;
      ::ios::ProcessId processId;
      RecursiveMutex mutex;
   };

   struct RegisteredService
   {
      ServiceId id;
      CommandHandler handler = nullptr;
   };

   Server(bool isMcpResourceManager = false,
          ::ios::kernel::ResourcePermissionGroup group = ::ios::kernel::ResourcePermissionGroup::None) :
      mIsMcpResourceManager(isMcpResourceManager),
      mResourcePermissionGroup(group)
   {
   }

   Result initialise(std::string_view deviceName,
                     phys_ptr<::ios::kernel::Message> messageBuffer,
                     uint32_t numMessages);
   Result start(phys_ptr<uint8_t> stackTop, uint32_t stackSize,
                ::ios::kernel::ThreadPriority priority);
   void registerService(ServiceId serviceId, CommandHandler commandHandler);

   template<typename ServiceType>
   void registerService()
   {
      registerService(ServiceType::id, ServiceType::commandHandler);
   }

protected:
   static ::ios::Error threadEntryWrapper(phys_ptr<void> ptr);

   Result threadEntry();
   Result waitForResume();
   Result runMessageLoop();
   Result resolvePendingMessages();

   virtual void intialiseServer() { }
   virtual void finaliseServer() { }

   ::ios::Error openDeviceHandle(uint64_t caps, ::ios::ProcessId processId);
   ::ios::Error closeDeviceHandle(DeviceHandleId handleId);
   ::ios::Error handleMessage(phys_ptr<::ios::kernel::ResourceRequest> request);

protected:
   bool mIsMcpResourceManager;
   ::ios::kernel::MessageQueueId mQueueId = -1;
   ::ios::kernel::TimerId mTimer = -1;
   ::ios::kernel::Message mTimerMessage = static_cast<::ios::kernel::Message>(-32);
   Thread mThread;
   std::string mDeviceName;
   std::array<DeviceHandle, MaxNumDeviceHandles> mDeviceHandles;
   ::ios::kernel::ResourcePermissionGroup mResourcePermissionGroup;

   RecursiveMutex mMutex;
   std::array<RegisteredService, MaxNumServices> mServices;
   ::ios::IpcRequestArgsResume mResumeArgs;
};

} // namespace nn::ipc
