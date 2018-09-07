#ifdef DECAF_VULKAN
#include "clilog.h"
#include "config.h"
#include "decafsdl_vulkan.h"
#include "decafsdl_vulkan_shaders.h"

#include <SDL_vulkan.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                     size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
   gCliLog->warn("Vulkan debug report: {}, {}, {}, {}, {}, {}, {}",
                 vk::to_string(vk::DebugReportFlagsEXT(flags)),
                 vk::to_string(vk::DebugReportObjectTypeEXT(objectType)),
                 object,
                 location,
                 messageCode,
                 pLayerPrefix,
                 pMessage);

#ifdef PLATFORM_WINDOWS
   auto logMsg = fmt::format("VKDBG: {} {}  ||=>  {}, {}, {}, {}, {}\n",
                             vk::to_string(vk::DebugReportFlagsEXT(flags)),
                             pMessage,
                             vk::to_string(vk::DebugReportObjectTypeEXT(objectType)),
                             object,
                             location,
                             messageCode,
                             pLayerPrefix);
   OutputDebugStringA(logMsg.c_str());
#endif
   
   return VK_FALSE;
}

static void
checkVkResult(vk::Result err)
{
   if (err == vk::Result::eSuccess) {
      return;
   }

   gCliLog->error("[vulkan] Unexpected error result: {}", vk::to_string(err));
   abort();
}

DecafSDLVulkan::DecafSDLVulkan()
{
   using config::display::background_colour;
   mBackgroundColour.float32[0] = background_colour.r / 255.0f;
   mBackgroundColour.float32[1] = background_colour.g / 255.0f;
   mBackgroundColour.float32[2] = background_colour.b / 255.0f;
}

DecafSDLVulkan::~DecafSDLVulkan()
{
}

bool
DecafSDLVulkan::createWindow(int width, int height)
{
   // Create TV window
   mWindow = SDL_CreateWindow("Decaf",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      gCliLog->error("Failed to create Vulkan SDL window");
      return false;
   }

   return true;
}

bool
DecafSDLVulkan::createInstance()
{
   auto appInfo = vk::ApplicationInfo(
      "Decaf",
      VK_MAKE_VERSION(1, 0, 0),
      "DecafSDL",
      VK_MAKE_VERSION(1, 0, 0),
      VK_API_VERSION_1_0
   );

   std::vector<const char*> layers =
   {
      "VK_LAYER_LUNARG_standard_validation"
   };

   std::vector<const char*> extensions =
   {
       VK_EXT_DEBUG_REPORT_EXTENSION_NAME
   };

   // Add the neccessary SDL Vulkan extensions
   uint32_t extensions_count = 0;
   if (!SDL_Vulkan_GetInstanceExtensions(mWindow, &extensions_count, NULL)) {
      gCliLog->error("Failed to get SDL Vulkan extension count");
      return false;
   }
   extensions.resize(extensions.size() + extensions_count);
   const char** sdlExtensions = extensions.data() + extensions.size() - extensions_count;
   if (!SDL_Vulkan_GetInstanceExtensions(mWindow, &extensions_count, sdlExtensions)) {
      gCliLog->error("Failed to get SDL Vulkan extensions");
      return false;
   }

   mVulkan = vk::createInstance(
      vk::InstanceCreateInfo(
         vk::InstanceCreateFlags(),
         &appInfo,
         static_cast<uint32_t>(layers.size()), layers.data(),
         static_cast<uint32_t>(extensions.size()), extensions.data()
      )
   );

   // Set up our debugging callbacks
   auto vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)mVulkan.getProcAddr("vkCreateDebugReportCallbackEXT");
   auto debugReportCallbackInfo = (VkDebugReportCallbackCreateInfoEXT)vk::DebugReportCallbackCreateInfoEXT(
      vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
      debugMessageCallback,
      nullptr);
   static VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
   auto err = (vk::Result)vkCreateDebugReportCallback(mVulkan, &debugReportCallbackInfo, nullptr, &debugCallback);
   checkVkResult(err);

   return true;
}

