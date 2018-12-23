#pragma once
#ifdef DECAF_VULKAN
#include "decafsdl_graphics.h"

#include <libdecaf/decaf.h>
#include <libgpu/gpu_vulkandriver.h>
#include <libdecaf/decaf_debugger.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

class DecafSDLVulkan : public DecafSDLGraphics
{
public:
   DecafSDLVulkan();
   ~DecafSDLVulkan() override;

   bool
   initialise(int width,
              int height,
              bool renderDebugger = true) override;

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
   bool createRenderPass();
   bool createSwapChain();
   bool createRenderPipeline();
   bool createDescriptorPools();
   bool createBuffers();

   bool destroySwapChain();

   void checkVkResult(vk::Result err);
   void acquireScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);
   void renderScanBuffer(vk::Viewport viewport, vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);
   void releaseScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView);

   static VKAPI_ATTR VkBool32 VKAPI_CALL
   debugMessageCallback(VkDebugReportFlagsEXT flags,
                        VkDebugReportObjectTypeEXT objectType,
                        uint64_t object,
                        size_t location,
                        int32_t messageCode,
                        const char* pLayerPrefix,
                        const char* pMessage,
                        void* pUserData);

   std::shared_ptr<spdlog::logger> mLog;
   std::thread mGraphicsThread;
   gpu::VulkanDriver *mDecafDriver = nullptr;
   decaf::VulkanUiRenderer *mDebugUiRenderer = nullptr;
   vk::ClearColorValue mBackgroundColour;

   vk::Instance mVulkan;
   vk::DispatchLoaderDynamic mVkDynLoader;
   vk::PhysicalDevice mPhysDevice;
   vk::Device mDevice;
   vk::SurfaceKHR mSurface;
   vk::Format mSurfaceFormat;
   uint32_t mQueueFamilyIndex;
   vk::Queue mQueue;
   vk::Queue mDriverQueue;
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
   std::vector<vk::Fence> mRenderFences;
   vk::DescriptorPool mDescriptorPool;
   std::vector<vk::DescriptorSet> mDescriptorSets;
   vk::Buffer mVertBuffer;
   std::vector<vk::Semaphore> mImageAvailableSemaphores;
   std::vector<vk::Semaphore> mRenderFinishedSemaphores;
   uint32_t mFrameIndex;

};

#endif // DECAF_VULKAN
