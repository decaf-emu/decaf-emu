#include "ios_kernel_ipc_thread.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_resourcemanager.h"
#include "ios_kernel_process.h"
#include "ios/ios_enum_string.h"
#include "ios/ios_stackobject.h"
#include "cafe/kernel/cafe_kernel_ipckdriver.h"

#include <common/log.h>
#include <map>
#include <string>
#include <string_view>

namespace ios::kernel
{

struct StaticResourceManagerData
{
   be2_val<BOOL> registrationEnabled;
   be2_struct<ResourceManagerList> resourceManagerList;
   be2_struct<ResourceRequestList> resourceRequestList;
   be2_array<ResourceHandleManager, ProcessId::Max> resourceHandleManagers;
   be2_val<uint32_t> totalOpenedHandles;
};

static phys_ptr<StaticResourceManagerData>
sData;

namespace internal
{

static Error
findResourceManager(std::string_view device,
                    phys_ptr<ResourceManager> *outResourceManager);

static phys_ptr<ResourceHandleManager>
getResourceHandleManager(ProcessId id);

static Error
allocResourceRequest(phys_ptr<ResourceHandleManager> resourceHandleManager,
                     CpuId cpuId,
                     phys_ptr<ResourceManager> resourceManager,
                     phys_ptr<MessageQueue> messageQueue,
                     phys_ptr<IpcRequest> ipcRequest,
                     phys_ptr<ResourceRequest> *outResourceRequest);

static Error
freeResourceRequest(phys_ptr<ResourceRequest> resourceRequest);

static Error
allocResourceHandle(phys_ptr<ResourceHandleManager> resourceHandleManager,
                    phys_ptr<ResourceManager> resourceManager,
                    ResourceHandleId *outResourceHandleId);

static Error
freeResourceHandle(phys_ptr<ResourceHandleManager> resourceHandleManager,
                   ResourceHandleId id);

static Error
getResourceHandle(ResourceHandleId id,
                  phys_ptr<ResourceHandleManager> resourceHandleManager,
                  phys_ptr<ResourceHandle> *outResourceHandle);

static Error
dispatchResourceReply(phys_ptr<ResourceRequest> resourceRequest,
                      Error reply,
                      bool freeRequest);

static Error
dispatchRequest(phys_ptr<ResourceRequest> request);

} // namespace internal


/**
 * Enable or disable any further resource manager registrations.
 *
 * \returns Error:Access
 * Registration can only be disabled or enabled from the MCP process.
 *
 * \returns Error::Busy
 * For some reason this function returns Error::Busy on success.
 */
Error
IOS_SetResourceManagerRegistrationDisabled(bool disable)
{
   auto pid = internal::getCurrentProcessId();
   if (pid != ProcessId::MCP) {
      return Error::Access;
   }

   sData->registrationEnabled = disable ? FALSE : TRUE;
   return Error::Busy;
}


/**
 * Register a message queue to receive messages for a device.
 */
Error
IOS_RegisterResourceManager(std::string_view device,
                            MessageQueueId queue)
{
   auto pid = internal::getCurrentProcessId();
   auto resourceHandleManager = internal::getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      return Error::Invalid;
   }

   auto error = internal::findResourceManager(device, nullptr);
   if (error >= Error::OK) {
      return Error::Exists;
   }

   if (!sData->registrationEnabled) {
      return Error::NotReady;
   }

   auto &resourceManagerList = sData->resourceManagerList;
   auto resourceManagerIdx = resourceManagerList.firstFreeIdx;
   if (resourceHandleManager->numResourceManagers >= resourceHandleManager->maxResourceManagers ||
       resourceManagerIdx < 0) {
      return Error::FailAlloc;
   }

   auto &resourceManager = resourceManagerList.resourceManagers[resourceManagerIdx];
   auto nextFreeIdx = resourceManager.nextResourceManagerIdx;

   if (nextFreeIdx < 0) {
      resourceManagerList.firstFreeIdx = int16_t { -1 };
      resourceManagerList.lastFreeIdx = int16_t { -1 };
   } else {
      auto &nextFreeResourceManager = resourceManagerList.resourceManagers[nextFreeIdx];
      nextFreeResourceManager.prevResourceManagerIdx = int16_t { -1 };
      resourceManagerList.firstFreeIdx = nextFreeIdx;
   }

   resourceManager.queueId = queue;

   std::strncpy(phys_addrof(resourceManager.device).getRawPointer(), device.data(), resourceManager.device.size());
   resourceManager.deviceLen = static_cast<uint16_t>(device.size());

   resourceManager.numRequests = uint16_t { 0u };
   resourceManager.firstRequestIdx = int16_t { -1 };
   resourceManager.lastRequestIdx = int16_t { -1 };

