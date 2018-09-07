#ifdef DECAF_VULKAN

#include "debugger/debugger.h"
#include "debugger_ui_vulkan.h"

#include <imgui.h>
#include "imgui_impl_vulkan.h"

namespace debugger
{

namespace ui
{

void RendererVulkan::initialise(decaf::VulkanUiRendererInitInfo *info)
{
   mDevice = info->device;
   mQueue = info->queue;
   mCommandPool = info->commandPool;

   ImGui_ImplVulkan_InitInfo imguiInfo;
   imguiInfo.PhysicalDevice = info->physDevice;
   imguiInfo.Device = info->device;
   imguiInfo.Queue = info->queue;
   imguiInfo.PipelineCache = vk::PipelineCache();
   imguiInfo.DescriptorPool = info->descriptorPool;
   imguiInfo.Allocator = nullptr;
   imguiInfo.CheckVkResultFn = nullptr;
   ImGui_ImplVulkan_Init(&imguiInfo, info->renderPass);
}

void RendererVulkan::shutdown()
{
   // TODO: Cleanup our resources
}

void RendererVulkan::onDebuggerInitialized()
{
   // Upload Fonts
   {
      // Use any command queue
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
}

void RendererVulkan::draw(unsigned width, unsigned height, vk::CommandBuffer cmdBuffer)
{
   // Draw the imgui ui
   debugger::draw(width, height);

   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_VULKAN
