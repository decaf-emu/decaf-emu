#pragma once
#ifdef DECAF_VULKAN

#include <common/platform.h>
#include <common/platform_debug.h>
#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

extern vk::Instance gVulkan;
extern vk::PhysicalDevice gPhysDevice;
extern vk::Device gDevice;
extern vk::Queue gQueue;
extern uint32_t gQueueFamilyIndex;
extern vk::CommandPool gCommandPool;

enum class SsboBufferUsage
{
   Gpu,
   CpuToGpu,
   GpuToCpu
};

struct SsboBuffer
{
   vk::Buffer buffer;
   vk::DeviceMemory memory;
};

struct SyncCmdBuffer
{
   vk::CommandBuffer cmds;
   vk::Fence fence;
};

bool initialiseVulkan();
bool shutdownVulkan();

SsboBuffer allocateSsboBuffer(uint32_t size, SsboBufferUsage usage);
void freeSsboBuffer(SsboBuffer buffer);
void uploadSsboBuffer(SsboBuffer buffer, void *data, uint32_t size);
void downloadSsboBuffer(SsboBuffer buffer, void *data, uint32_t size);

SyncCmdBuffer allocSyncCmdBuffer();
void freeSyncCmdBuffer(SyncCmdBuffer cmdBuffer);
void beginSyncCmdBuffer(SyncCmdBuffer cmdBuffer);
void endSyncCmdBuffer(SyncCmdBuffer cmdBuffer);
void execSyncCmdBuffer(SyncCmdBuffer cmdBuffer);

void globalVkMemoryBarrier(vk::CommandBuffer cmdBuffer, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask);

#endif // DECAF_VULKAN
