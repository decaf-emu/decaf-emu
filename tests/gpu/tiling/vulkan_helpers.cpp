#ifdef DECAF_VULKAN

#include "vulkan_helpers.h"

static constexpr bool ENABLE_VALIDATION = false;

vk::Instance gVulkan = {};
vk::PhysicalDevice gPhysDevice = {};
vk::Device gDevice = {};
vk::Queue gQueue = {};
uint32_t gQueueFamilyIndex = -1;
vk::CommandPool gCommandPool = {};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugReportFlagsEXT flags,
                     VkDebugReportObjectTypeEXT objectType,
                     uint64_t object,
                     size_t location,
                     int32_t messageCode,
                     const char* pLayerPrefix,
                     const char* pMessage,
                     void* pUserData)
{
   platform::debugLog(
      fmt::format("Vulkan Debug Report: {}, {}, {}, {}, {}, {}, {}\n",
                  vk::to_string(vk::DebugReportFlagsEXT(flags)),
                  vk::to_string(vk::DebugReportObjectTypeEXT(objectType)),
                  object,
                  location,
                  messageCode,
                  pLayerPrefix,
                  pMessage));

   if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT || flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
      platform::debugBreak();
   }

   return VK_FALSE;
}

bool
initialiseVulkan()
{
   // Create our instance
   std::vector<const char *> instanceLayers = { };
   std::vector<const char *> instanceExtensions = { };

   if (ENABLE_VALIDATION) {
      instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
      instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
   }

   vk::ApplicationInfo appDesc = {};
   appDesc.pApplicationName = "gpu-tile-perf";
   appDesc.applicationVersion = 0;
   appDesc.pEngineName = "";
   appDesc.engineVersion = 0;
   appDesc.apiVersion = VK_API_VERSION_1_0;;

   vk::InstanceCreateInfo instanceDesc = {};
   instanceDesc.pApplicationInfo = &appDesc;
   instanceDesc.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
   instanceDesc.ppEnabledLayerNames = instanceLayers.data();
   instanceDesc.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
   instanceDesc.ppEnabledExtensionNames = instanceExtensions.data();

   gVulkan = vk::createInstance(instanceDesc);

   // Get our Physical Device
   auto physDevices = gVulkan.enumeratePhysicalDevices();
   gPhysDevice = physDevices[0];

   std::vector<const char*> deviceLayers = { };
   std::vector<const char*> deviceExtensions = { };

   if (ENABLE_VALIDATION) {
      deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
   }

   // Find an appropriate queue
   auto queueFamilyProps = gPhysDevice.getQueueFamilyProperties();
   uint32_t queueFamilyIndex = 0;
   for (; queueFamilyIndex < queueFamilyProps.size(); ++queueFamilyIndex) {
      auto &qfp = queueFamilyProps[queueFamilyIndex];

      if (!(qfp.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute))) {
         continue;
      }

      break;
   }

   if (queueFamilyIndex >= queueFamilyProps.size()) {
      printf("Failed to find a suitable Vulkan queue to use.\n");
      return false;
   }

   std::array<float, 1> queuePriorities = { 0.0f };
   vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(),
      queueFamilyIndex,
      static_cast<uint32_t>(queuePriorities.size()),
      queuePriorities.data());

   vk::PhysicalDeviceFeatures deviceFeatures;

   vk::DeviceCreateInfo deviceDesc = { };
   deviceDesc.queueCreateInfoCount = 1;
   deviceDesc.pQueueCreateInfos = &deviceQueueCreateInfo;
   deviceDesc.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
   deviceDesc.ppEnabledLayerNames = deviceLayers.data();
   deviceDesc.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
   deviceDesc.ppEnabledExtensionNames = deviceExtensions.data();
   deviceDesc.pEnabledFeatures = &deviceFeatures;
   deviceDesc.pNext = nullptr;
   gDevice = gPhysDevice.createDevice(deviceDesc);

   // Pick our queue
   gQueue = gDevice.getQueue(queueFamilyIndex, 0);
   gQueueFamilyIndex = queueFamilyIndex;

   // Grab a command pool
   gCommandPool = gDevice.createCommandPool(
      vk::CommandPoolCreateInfo(
         vk::CommandPoolCreateFlagBits::eTransient,
         gQueueFamilyIndex));

   // Set up our debug reporting callback
   vk::DispatchLoaderDynamic vkDynLoader;
   vkDynLoader.init(gVulkan);
   if (vkDynLoader.vkCreateDebugReportCallbackEXT) {
      vk::DebugReportCallbackCreateInfoEXT dbgReportDesc;
      dbgReportDesc.flags =
         vk::DebugReportFlagBitsEXT::eDebug |
         vk::DebugReportFlagBitsEXT::eWarning |
         vk::DebugReportFlagBitsEXT::eError |
         vk::DebugReportFlagBitsEXT::ePerformanceWarning;
      dbgReportDesc.pfnCallback = debugMessageCallback;
      dbgReportDesc.pUserData = nullptr;
      gVulkan.createDebugReportCallbackEXT(dbgReportDesc, nullptr, vkDynLoader);
   }

   return true;
}

bool
shutdownVulkan()
{
   return true;
}