   resourceManager.prevResourceManagerIdx = int16_t { -1 };
   resourceManager.nextResourceManagerIdx = int16_t { -1 };

   resourceManager.numHandles = uint16_t { 0u };
   resourceManager.unk0x3C = uint16_t { 8u };

   resourceManagerList.numRegistered++;
   if (resourceManagerList.numRegistered > resourceManagerList.mostRegistered) {
      resourceManagerList.mostRegistered = resourceManagerList.numRegistered;
   }

   resourceHandleManager->numResourceManagers++;
   resourceManager.resourceHandleManager = resourceHandleManager;

   if (resourceManagerList.firstRegisteredIdx < 0) {
      resourceManagerList.firstRegisteredIdx = resourceManagerIdx;
      resourceManagerList.lastRegisteredIdx = resourceManagerIdx;
   } else {
      auto insertBeforeIndex = resourceManagerList.firstRegisteredIdx;
      while (insertBeforeIndex >= 0) {
         auto &other = resourceManagerList.resourceManagers[insertBeforeIndex];
         if (device.compare(phys_addrof(other.device).getRawPointer()) < 0) {
            break;
         }

         insertBeforeIndex = other.nextResourceManagerIdx;
      }

      if (insertBeforeIndex == resourceManagerList.firstRegisteredIdx) {
         // Insert at front
         auto &insertBefore = resourceManagerList.resourceManagers[insertBeforeIndex];
         insertBefore.prevResourceManagerIdx = resourceManagerIdx;
         resourceManager.nextResourceManagerIdx = insertBeforeIndex;
         resourceManagerList.firstRegisteredIdx = resourceManagerIdx;
      } else if (insertBeforeIndex < 0) {
         // Insert at back
         auto insertAfterIndex = resourceManagerList.lastRegisteredIdx;
         auto &insertAfter = resourceManagerList.resourceManagers[insertAfterIndex];
         insertAfter.nextResourceManagerIdx = resourceManagerIdx;
         resourceManager.prevResourceManagerIdx = insertAfterIndex;
         resourceManagerList.lastRegisteredIdx = resourceManagerIdx;
      } else {
         // Insert in middle
         auto &insertBefore = resourceManagerList.resourceManagers[insertBeforeIndex];
         auto insertAfterIndex = insertBefore.prevResourceManagerIdx;
         auto &insertAfter = resourceManagerList.resourceManagers[insertAfterIndex];

         insertAfter.nextResourceManagerIdx = resourceManagerIdx;
         insertBefore.prevResourceManagerIdx = resourceManagerIdx;

         resourceManager.nextResourceManagerIdx = insertBeforeIndex;
         resourceManager.prevResourceManagerIdx = insertAfterIndex;
      }
   }

   return Error::OK;
}


/**
 * Associate a resource manager with a permission group (matching cos.xml)
 */
Error
IOS_AssociateResourceManager(std::string_view device,
                             ResourcePermissionGroup group)
{
   auto resourceManager = phys_ptr<ResourceManager> { nullptr };
   auto error = internal::findResourceManager(device, &resourceManager);
   if (error < Error::OK) {
      return error;
   }

   auto pid = internal::getCurrentProcessId();
   if (pid != resourceManager->resourceHandleManager->processId) {
      return Error::Access;
   }

   resourceManager->permissionGroup = group;
   return Error::OK;
}


/**
 * Send a reply to a resource request.
 */
Error
IOS_ResourceReply(phys_ptr<ResourceRequest> resourceRequest,
                  Error reply)
{
   // Get the resource handle manager for the current process
   auto pid = internal::getCurrentProcessId();
   auto resourceHandleManager = internal::getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      resourceHandleManager->failedResourceReplies++;
      return Error::Invalid;
   }

   // Calculate the request index
   auto &resourceRequestList = sData->resourceRequestList;
   auto resourceRequestIndex = resourceRequest - phys_addrof(resourceRequestList.resourceRequests[0]);
   if (resourceRequestIndex < 0 ||
       resourceRequestIndex >= resourceRequestList.resourceRequests.size()) {
      resourceHandleManager->failedResourceReplies++;
      return Error::Invalid;
   }

   if (resourceRequest != phys_addrof(resourceRequestList.resourceRequests[resourceRequestIndex]) ||
      resourceHandleManager != resourceRequest->resourceManager->resourceHandleManager) {
      resourceHandleManager->failedResourceReplies++;
      return Error::Invalid;
   }

   auto resourceHandle = phys_ptr<ResourceHandle> { nullptr };
   auto requestHandleManager = resourceRequest->resourceHandleManager;
   auto error = internal::getResourceHandle(resourceRequest->resourceHandleId,
                                            requestHandleManager,
                                            &resourceHandle);
   if (error < Error::OK) {
      gLog->warn("IOS_ResourceReply({}, {}) passed invalid resource request.",
                 phys_cast<phys_addr>(resourceRequest),
                 reply);
      resourceHandleManager->failedResourceReplies++;
   } else if (resourceRequest->requestData.command == Command::Open) {
      if (reply < Error::OK) {
         // Resource open failed, free the resource handle.
         internal::freeResourceHandle(requestHandleManager,
                                      resourceRequest->resourceHandleId);
      } else {
         // Resource open succeeded, save the resource handle.
         resourceHandle->handle = static_cast<int32_t>(reply);
         resourceHandle->state = ResourceHandleState::Open;
         reply = static_cast<Error>(resourceHandle->id);
      }
   } else if (resourceRequest->requestData.command == Command::Close) {
      // Resource closed, close the resource handle.
      internal::freeResourceHandle(requestHandleManager,
                                   resourceRequest->resourceHandleId);
   }

   return internal::dispatchResourceReply(resourceRequest, reply, true);
}