bool
DecafSDLVulkan::createDevice()
{
   std::vector<const char*> deviceLayers =
   {
      "VK_LAYER_LUNARG_standard_validation"
   };

   std::vector<const char*> deviceExtensions = {
      "VK_KHR_swapchain"
   };

   auto queueFamilyProps = mPhysDevice.getQueueFamilyProperties();
   uint32_t queueFamilyIndex = 0;
   for (; queueFamilyIndex < queueFamilyProps.size(); ++queueFamilyIndex) {
      auto &qfp = queueFamilyProps[queueFamilyIndex];

      if (!mPhysDevice.getSurfaceSupportKHR(queueFamilyIndex, mSurface)) {
         continue;
      }

      if (!(qfp.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute))) {
         continue;
      }

      break;
   }

   if (queueFamilyIndex >= queueFamilyProps.size()) {
      gCliLog->error("Failed to find a suitable queue to use");
      return false;
   }

   float queuePriority = 0.0f;
   vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(),
      queueFamilyIndex,
      1,
      &queuePriority);

   mDevice = mPhysDevice.createDevice(
      vk::DeviceCreateInfo(
         vk::DeviceCreateFlags(),
         1, &deviceQueueCreateInfo,
         static_cast<uint32_t>(deviceLayers.size()), deviceLayers.data(),
         static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()));

   mQueue = mDevice.getQueue(queueFamilyIndex, 0);

   mCommandPool = mDevice.createCommandPool(
      vk::CommandPoolCreateInfo(
         vk::CommandPoolCreateFlags(),
         deviceQueueCreateInfo.queueFamilyIndex));

   return true;
}

