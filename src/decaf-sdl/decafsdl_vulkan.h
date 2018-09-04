#pragma once
#ifdef DECAF_VULKAN
#include "decafsdl_graphics.h"

#include <vulkan/vulkan.hpp>
#include <libdecaf/decaf.h>
#include <libgpu/gpu_vulkandriver.h>
#include <libdecaf/decaf_debugger.h>

class DecafSDLVulkan : public DecafSDLGraphics
{
public:
   DecafSDLVulkan();
   ~DecafSDLVulkan() override;

   bool
   initialise(int width,
              int height) override;

   void
   shutdown() override;

   void
   windowResized() override;

   void
   renderFrame(Viewport &tv,
               Viewport &drc) override;

   gpu::GraphicsDriver *
   getDecafDriver() override;

   decaf::DebugUiRenderer *
   getDecafDebugUiRenderer() override;

protected:
   bool createWindow(int width, int height);
   bool createInstance();
   bool createDevice();
   bool createSwapChain();

   bool destroySwapChain();

   std::thread mGraphicsThread;
   gpu::VulkanDriver *mDecafDriver = nullptr;
   decaf::VulkanUiRenderer *mDebugUiRenderer = nullptr;
   vk::ClearColorValue mBackgroundColour;

   vk::Instance mVulkan;
   vk::PhysicalDevice mPhysDevice;
   vk::Device mDevice;
   vk::SurfaceKHR mSurface;
   vk::Queue mQueue;
   vk::CommandPool mCommandPool;
   vk::SwapchainKHR mSwapchain;
   std::vector<vk::ImageView> mSwapChainImageViews;
   std::vector<vk::Framebuffer> mFramebuffers;
   vk::RenderPass mRenderPass;
   vk::Pipeline mGraphicsPipeline;
   vk::Fence mRenderFence;
   vk::DescriptorPool mDescriptorPool;
   vk::Semaphore mImageAvailableSemaphore;
   vk::Semaphore mRenderFinishedSemaphore;

};

#endif // DECAF_VULKAN