SsboBuffer
allocateSsboBuffer(uint32_t size, SsboBufferUsage usage)
{
   auto findMemoryType = [&](uint32_t typeFilter, vk::MemoryPropertyFlags props)
   {
      auto memProps = gPhysDevice.getMemoryProperties();

      for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
         if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
         }
      }

      printf("Failed to find suitable Vulkan memory type.\n");
      throw std::runtime_error("invalid memory type");
   };

   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage =
      vk::BufferUsageFlagBits::eStorageBuffer |
      vk::BufferUsageFlagBits::eTransferSrc |
      vk::BufferUsageFlagBits::eTransferDst;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 1;
   bufferDesc.pQueueFamilyIndices = &gQueueFamilyIndex;
   auto buffer = gDevice.createBuffer(bufferDesc);

   auto bufferMemReqs = gDevice.getBufferMemoryRequirements(buffer);

   // These memory properties are stolen from VMA
   vk::MemoryPropertyFlags memoryProps;
   if (usage == SsboBufferUsage::Gpu) {
      memoryProps |= vk::MemoryPropertyFlagBits::eDeviceLocal;
   } else if (usage == SsboBufferUsage::CpuToGpu) {
      memoryProps |= vk::MemoryPropertyFlagBits::eHostVisible;
   } else if (usage == SsboBufferUsage::GpuToCpu) {
      memoryProps |= vk::MemoryPropertyFlagBits::eHostVisible;
      memoryProps |= vk::MemoryPropertyFlagBits::eHostCoherent;
      memoryProps |= vk::MemoryPropertyFlagBits::eHostCached;
   }

   vk::MemoryAllocateInfo allocDesc;
   allocDesc.allocationSize = bufferMemReqs.size;
   allocDesc.memoryTypeIndex = findMemoryType(bufferMemReqs.memoryTypeBits, memoryProps);
   auto bufferMem = gDevice.allocateMemory(allocDesc);

   gDevice.bindBufferMemory(buffer, bufferMem, 0);

   return SsboBuffer {
      buffer,
      bufferMem
   };
}

void
freeSsboBuffer(SsboBuffer buffer)
{
   gDevice.destroyBuffer(buffer.buffer);
   gDevice.freeMemory(buffer.memory);
}

void
uploadSsboBuffer(SsboBuffer buffer, void *data, uint32_t size)
{
   auto mappedPtr = gDevice.mapMemory(buffer.memory, 0, size);
   memcpy(mappedPtr, data, size);
   gDevice.flushMappedMemoryRanges({ vk::MappedMemoryRange{ buffer.memory, 0, size } });
   gDevice.unmapMemory(buffer.memory);
}

void
downloadSsboBuffer(SsboBuffer buffer, void *data, uint32_t size)
{
   auto mappedPtr = gDevice.mapMemory(buffer.memory, 0, size);
   gDevice.invalidateMappedMemoryRanges({ vk::MappedMemoryRange{ buffer.memory, 0, size } });
   memcpy(data, mappedPtr, size);
   gDevice.unmapMemory(buffer.memory);
}

SyncCmdBuffer
allocSyncCmdBuffer()
{
   // Allocate a command buffer to use
   vk::CommandBufferAllocateInfo cmdBufferDesc = { };
   cmdBufferDesc.commandPool = gCommandPool;
   cmdBufferDesc.level = vk::CommandBufferLevel::ePrimary;
   cmdBufferDesc.commandBufferCount = 1;
   auto cmdBuffer = gDevice.allocateCommandBuffers(cmdBufferDesc)[0];

   // Preallocate a fence for executing
   auto waitFence = gDevice.createFence(vk::FenceCreateInfo {});

   // Return our object
   SyncCmdBuffer syncCmdBuffer;
   syncCmdBuffer.cmds = cmdBuffer;
   syncCmdBuffer.fence = waitFence;
   return syncCmdBuffer;
}

void
freeSyncCmdBuffer(SyncCmdBuffer cmdBuffer)
{
   // Free our temporary fence
   gDevice.destroyFence(cmdBuffer.fence);

   // Free our command buffer
   gDevice.freeCommandBuffers(gCommandPool, { cmdBuffer.cmds });
}

void
beginSyncCmdBuffer(SyncCmdBuffer cmdBuffer)
{
   // Start recording our command buffer
   cmdBuffer.cmds.begin(
      vk::CommandBufferBeginInfo(
         vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
}

void
endSyncCmdBuffer(SyncCmdBuffer cmdBuffer)
{
   // End recording our command buffer
   cmdBuffer.cmds.end();
}

void
execSyncCmdBuffer(SyncCmdBuffer cmdBuffer)
{
   // Submit this command buffer and wait for completion
   vk::SubmitInfo submitDesc;
   submitDesc.commandBufferCount = 1;
   submitDesc.pCommandBuffers = &cmdBuffer.cmds;
   gQueue.submit(submitDesc, cmdBuffer.fence);

   // Wait for the command buffer to complete
   gDevice.waitForFences({ cmdBuffer.fence }, true, -1);
}

void
globalVkMemoryBarrier(vk::CommandBuffer cmdBuffer, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask)
{
   // Barrier our host writes the transfer reads
   cmdBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllCommands,
      vk::PipelineStageFlagBits::eAllCommands,
      vk::DependencyFlags(),
      { vk::MemoryBarrier(srcAccessMask, dstAccessMask) },
      {}, {}, {}
   );
}

#endif