bool
DecafSDLVulkan::createSwapChain()
{
   auto surfaceFormats = mPhysDevice.getSurfaceFormatsKHR(mSurface);
   auto surfaceCaps = mPhysDevice.getSurfaceCapabilitiesKHR(mSurface);

   auto selectedSurfaceFormat = surfaceFormats[0];

   auto swapChainImageFormat = selectedSurfaceFormat.format;
   auto swapChainExtents = surfaceCaps.currentExtent;

   mSwapchain = mDevice.createSwapchainKHR(
      vk::SwapchainCreateInfoKHR(
         {},
         mSurface,
         surfaceCaps.minImageCount,
         swapChainImageFormat,
         vk::ColorSpaceKHR::eSrgbNonlinear,
         swapChainExtents,
         1,
         vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
         vk::SharingMode::eExclusive,
         0,
         nullptr,
         vk::SurfaceTransformFlagBitsKHR::eIdentity,
         vk::CompositeAlphaFlagBitsKHR::eOpaque,
         vk::PresentModeKHR::eFifo,
         true,
         nullptr));

   auto swapChainImages = mDevice.getSwapchainImagesKHR(mSwapchain);

   std::array<vk::AttachmentDescription, 1> rpAttachments;
   rpAttachments[0] = vk::AttachmentDescription(
      {},
      swapChainImageFormat,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR);

   vk::AttachmentReference colorReference(
      0,
      vk::ImageLayout::eColorAttachmentOptimal);

   std::array<vk::SubpassDescription, 1> subpasses;
   subpasses[0] = vk::SubpassDescription(
      {},
      vk::PipelineBindPoint::eGraphics,
      0, nullptr,
      1, &colorReference,
      nullptr,
      nullptr,
      0, nullptr);

   mRenderPass = mDevice.createRenderPass(
      vk::RenderPassCreateInfo(
         {},
         static_cast<uint32_t>(rpAttachments.size()), rpAttachments.data(),
         static_cast<uint32_t>(subpasses.size()), subpasses.data()));


   mSwapChainImageViews.resize(swapChainImages.size());
   mFramebuffers.resize(swapChainImages.size());

   for (auto i = 0u; i < swapChainImages.size(); ++i) {
      mSwapChainImageViews[i] = mDevice.createImageView(
         vk::ImageViewCreateInfo(
            {},
            swapChainImages[i],
            vk::ImageViewType::e2D,
            swapChainImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

      auto swapChainImage = mSwapChainImageViews[i];
      mFramebuffers[i] = mDevice.createFramebuffer(
         vk::FramebufferCreateInfo(
            {},
            mRenderPass,
            1,
            &swapChainImage,
            swapChainExtents.width,
            swapChainExtents.height,
            1));
   }

   return true;
}

bool
DecafSDLVulkan::destroySwapChain()
{
   for (auto &framebuffer : mFramebuffers) {
      mDevice.destroyFramebuffer(framebuffer);
   }
   mFramebuffers.clear();

   for (auto swapChainImageView : mSwapChainImageViews) {
      mDevice.destroyImageView(swapChainImageView);
   }
   mSwapChainImageViews.clear();

   mDevice.destroyRenderPass(mRenderPass);
   mRenderPass = vk::RenderPass();

   mDevice.destroySwapchainKHR(mSwapchain);
   mSwapchain = vk::SwapchainKHR();

   return true;
}

bool
DecafSDLVulkan::initialise(int width, int height)
{
   if (!createWindow(width, height)) {
      return false;
   }

   if (!createInstance()) {
      return false;
   }

   // Set up our devices
   auto physDevices = mVulkan.enumeratePhysicalDevices();
   mPhysDevice = physDevices[0];

   // Create a window surface
   VkSurfaceKHR sdlSurface;
   if (!SDL_Vulkan_CreateSurface(mWindow, mVulkan, &sdlSurface)) {
      gCliLog->error("Failed to create Vulkan render surface");
      return false;
   }
   mSurface = sdlSurface;

   if (!createDevice()) {
      return false;
   }
   
   if (!createSwapChain()) {
      return false;
   }

   vk::UniqueSampler baseSampler = mDevice.createSamplerUnique(
      vk::SamplerCreateInfo(
         {},
         vk::Filter::eLinear,
         vk::Filter::eLinear,
         vk::SamplerMipmapMode::eNearest,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         0.0f,
         false,
         0.0f,
         false,
         vk::CompareOp::eAlways,
         0.0f,
         0.0f,
         vk::BorderColor::eFloatTransparentBlack,
         false));

   std::vector<vk::Sampler> immutableSamplers = { baseSampler.get() };
   std::vector<vk::DescriptorSetLayoutBinding> bindings = {
      vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, immutableSamplers.data()),
      vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
   };

   auto descriptorSetLayout = mDevice.createDescriptorSetLayout(
      vk::DescriptorSetLayoutCreateInfo(
         {},
         static_cast<uint32_t>(bindings.size()), bindings.data()));

   std::vector<vk::DescriptorSetLayout> layoutBindings = { descriptorSetLayout };
   auto pipelineLayout = mDevice.createPipelineLayout(
      vk::PipelineLayoutCreateInfo(
         {},
         static_cast<uint32_t>(layoutBindings.size()), layoutBindings.data(),
         0, nullptr));

   auto scanbufferVertBytesSize = sizeof(scanbufferVertBytes) / sizeof(scanbufferVertBytes[0]);
   auto scanbufferVertModule = mDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, scanbufferVertBytesSize, reinterpret_cast<const uint32_t*>(scanbufferVertBytes)));

   auto scanbufferFragBytesSize = sizeof(scanbufferFragBytes) / sizeof(scanbufferFragBytes[0]);
   auto scanbufferFragModule = mDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, scanbufferFragBytesSize, reinterpret_cast<const uint32_t*>(scanbufferFragBytes)));

   std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, scanbufferVertModule.get(), "main", nullptr),
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, scanbufferFragModule.get(), "main", nullptr)
   };

   std::vector<vk::VertexInputBindingDescription> vtxBindings = {
      vk::VertexInputBindingDescription(0, 16, vk::VertexInputRate::eVertex)
   };

   std::vector<vk::VertexInputAttributeDescription> vtxAttribs = {
      vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0),
      vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, 8),
   };

   // Vertex input stage, we store all our vertices in the actual shaders
   vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
      {},
      static_cast<uint32_t>(vtxBindings.size()), vtxBindings.data(),
      static_cast<uint32_t>(vtxAttribs.size()), vtxAttribs.data());

   vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
      {},
      vk::PrimitiveTopology::eTriangleList,
      false);

   vk::PipelineRasterizationStateCreateInfo rasterizer(
      {},
      false,
      true,
      vk::PolygonMode::eFill,
      vk::CullModeFlagBits::eNone,
      vk::FrontFace::eClockwise,
      false,
      0.0f,
      0.0f,
      0.0f,
      1.0f);

   vk::PipelineMultisampleStateCreateInfo multisampling(
      {},
      vk::SampleCountFlagBits::e1,
      false,
      1.0f,
      nullptr,
      false,
      false);

   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
      vk::PipelineColorBlendAttachmentState(
         false,
         vk::BlendFactor::eOne,
         vk::BlendFactor::eZero,
         vk::BlendOp::eAdd,
         vk::BlendFactor::eOne,
         vk::BlendFactor::eZero,
         vk::BlendOp::eAdd)
   };

   vk::PipelineColorBlendStateCreateInfo colorBlendState(
      {},
      false,
      vk::LogicOp::eCopy,
      static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data(),
      { 0.0f, 0.0f, 0.0f, 0.0f });

   std::vector<vk::DynamicState> dynamicStates = {
      vk::DynamicState::eViewport
   };

   vk::PipelineDynamicStateCreateInfo dynamicState(
      {},
      static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data());

   vk::GraphicsPipelineCreateInfo pipelineInfo;
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.pViewportState = nullptr;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = nullptr;
   pipelineInfo.pColorBlendState = &colorBlendState;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = pipelineLayout;
   pipelineInfo.renderPass = mRenderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = vk::Pipeline();
   pipelineInfo.basePipelineIndex = -1;
   mGraphicsPipeline = mDevice.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

   std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, static_cast<uint32_t>(mFramebuffers.size()))
   };
   vk::DescriptorPoolCreateInfo descriptorPoolInfo;
   descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
   descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets = static_cast<uint32_t>(mFramebuffers.size());
   mDescriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

   mImageAvailableSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
   mRenderFinishedSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
   mRenderFence = mDevice.createFence(vk::FenceCreateInfo());

   // Setup decaf driver
   mDecafDriver = reinterpret_cast<gpu::VulkanDriver*>(gpu::createVulkanDriver());
   mDebugUiRenderer = reinterpret_cast<decaf::VulkanUiRenderer*>(decaf::createDebugVulkanRenderer());

   decaf::VulkanUiRendererInitInfo uiInitInfo;
   uiInitInfo.physDevice = mPhysDevice;
   uiInitInfo.device = mDevice;
   uiInitInfo.queue = mQueue;
   uiInitInfo.descriptorPool = mDescriptorPool;
   uiInitInfo.renderPass = mRenderPass;
   uiInitInfo.commandPool = mCommandPool;
   mDebugUiRenderer->initialise(&uiInitInfo);

   return true;
}

