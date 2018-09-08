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
   bool pickPhysicalDevice();
   bool createWindowSurface();
   bool createDevice();
   bool createSwapChain();
   bool createRenderPipeline();
   bool createDescriptorPools();
   bool createBuffers();

   bool destroySwapChain();

   void acquireScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);
   void renderScanBuffer(vk::Viewport viewport, vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);
   void releaseScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);

   std::thread mGraphicsThread;
   gpu::VulkanDriver *mDecafDriver = nullptr;
   decaf::VulkanUiRenderer *mDebugUiRenderer = nullptr;
   vk::ClearColorValue mBackgroundColour;

   vk::Instance mVulkan;
   vk::PhysicalDevice mPhysDevice;
   vk::Device mDevice;
   vk::SurfaceKHR mSurface;
   uint32_t mQueueFamilyIndex;
   vk::Queue mQueue;
   vk::CommandPool mCommandPool;
   vk::Extent2D mSwapChainExtents;
   vk::SwapchainKHR mSwapchain;
   std::vector<vk::ImageView> mSwapChainImageViews;
   std::vector<vk::Framebuffer> mFramebuffers;
   vk::Sampler mTrivialSampler;
   vk::DescriptorSetLayout mDescriptorSetLayout;
   vk::RenderPass mRenderPass;
   vk::PipelineLayout mPipelineLayout;
   vk::Pipeline mGraphicsPipeline;
   vk::Fence mRenderFence;
   vk::DescriptorPool mDescriptorPool;
   std::vector<vk::DescriptorSet> mDescriptorSets;
   vk::Buffer mVertBuffer;
   vk::Semaphore mImageAvailableSemaphore;
   vk::Semaphore mRenderFinishedSemaphore;

};

#endif // DECAF_VULKAN