/**
 * Set the client capability mask for a specific process & feature ID.
 *
 * Can only be set by the MCP process.
 */
Error
IOS_SetClientCapabilities(ProcessId pid,
                          FeatureId featureId,
                          phys_ptr<uint64_t> mask)
{
   if (internal::getCurrentProcessId() != ProcessId::MCP) {
      return Error::Access;
   }

   return internal::setClientCapability(pid, featureId, *mask);
}


namespace internal
{


/**
 * Find a registered ResourceManager for the given device name.
 */
static Error
findResourceManager(std::string_view device,
                    phys_ptr<ResourceManager> *outResourceManager)
{
   auto index = sData->resourceManagerList.firstRegisteredIdx;
   while (index >= 0) {
      auto &resourceManager = sData->resourceManagerList.resourceManagers[index];
      auto resourceManagerDevice = std::string_view {
            phys_addrof(resourceManager.device).getRawPointer(),
            resourceManager.deviceLen
         };

      if (device.compare(resourceManagerDevice) == 0) {
         if (outResourceManager) {
            *outResourceManager = phys_addrof(resourceManager);
         }

         return Error::OK;
      }

      index = resourceManager.nextResourceManagerIdx;
   }

   return Error::NoExists;
}


/**
 * Get the ResourceHandleManager for the specified process.
 */
static phys_ptr<ResourceHandleManager>
getResourceHandleManager(ProcessId id)
{
   if (id >= ProcessId::Max) {
      return nullptr;
   }

   return phys_addrof(sData->resourceHandleManagers[id]);
}


/**
 * Allocate a ResourceRequest.
 */
static Error
allocResourceRequest(phys_ptr<ResourceHandleManager> resourceHandleManager,
                     CpuId cpuId,
                     phys_ptr<ResourceManager> resourceManager,
                     phys_ptr<MessageQueue> messageQueue,
                     phys_ptr<IpcRequest> ipcRequest,
                     phys_ptr<ResourceRequest> *outResourceRequest)
{
   if (resourceHandleManager->numResourceRequests >= resourceHandleManager->maxResourceRequests) {
      return Error::ClientTxnLimit;
   }

   // Find a free resource request to allocate.
   auto &resourceRequestList = sData->resourceRequestList;
   if (resourceRequestList.firstFreeIdx < 0) {
      return Error::FailAlloc;
   }

   auto resourceRequestIdx = resourceRequestList.firstFreeIdx;
   auto resourceRequest = phys_addrof(resourceRequestList.resourceRequests[resourceRequestIdx]);

   auto nextFreeIdx = resourceRequest->nextIdx;
   if (nextFreeIdx < 0) {
      resourceRequestList.firstFreeIdx = int16_t { -1 };
      resourceRequestList.lastFreeIdx = int16_t { -1 };
   } else {
      auto &nextFreeResourceRequest = resourceRequestList.resourceRequests[nextFreeIdx];
      nextFreeResourceRequest.prevIdx = int16_t { -1 };
      resourceRequestList.firstFreeIdx = nextFreeIdx;
   }

   resourceRequest->resourceHandleId = static_cast<ResourceHandleId>(Error::Invalid);
   resourceRequest->messageQueueId = messageQueue->uid;
   resourceRequest->messageQueue = messageQueue;
   resourceRequest->resourceHandleManager = resourceHandleManager;
   resourceRequest->resourceManager = resourceManager;
   resourceRequest->ipcRequest = ipcRequest;

   resourceRequest->requestData.cpuId = cpuId;
   resourceRequest->requestData.processId = resourceHandleManager->processId;
   resourceRequest->requestData.titleId = resourceHandleManager->titleId;
   resourceRequest->requestData.groupId = resourceHandleManager->groupId;

   // Insert into the allocated request list.
   resourceRequest->nextIdx = int16_t { -1 };
   resourceRequest->prevIdx = resourceManager->lastRequestIdx;

   if (resourceManager->lastRequestIdx < 0) {
      resourceManager->firstRequestIdx = resourceRequestIdx;
      resourceManager->lastRequestIdx = resourceRequestIdx;
   } else {
      auto &lastRequest = resourceRequestList.resourceRequests[resourceManager->lastRequestIdx];
      lastRequest.nextIdx = resourceRequestIdx;
      resourceManager->lastRequestIdx = resourceRequestIdx;
   }

   // Increment our counters!
   resourceManager->numRequests++;

   resourceRequestList.numRegistered++;
   if (resourceRequestList.numRegistered > resourceRequestList.mostRegistered) {
      resourceRequestList.mostRegistered = resourceRequestList.numRegistered;
   }

   resourceHandleManager->numResourceRequests++;
   if (resourceHandleManager->numResourceRequests > resourceHandleManager->mostResourceRequests) {
      resourceHandleManager->mostResourceRequests = resourceHandleManager->numResourceRequests;
   }

   *outResourceRequest = resourceRequest;
   return Error::OK;
}


/**
 * Free a ResourceRequest.
 */
static Error
freeResourceRequest(phys_ptr<ResourceRequest> resourceRequest)
{
   auto &resourceRequestList = sData->resourceRequestList;
   auto resourceManager = resourceRequest->resourceManager;

   if (!resourceManager) {
      return Error::Invalid;
   }

   // Remove from the request list
   auto resourceRequestIndex = static_cast<int16_t>(resourceRequest - phys_addrof(resourceRequestList.resourceRequests[0]));
   auto lastFreeIdx = resourceRequestList.lastFreeIdx;
   auto nextIdx = resourceRequest->nextIdx;
   auto prevIdx = resourceRequest->prevIdx;

   if (nextIdx >= 0) {
      auto &nextResourceRequest = resourceRequestList.resourceRequests[nextIdx];
      nextResourceRequest.prevIdx = prevIdx;
   }

   if (prevIdx >= 0) {
      auto &prevResourceRequest = resourceRequestList.resourceRequests[prevIdx];
      prevResourceRequest.nextIdx = nextIdx;
   }

   if (resourceManager->firstRequestIdx == resourceRequestIndex) {
      resourceManager->firstRequestIdx = nextIdx;
   }

   if (resourceManager->lastRequestIdx == resourceRequestIndex) {
      resourceManager->lastRequestIdx = prevIdx;
   }

   // Decrement our counters!
   auto resourceHandleManager = resourceRequest->resourceHandleManager;
   resourceHandleManager->numResourceRequests--;
   resourceRequestList.numRegistered--;
   resourceManager->numRequests--;

   // Reset the resource request
   std::memset(resourceRequest.getRawPointer(), 0, sizeof(ResourceRequest));
   resourceRequest->prevIdx = lastFreeIdx;
   resourceRequest->nextIdx = int16_t { -1 };

   // Reinsert into free list
   resourceRequestList.lastFreeIdx = resourceRequestIndex;

   if (lastFreeIdx < 0) {
      resourceRequestList.firstFreeIdx = resourceRequestIndex;
   } else {
      auto &lastFreeResourceRequest = resourceRequestList.resourceRequests[lastFreeIdx];
      lastFreeResourceRequest.nextIdx = resourceRequestIndex;
   }

   return Error::OK;
}


/**
 * Allocate a ResourceHandle.
 */
static Error
allocResourceHandle(phys_ptr<ResourceHandleManager> resourceHandleManager,
                    phys_ptr<ResourceManager> resourceManager,
                    ResourceHandleId *outResourceHandleId)
{
   // Check if we have a free resource handle to register.
   if (resourceHandleManager->numResourceHandles >= resourceHandleManager->maxResourceHandles) {
      return Error::Max;
   }

   // Find a free resource handle
   phys_ptr<ResourceHandle> resourceHandle = nullptr;
   auto resourceHandleIdx = 0u;

   for (auto i = 0u; i < resourceHandleManager->handles.size(); ++i) {
      if (resourceHandleManager->handles[i].state == ResourceHandleState::Free) {
         resourceHandle = phys_addrof(resourceHandleManager->handles[i]);
         resourceHandleIdx = i;
         break;
      }
   }

   // Double check we have one... should never happen really.
   if (!resourceHandle) {
      return Error::Max;
   }

   resourceHandle->id = static_cast<ResourceHandleId>(resourceHandleIdx | ((sData->totalOpenedHandles << 12) & 0x7FFFFFFF));
   resourceHandle->resourceManager = resourceManager;
   resourceHandle->state = ResourceHandleState::Opening;
   resourceHandle->handle = -10;

   *outResourceHandleId = resourceHandle->id;
   return Error::OK;
}


/**
 * Free a ResourceHandle.
 */
static Error
freeResourceHandle(phys_ptr<ResourceHandleManager> resourceHandleManager,
                   ResourceHandleId id)
{
   phys_ptr<ResourceHandle> resourceHandle;
   auto error = getResourceHandle(id, resourceHandleManager, &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   auto resourceManager = resourceHandle->resourceManager;
   resourceHandle->handle = -4;
   resourceHandle->resourceManager = nullptr;
   resourceHandle->state = ResourceHandleState::Free;
   resourceHandle->id = static_cast<ResourceHandleId>(Error::Invalid);

   resourceHandleManager->numResourceHandles--;
   resourceManager->numHandles--;
   return Error::OK;
}


/**
 * Get ResourceHandle by id.
 */
static Error
getResourceHandle(ResourceHandleId id,
                  phys_ptr<ResourceHandleManager> resourceHandleManager,
                  phys_ptr<ResourceHandle> *outResourceHandle)
{
   auto index = id & 0xFFF;

   if (id < 0 || index >= static_cast<ResourceHandleId>(resourceHandleManager->handles.size())) {
      return Error::Invalid;
   }

   if (resourceHandleManager->handles[index].id != id) {
      return Error::NoExists;
   }

   *outResourceHandle = phys_addrof(resourceHandleManager->handles[index]);
   return Error::OK;
}


/**
 * Lookup an open resource handle.
 */
static Error
getOpenResource(ProcessId pid,
                ResourceHandleId id,
                phys_ptr<ResourceHandleManager> *outResourceHandleManager,
                phys_ptr<ResourceHandle> *outResourceHandle)
{
   phys_ptr<ResourceHandle> resourceHandle;

   // Try get the resource handle manager for this process.
   auto resourceHandleManager = getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      return Error::Invalid;
   }

   auto error = getResourceHandle(id, resourceHandleManager, &resourceHandle);
   if (resourceHandle->state == ResourceHandleState::Closed) {
      return Error::StaleHandle;
   } else if (resourceHandle->state != ResourceHandleState::Open) {
      return Error::Invalid;
   }

   *outResourceHandle = resourceHandle;
   *outResourceHandleManager = resourceHandleManager;
   return Error::OK;
}


/**
 * Dispatch a reply to a ResourceRequest.
 */
static Error
dispatchResourceReply(phys_ptr<ResourceRequest> resourceRequest,
                      Error reply,
                      bool freeRequest)
{
   auto error = Error::Invalid;
   phys_ptr<IpcRequest> ipcRequest = resourceRequest->ipcRequest;
   ipcRequest->command = Command::Reply;
   ipcRequest->reply = reply;

   if (resourceRequest->messageQueueId == getIpcMessageQueueId()) {
      cafe::kernel::ipckDriverIosSubmitReply(ipcRequest);
      error = Error::OK;
   } else {
      phys_ptr<MessageQueue> queue = nullptr;

      if (resourceRequest->messageQueueId < 0) {
         queue = resourceRequest->messageQueue;
      } else {
         error = getMessageQueue(resourceRequest->messageQueueId, &queue);
      }

      if (queue) {
         error = sendMessage(queue,
                             makeMessage(ipcRequest),
                             MessageFlags::NonBlocking);
      }
   }

   if (freeRequest) {
      freeResourceRequest(resourceRequest);
   }

   return error;
}


/**
 * Dispatch a request to it's message queue.
 */
static Error
dispatchRequest(phys_ptr<ResourceRequest> request)
{
   phys_ptr<MessageQueue> queue;
   auto error = getMessageQueue(request->resourceManager->queueId, &queue);
   if (error < Error::OK) {
      return error;
   }

   return sendMessage(queue, makeMessage(request), MessageFlags::NonBlocking);
}


/**
 * Dispatch an IOS_Open request.
 */
Error
dispatchIosOpen(std::string_view device,
                OpenMode mode,
                phys_ptr<MessageQueue> queue,
                phys_ptr<IpcRequest> ipcRequest,
                ProcessId pid,
                CpuId cpuId)
{
   phys_ptr<ClientCapability> clientCapability;
   phys_ptr<ResourceManager> resourceManager;
   phys_ptr<ResourceRequest> resourceRequest;
   ResourceHandleId resourceHandleId;

   // Try get the resource handle manager for this process.
   auto resourceHandleManager = internal::getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      return Error::Invalid;
   }