void
DecafSDLVulkan::shutdown()
{
}

void
DecafSDLVulkan::windowResized()
{
   destroySwapChain();
   createSwapChain();
}

void
DecafSDLVulkan::renderFrame(Viewport &tv, Viewport &drc)
{
   // Grab window information
   int width, height;
   SDL_GetWindowSize(mWindow, &width, &height);

   auto renderCmdBuf = mDevice.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(
         mCommandPool,
         vk::CommandBufferLevel::ePrimary, 1))[0];


   uint32_t nextSwapImage;
   mDevice.acquireNextImageKHR(mSwapchain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphore, vk::Fence{}, &nextSwapImage);

   mDevice.resetCommandPool(mCommandPool, vk::CommandPoolResetFlags());

   renderCmdBuf.begin(vk::CommandBufferBeginInfo({}, nullptr));
   {
      vk::RenderPassBeginInfo renderPassBeginInfo;
      renderPassBeginInfo.renderPass = mRenderPass;
      renderPassBeginInfo.framebuffer = mFramebuffers[nextSwapImage];
      renderPassBeginInfo.renderArea = vk::Rect2D({ 0, 0 }, { static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
      renderPassBeginInfo.clearValueCount = 1;
      vk::ClearValue clearValue(mBackgroundColour);
      renderPassBeginInfo.pClearValues = &clearValue;
      renderCmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      renderCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

      // Draw the scan buffers...
      // TODO: Actually draw the scan buffers from libgpu...

      // Draw the debug UI
      mDebugUiRenderer->draw(width, height, renderCmdBuf);

      renderCmdBuf.endRenderPass();
   }
   renderCmdBuf.end();

   {
      vk::SubmitInfo submitInfo;

      std::array<vk::Semaphore, 1> waitSemaphores = { mImageAvailableSemaphore };
      submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
      submitInfo.pWaitSemaphores = waitSemaphores.data();
      vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      submitInfo.pWaitDstStageMask = &waitStage;

      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &renderCmdBuf;

      std::array<vk::Semaphore, 1> signalSemaphores = { mRenderFinishedSemaphore };
      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
      submitInfo.pSignalSemaphores = signalSemaphores.data();

      mQueue.submit({ submitInfo }, mRenderFence);
   }

   {
      vk::PresentInfoKHR presentInfo;

      std::array<vk::Semaphore, 1> waitSemaphores = { mRenderFinishedSemaphore };
      presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
      presentInfo.pWaitSemaphores = waitSemaphores.data();

      presentInfo.pImageIndices = &nextSwapImage;

      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &mSwapchain;

      mQueue.presentKHR(presentInfo);
   }

   mDevice.waitForFences({ mRenderFence }, true, std::numeric_limits<uint64_t>::max());
   mDevice.resetFences({ mRenderFence });
}

gpu::GraphicsDriver *
DecafSDLVulkan::getDecafDriver()
{
   return mDecafDriver;
}

decaf::DebugUiRenderer *
DecafSDLVulkan::getDecafDebugUiRenderer()
{
   return mDebugUiRenderer;
}

#endif // ifdef DECAF_VULKAN
