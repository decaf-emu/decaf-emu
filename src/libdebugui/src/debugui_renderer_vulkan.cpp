#ifdef DECAF_VULKAN
#include "debugui.h"
#include "debugui_manager.h"
#include "debugui_renderer_vulkan.h"

#include <imgui.h>
#include "imgui_impl_vulkan.h"

namespace debugui
{

RendererVulkan::RendererVulkan(const std::string &configPath,
                               VulkanRendererInfo &info) :
   mUi(configPath)
{
   // Initialise vulkan
   mDevice = info.device;
   mQueue = info.queue;
   mCommandPool = info.commandPool;

   ImGui_ImplVulkan_InitInfo imguiInfo;
   imguiInfo.PhysicalDevice = info.physDevice;
   imguiInfo.Device = info.device;
   imguiInfo.Queue = info.queue;
   imguiInfo.PipelineCache = vk::PipelineCache();
   imguiInfo.DescriptorPool = info.descriptorPool;
   imguiInfo.Allocator = nullptr;
   imguiInfo.CheckVkResultFn = nullptr;
   ImGui_ImplVulkan_Init(&imguiInfo, info.renderPass);

   // Uploade imgui fonts
   auto uploadCmdBuf = mDevice.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(
         mCommandPool,
         vk::CommandBufferLevel::ePrimary,
         1))[0];

   uploadCmdBuf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
   ImGui_ImplVulkan_CreateFontsTexture(uploadCmdBuf);
   uploadCmdBuf.end();

   vk::SubmitInfo submitInfo;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &uploadCmdBuf;
   mQueue.submit({ submitInfo }, {});

   mDevice.waitIdle();
   ImGui_ImplVulkan_InvalidateFontUploadObjects();
}

RendererVulkan::~RendererVulkan()
{
   // TODO: Cleanup resources
}

void RendererVulkan::draw(vk::CommandBuffer cmdBuffer, unsigned width, unsigned height)
{
   mUi.draw(width, height);
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

} // namespace debugui

#endif // ifdef DECAF_VULKAN
