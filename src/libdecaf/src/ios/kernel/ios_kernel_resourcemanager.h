#pragma once
#include "ios_kernel_enum.h"
#include "ios_kernel_messagequeue.h"
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::kernel
{

static constexpr auto MaxNumResourceManagers = 96u;
static constexpr auto MaxNumResourceRequests = 256u;

static constexpr auto MaxNumResourceHandlesPerProcess = 96u;
static constexpr auto MaxNumResourceManagersPerProcess = 32u;
static constexpr auto MaxNumResourceRequestsPerProcess = 32u;
static constexpr auto MaxNumClientCapabilitiesPerProcess = 20u;

using ResourceHandleID = int32_t;
using FeatureID = int32_t;

struct ResourceManager;
struct ResourceManagerList;

struct ResourceHandle;
struct ResourceHandleManager;

struct ResourceRequest;
struct ResourceRequestList;

#pragma pack(push, 1)

struct ResourceManager
{
   //! Name of the device this resource manager manages.
   be2_array<char, 32> device;

   //! ID of the message queue associated with this resource.
   be2_val<MessageQueueID> queueId = 0u;

   //! Pointer to the resource handle manager for this resource manager.
   be2_phys_ptr<ResourceHandleManager> resourceHandleManager = nullptr;

   //! Permission Group this resource belongs to, this matches values in cos.xml.
   be2_val<uint32_t> permissionGroup = 0u;

   //! Length of the string in this->device
   be2_val<uint16_t> deviceLen = 0u;

   //! Index of the first resource request for this resource in the global
   //! resource request list.
   be2_val<int16_t> firstRequestIdx = -1;

   //! Index of the last resource request for this resource in the global
   //! resource request list.
   be2_val<int16_t> lastRequestIdx = -1;

   //! Number of resource requests active for this resource.
   be2_val<uint16_t> numRequests = 0u;

   //! Number of resource handles active for this resource.
   be2_val<uint16_t> numHandles = 0u;

   //! Index of the next active resource manager.
   be2_val<int16_t> nextResourceManagerIdx = -1;

   //! Index of the previous active resource manager.
   be2_val<int16_t> prevResourceManagerIdx = -1;

   be2_val<uint16_t> unk0x3A;
   be2_val<uint16_t> unk0x3C;
   be2_val<uint16_t> unk0x3E;
};
CHECK_OFFSET(ResourceManager, 0x00, device);
CHECK_OFFSET(ResourceManager, 0x20, queueId);
CHECK_OFFSET(ResourceManager, 0x24, resourceHandleManager);
CHECK_OFFSET(ResourceManager, 0x28, permissionGroup);
CHECK_OFFSET(ResourceManager, 0x2C, deviceLen);
CHECK_OFFSET(ResourceManager, 0x2E, firstRequestIdx);
CHECK_OFFSET(ResourceManager, 0x30, lastRequestIdx);
CHECK_OFFSET(ResourceManager, 0x32, numRequests);
CHECK_OFFSET(ResourceManager, 0x34, numHandles);
CHECK_OFFSET(ResourceManager, 0x36, nextResourceManagerIdx);
CHECK_OFFSET(ResourceManager, 0x38, prevResourceManagerIdx);
CHECK_OFFSET(ResourceManager, 0x3A, unk0x3A);
CHECK_OFFSET(ResourceManager, 0x3C, unk0x3C);
CHECK_OFFSET(ResourceManager, 0x3E, unk0x3E);
CHECK_SIZE(ResourceManager, 0x40);

struct ResourceManagerList
{
   //! Number of registered resource managers.
   be2_val<uint16_t> numRegistered;

   //! The highest number of registered resource managers at one time.
   be2_val<uint16_t> mostRegistered;

   //! Index of the first registered resource manager.
   be2_val<int16_t> firstRegisteredIdx;

   //! Index of the last registered resource manager.
   be2_val<int16_t> lastRegisteredIdx;

   //! Index of the first free resource manager.
   be2_val<int16_t> firstFreeIdx;

   //! Index of the last free resource manager.
   be2_val<int16_t> lastFreeIdx;

   //! List of resource managers.
   be2_array<ResourceManager, MaxNumResourceManagers> resourceManagers;
};
CHECK_OFFSET(ResourceManagerList, 0x00, numRegistered);
CHECK_OFFSET(ResourceManagerList, 0x02, mostRegistered);
CHECK_OFFSET(ResourceManagerList, 0x04, firstRegisteredIdx);
CHECK_OFFSET(ResourceManagerList, 0x06, lastRegisteredIdx);
CHECK_OFFSET(ResourceManagerList, 0x08, firstFreeIdx);
CHECK_OFFSET(ResourceManagerList, 0x0A, lastFreeIdx);
CHECK_OFFSET(ResourceManagerList, 0x0C, resourceManagers);
CHECK_SIZE(ResourceManagerList, 0x180C);

struct ResourceHandle
{
   //! The handle returned by a succesful IOS_Open request.
   be2_val<int32_t> handle;

   //! The unique id of this resource handle.
   be2_val<ResourceHandleID> id;

   //! The resource manager this handle belongs to.
   be2_phys_ptr<ResourceManager> resourceManager;

   //! The state of this resource handle.
   be2_val<ResourceHandleState> state;
};
CHECK_OFFSET(ResourceHandle, 0x00, handle);
CHECK_OFFSET(ResourceHandle, 0x04, id);
CHECK_OFFSET(ResourceHandle, 0x08, resourceManager);
CHECK_OFFSET(ResourceHandle, 0x0C, state);
CHECK_SIZE(ResourceHandle, 0x10);

struct ClientCapability
{
   be2_val<FeatureID> featureID;
   be2_val<uint64_t> mask;
};
CHECK_OFFSET(ClientCapability, 0x00, featureID);
CHECK_OFFSET(ClientCapability, 0x04, mask);
CHECK_SIZE(ClientCapability, 0x0C);

/**
 * A per process structure to manage resource handles.
 */
struct ResourceHandleManager
{
   //! Title ID this resource handle manager belongs to.
   be2_val<uint64_t> tid;

   //! 'gid'?? this resource handle manager belongs to.
   be2_val<uint32_t> gid;

   //! Process this resource handle manager belongs to.
   be2_val<ProcessID> pid;

   //! Number of current resource handles.
   be2_val<uint32_t> numResourceHandles;

   //! Highest number of resource handles at one time.
   be2_val<uint32_t> mostResourceHandles;

   //! Maximum number of resource handles, aka size of handles array.
   be2_val<uint32_t> maxResourceHandles;

   //! List of resource handles.
   be2_array<ResourceHandle, MaxNumResourceHandlesPerProcess> handles;

   //! Number of resource requests.
   be2_val<uint32_t> numResourceRequests;

   //! Highest number of resource requests at once.
   be2_val<uint32_t> mostResourceRequests;

   //! Number of times registerIpcRequest failed due to max number of resource requests.
   be2_val<uint32_t> failedRegisterMaxResourceRequests;

   //! Maxmimum allowed number of resource requests per process.
   be2_val<uint32_t> maxResourceRequests;

   //! Client Capabilities
   be2_array<ClientCapability, MaxNumClientCapabilitiesPerProcess> clientCapabilities;

   //! Number of resource managers for this process.
   be2_val<uint32_t> numResourceManagers;

   //! Maximum allowed number of resource managers for this process.
   be2_val<uint32_t> maxResourceManagers;

   //! Number of times IOS_ResourceReply failed
   be2_val<uint32_t> failedResourceReplies;
};
CHECK_OFFSET(ResourceHandleManager, 0x00, tid);
CHECK_OFFSET(ResourceHandleManager, 0x08, gid);
CHECK_OFFSET(ResourceHandleManager, 0x0C, pid);
CHECK_OFFSET(ResourceHandleManager, 0x10, numResourceHandles);
CHECK_OFFSET(ResourceHandleManager, 0x14, mostResourceHandles);
CHECK_OFFSET(ResourceHandleManager, 0x18, maxResourceHandles);
CHECK_OFFSET(ResourceHandleManager, 0x1C, handles);
CHECK_OFFSET(ResourceHandleManager, 0x61C, numResourceRequests);
CHECK_OFFSET(ResourceHandleManager, 0x620, mostResourceRequests);
CHECK_OFFSET(ResourceHandleManager, 0x624, failedRegisterMaxResourceRequests);
CHECK_OFFSET(ResourceHandleManager, 0x628, maxResourceRequests);
CHECK_OFFSET(ResourceHandleManager, 0x62C, clientCapabilities);
CHECK_OFFSET(ResourceHandleManager, 0x71C, numResourceManagers);
CHECK_OFFSET(ResourceHandleManager, 0x720, maxResourceManagers);
CHECK_OFFSET(ResourceHandleManager, 0x724, failedResourceReplies);
CHECK_SIZE(ResourceHandleManager, 0x728);

struct ResourceRequest
{
   //! Data store for the actual request.
   be2_struct<IpcRequest> requestData;

   //! Message queue this resource request came from.
   be2_phys_ptr<MessageQueue> messageQueue;

   //! Message queue id, why store this and message queue, who knows..?
   be2_val<MessageQueueID> messageQueueId;

   //! Pointer to the IpcRequest that this resource request originated from.
   be2_phys_ptr<IpcRequest> ipcRequest;

   //! Pointer to the resource handle manager for this request.
   be2_phys_ptr<ResourceHandleManager> resourceHandleManager;

   //! Pointer to the resource manager for this request.
   be2_phys_ptr<ResourceManager> resourceManager;

   //! ID of the resource handle associated with this request.
   be2_val<ResourceHandleID> resourceHandleID;

   //! Index of the next resource request, used for either free or registered
   //! list in ResourceRequestList.
   be2_val<int16_t> nextIdx;

   //! Index of the previous resource request, used for either free or registered
   //! list in ResourceRequestList.
   be2_val<int16_t> prevIdx;

   //! Buffer to copy the device name to for IOS_Open calls.
   be2_array<char, 32> openNameBuffer;

   UNKNOWN(0xB4 - 0x74);
};
CHECK_OFFSET(ResourceRequest, 0x00, requestData);
CHECK_OFFSET(ResourceRequest, 0x38, messageQueue);
CHECK_OFFSET(ResourceRequest, 0x3C, messageQueueId);
CHECK_OFFSET(ResourceRequest, 0x40, ipcRequest);
CHECK_OFFSET(ResourceRequest, 0x44, resourceHandleManager);
CHECK_OFFSET(ResourceRequest, 0x48, resourceManager);
CHECK_OFFSET(ResourceRequest, 0x4C, resourceHandleID);
CHECK_OFFSET(ResourceRequest, 0x50, nextIdx);
CHECK_OFFSET(ResourceRequest, 0x52, prevIdx);
CHECK_OFFSET(ResourceRequest, 0x54, openNameBuffer);
CHECK_SIZE(ResourceRequest, 0xB4);

/**
 * Storage for all resource requests.
 */
struct ResourceRequestList
{
   //! Number of registered resource requests.
   be2_val<uint16_t> numRegistered;

   //! Highest number of registered resource requests.
   be2_val<uint16_t> mostRegistered;

   be2_val<uint16_t> unk0x04;

   //! Index of first free element in resourceRequests.
   be2_val<int16_t> firstFreeIdx;

   //! Index of last free element in resourceRequests.
   be2_val<int16_t> lastFreeIdx;

   be2_val<uint16_t> unk0x0A;

   //! Resource Request storage.
   be2_array<ResourceRequest, MaxNumResourceRequests> resourceRequests;
};
CHECK_OFFSET(ResourceRequestList, 0x00, numRegistered);
CHECK_OFFSET(ResourceRequestList, 0x02, mostRegistered);
CHECK_OFFSET(ResourceRequestList, 0x04, unk0x04);
CHECK_OFFSET(ResourceRequestList, 0x06, firstFreeIdx);
CHECK_OFFSET(ResourceRequestList, 0x08, lastFreeIdx);
CHECK_OFFSET(ResourceRequestList, 0x0A, unk0x0A);
CHECK_OFFSET(ResourceRequestList, 0x0C, resourceRequests);
CHECK_SIZE(ResourceRequestList, 0xB40C);

#pragma pack(pop)

Error
IOS_RegisterResourceManager(std::string_view device,
                            MessageQueueID queue);

Error
IOS_SetResourcePermissionGroup(std::string_view device,
                               uint32_t group);

Error
IOS_ResourceReply(phys_ptr<ResourceRequest> request,
                  Error reply);

namespace internal
{

Error
dispatchIosOpen(std::string_view name,
                OpenMode mode,
                phys_ptr<MessageQueue> queue,
                phys_ptr<IpcRequest> request,
                ProcessID pid,
                CpuID cpuID);

void
kernelInitialiseResourceManager();

} // namespace internal

} // namespace ios::kernel