   // Try find the resource manager for this device name.
   auto error = internal::findResourceManager(device, &resourceManager);
   if (error < Error::OK) {
      return error;
   }

   // Try get the cap bits
   error = getClientCapability(resourceHandleManager,
                               resourceManager->permissionGroup,
                               &clientCapability);
   if (error < Error::OK) {
      return Error::Access;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Open;

   // Set the IOS_Open arguments
   resourceRequest->requestData.args.open.name = phys_addrof(resourceRequest->openNameBuffer);
   resourceRequest->requestData.args.open.mode = mode;
   resourceRequest->requestData.args.open.caps = clientCapability->mask;

   std::strncpy(phys_addrof(resourceRequest->openNameBuffer).getRawPointer(),
                device.data(),
                resourceRequest->openNameBuffer.size());

   // Try allocate a resource handle.
   error = allocResourceHandle(resourceHandleManager,
                               resourceManager,
                               &resourceHandleId);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   resourceRequest->resourceHandleId = resourceHandleId;

   // Increment our counters!
   sData->totalOpenedHandles++;
   resourceManager->numHandles++;
   resourceHandleManager->numResourceHandles++;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceHandle(resourceHandleManager, resourceHandleId);
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Close request.
 */
Error
dispatchIosClose(ResourceHandleId resourceHandleId,
                 phys_ptr<MessageQueue> queue,
                 phys_ptr<IpcRequest> ipcRequest,
                 uint32_t unkArg0,
                 ProcessId pid,
                 CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceRequest> resourceRequest;

   // Try get the resource handle manager for this process.
   auto resourceHandleManager = getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      return Error::Invalid;
   }

   // Try lookup the resource handle.
   auto error = getResourceHandle(resourceHandleId,
                                  resourceHandleManager,
                                  &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   // If the handle no longer has a resource manager associated with it then
   // we need to hack in a resource request.
   auto resourceManager = resourceHandle->resourceManager;
   if (!resourceManager) {
      ios::StackObject<ResourceRequest> resourceRequest;
      std::memset(resourceRequest.getRawPointer(), 0, sizeof(ResourceRequest));
      resourceRequest->requestData.command = Command::Close;
      resourceRequest->requestData.cpuId = cpuId;
      resourceRequest->requestData.processId = pid;
      resourceRequest->requestData.handle = resourceHandleId;
      resourceRequest->ipcRequest = ipcRequest;
      resourceRequest->messageQueue = queue;
      resourceRequest->messageQueueId = queue->uid;

      freeResourceHandle(resourceHandleManager, resourceHandleId);
      return dispatchRequest(resourceRequest);
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Close;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.close.unkArg0 = unkArg0;

   auto previousResourceHandleState = resourceHandle->state;
   resourceHandle->state = ResourceHandleState::Closed;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      resourceHandle->state = previousResourceHandleState;
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Read request.
 */
Error
dispatchIosRead(ResourceHandleId resourceHandleId,
                phys_ptr<void> buffer,
                uint32_t length,
                phys_ptr<MessageQueue> queue,
                phys_ptr<IpcRequest> ipcRequest,
                ProcessId pid,
                CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid, resourceHandleId, &resourceHandleManager, &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   if (length && !buffer) {
      return Error::Access;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Read;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.read.data = buffer;
   resourceRequest->requestData.args.read.length = length;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Write request.
 */
Error
dispatchIosWrite(ResourceHandleId resourceHandleId,
                 phys_ptr<const void> buffer,
                 uint32_t length,
                 phys_ptr<MessageQueue> queue,
                 phys_ptr<IpcRequest> ipcRequest,
                 ProcessId pid,
                 CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   if (length && !buffer) {
      return Error::Access;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Write;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.write.data = buffer;
   resourceRequest->requestData.args.write.length = length;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Seek request.
 */
Error
dispatchIosSeek(ResourceHandleId resourceHandleId,
                uint32_t offset,
                SeekOrigin origin,
                phys_ptr<MessageQueue> queue,
                phys_ptr<IpcRequest> ipcRequest,
                ProcessId pid,
                CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Seek;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.seek.offset = offset;
   resourceRequest->requestData.args.seek.origin = origin;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Ioctl request.
 */
Error
dispatchIosIoctl(ResourceHandleId resourceHandleId,
                 uint32_t ioctlRequest,
                 phys_ptr<const void> inputBuffer,
                 uint32_t inputLength,
                 phys_ptr<void> outputBuffer,
                 uint32_t outputLength,
                 phys_ptr<MessageQueue> queue,
                 phys_ptr<IpcRequest> ipcRequest,
                 ProcessId pid,
                 CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   if ((inputLength && !inputBuffer) || (outputLength && !outputBuffer)) {
      return Error::Access;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Ioctl;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.ioctl.request = ioctlRequest;
   resourceRequest->requestData.args.ioctl.inputBuffer = inputBuffer;
   resourceRequest->requestData.args.ioctl.inputLength = inputLength;
   resourceRequest->requestData.args.ioctl.outputBuffer = outputBuffer;
   resourceRequest->requestData.args.ioctl.outputLength = outputLength;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Ioctlv request.
 */
Error
dispatchIosIoctlv(ResourceHandleId resourceHandleId,
                  uint32_t ioctlRequest,
                  uint32_t numVecIn,
                  uint32_t numVecOut,
                  phys_ptr<IoctlVec> vecs,
                  phys_ptr<MessageQueue> queue,
                  phys_ptr<IpcRequest> ipcRequest,
                  ProcessId pid,
                  CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   if (numVecIn + numVecOut > 0 && !vecs) {
      return Error::Access;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Ioctlv;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.ioctlv.request = ioctlRequest;
   resourceRequest->requestData.args.ioctlv.numVecIn = numVecIn;
   resourceRequest->requestData.args.ioctlv.numVecOut = numVecOut;
   resourceRequest->requestData.args.ioctlv.vecs = vecs;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
* Dispatch an IOS_Resume request.
*/
Error
dispatchIosResume(ResourceHandleId resourceHandleId,
                  uint32_t unkArg0,
                  uint32_t unkArg1,
                  phys_ptr<MessageQueue> queue,
                  phys_ptr<IpcRequest> ipcRequest,
                  ProcessId pid,
                  CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Resume;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.resume.unkArg0 = unkArg0;
   resourceRequest->requestData.args.resume.unkArg1 = unkArg1;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_Suspend request.
 */
Error
dispatchIosSuspend(ResourceHandleId resourceHandleId,
                   uint32_t unkArg0,
                   uint32_t unkArg1,
                   phys_ptr<MessageQueue> queue,
                   phys_ptr<IpcRequest> ipcRequest,
                   ProcessId pid,
                   CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::Suspend;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.suspend.unkArg0 = unkArg0;
   resourceRequest->requestData.args.suspend.unkArg1 = unkArg1;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Dispatch an IOS_SvcMsg request.
 */
Error
dispatchIosSvcMsg(ResourceHandleId resourceHandleId,
                  uint32_t command,
                  uint32_t unkArg1,
                  uint32_t unkArg2,
                  uint32_t unkArg3,
                  phys_ptr<MessageQueue> queue,
                  phys_ptr<IpcRequest> ipcRequest,
                  ProcessId pid,
                  CpuId cpuId)
{
   phys_ptr<ResourceHandle> resourceHandle;
   phys_ptr<ResourceHandleManager> resourceHandleManager;
   phys_ptr<ResourceRequest> resourceRequest;

   auto error = getOpenResource(pid,
                                resourceHandleId,
                                &resourceHandleManager,
                                &resourceHandle);
   if (error < Error::OK) {
      return error;
   }

   // Try allocate a resource request.
   error = allocResourceRequest(resourceHandleManager,
                                cpuId,
                                resourceHandle->resourceManager,
                                queue,
                                ipcRequest,
                                &resourceRequest);
   if (error < Error::OK) {
      return error;
   }

   resourceRequest->requestData.command = Command::SvcMsg;
   resourceRequest->resourceHandleId = resourceHandleId;
   resourceRequest->requestData.handle = resourceHandle->handle;
   resourceRequest->requestData.args.svcMsg.command = command;
   resourceRequest->requestData.args.svcMsg.unkArg1 = unkArg1;
   resourceRequest->requestData.args.svcMsg.unkArg2 = unkArg2;
   resourceRequest->requestData.args.svcMsg.unkArg3 = unkArg3;

   // Try dispatch the request to the relevant resource manager.
   error = dispatchRequest(resourceRequest);
   if (error < Error::OK) {
      freeResourceRequest(resourceRequest);
      return error;
   }

   return Error::OK;
}


/**
 * Find the ClientCapability structure for a specific feature ID.
 */
Error
getClientCapability(phys_ptr<ResourceHandleManager> resourceHandleManager,
                    FeatureId featureId,
                    phys_ptr<ClientCapability> *outClientCapability)
{
   for (auto i = 0u; i < resourceHandleManager->clientCapabilities.size(); ++i) {
      auto caps = phys_addrof(resourceHandleManager->clientCapabilities[i]);
      if (caps->featureId == ResourcePermissionGroup::All ||
          caps->featureId == featureId) {
         if (outClientCapability) {
            *outClientCapability = caps;
         }

         return Error::OK;
      }
   }

   return Error::NoExists;
}


/**
 * Set the client capability mask for a specific process & feature ID.
 */
Error
setClientCapability(ProcessId pid,
                    FeatureId featureId,
                    uint64_t mask)
{
   auto clientCapability = phys_ptr<ClientCapability> { nullptr };
   auto resourceHandleManager = getResourceHandleManager(pid);
   if (!resourceHandleManager) {
      return Error::InvalidArg;
   }

   auto error = getClientCapability(resourceHandleManager,
                                    featureId,
                                    &clientCapability);
   if (error >= Error::OK) {
      if (mask == 0) {
         // Delete client cap
         clientCapability->featureId = -1;
         clientCapability->mask = 0ull;
         return Error::OK;
      }

      // Update client cap
      clientCapability->mask = mask;
      return Error::OK;
   }

   if (mask == 0) {
      return Error::OK;
   }

   // Add new client cap
   clientCapability = nullptr;

   for (auto i = 0u; i < resourceHandleManager->clientCapabilities.size(); ++i) {
      auto cap = phys_addrof(resourceHandleManager->clientCapabilities[i]);

      if (cap->featureId == -1) {
         clientCapability = cap;
         break;
      }
   }

   if (!clientCapability) {
      return Error::FailAlloc;
   }

   clientCapability->featureId = featureId;
   clientCapability->mask = mask;
   return Error::OK;
}

void
initialiseStaticResourceManagerData()
{
   sData = phys_cast<StaticResourceManagerData *>(allocProcessStatic(sizeof(StaticResourceManagerData)));
   sData->registrationEnabled = TRUE;

   // Initialise resourceManagerList
   auto &resourceManagerList = sData->resourceManagerList;
   resourceManagerList.firstRegisteredIdx = int16_t { -1 };
   resourceManagerList.lastRegisteredIdx = int16_t { -1 };

   resourceManagerList.firstFreeIdx = int16_t { 0 };
   resourceManagerList.lastFreeIdx = static_cast<int16_t>(MaxNumResourceManagers - 1);

   for (auto i = 0; i < MaxNumResourceManagers; ++i) {
      auto &resourceManager = resourceManagerList.resourceManagers[i];
      resourceManager.prevResourceManagerIdx = static_cast<int16_t>(i - 1);
      resourceManager.nextResourceManagerIdx = static_cast<int16_t>(i + 1);
   }

   resourceManagerList.resourceManagers[resourceManagerList.firstFreeIdx].prevResourceManagerIdx = int16_t { -1 };
   resourceManagerList.resourceManagers[resourceManagerList.lastFreeIdx].nextResourceManagerIdx = int16_t { -1 };

   // Initialise resourceRequestList
   auto &resourceRequestList = sData->resourceRequestList;
   resourceRequestList.firstFreeIdx = int16_t { 0 };
   resourceRequestList.lastFreeIdx = static_cast<int16_t>(MaxNumResourceRequests - 1);

   for (auto i = 0; i < MaxNumResourceRequests; ++i) {
      auto &resourceRequest = resourceRequestList.resourceRequests[i];
      resourceRequest.prevIdx = static_cast<int16_t>(i - 1);
      resourceRequest.nextIdx = static_cast<int16_t>(i + 1);
   }

   resourceRequestList.resourceRequests[resourceRequestList.firstFreeIdx].prevIdx = int16_t { -1 };
   resourceRequestList.resourceRequests[resourceRequestList.lastFreeIdx].nextIdx = int16_t { -1 };

   // Initialise resourceHandleManagers
   auto &resourceHandleManagers = sData->resourceHandleManagers;
   for (auto i = 0u; i < resourceHandleManagers.size(); ++i) {
      auto &resourceHandleManager = resourceHandleManagers[i];

      resourceHandleManager.processId = static_cast<ProcessId>(i);
      resourceHandleManager.maxResourceHandles = MaxNumResourceHandlesPerProcess;
      resourceHandleManager.maxResourceRequests = MaxNumResourceRequestsPerProcess;

      if (resourceHandleManager.processId >= ProcessId::COSKERNEL) {
         resourceHandleManager.maxResourceManagers = 0u;
      } else {
         resourceHandleManager.maxResourceManagers = MaxNumResourceManagersPerProcess;
      }

      for (auto j = 0u; j < MaxNumResourceHandlesPerProcess; ++j) {
         auto &handle = resourceHandleManager.handles[j];
         handle.handle = -4;
         handle.id = -4;
         handle.resourceManager = nullptr;
         handle.state = ResourceHandleState::Free;
      }

      for (auto j = 0u; j < MaxNumClientCapabilitiesPerProcess; ++j) {
         auto &caps = resourceHandleManager.clientCapabilities[j];
         caps.featureId = -1;
         caps.mask = 0ull;
      }

      setClientCapability(resourceHandleManager.processId, 0, 0xFFFFFFFFFFFFFFFFull);
   }
}

} // namespace internal

} // namespace ios
