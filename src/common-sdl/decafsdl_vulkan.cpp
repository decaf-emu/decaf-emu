#ifdef DECAF_VULKAN
#include "decafsdl_config.h"
#include "decafsdl_vulkan.h"
#include "decafsdl_vulkan_shaders.h"

#include <common/log.h>
#include <SDL_vulkan.h>

DecafSDLVulkan::DecafSDLVulkan()
{
   // Initialise logger
   auto decafLog = spdlog::get("decaf");
   auto sinks = decafLog->sinks();
   mLog = std::make_shared<spdlog::logger>("sdl-vk",
                                           std::begin(sinks),
                                           std::end(sinks));
   mLog->set_level(decafLog->level());

   // Setup background colour
   using config::display::background_colour;
   mBackgroundColour.float32[0] = background_colour.r / 255.0f;
   mBackgroundColour.float32[1] = background_colour.g / 255.0f;
   mBackgroundColour.float32[2] = background_colour.b / 255.0f;
}

DecafSDLVulkan::~DecafSDLVulkan()
{
}

void
DecafSDLVulkan::checkVkResult(vk::Result err)
{
   if (err == vk::Result::eSuccess) {
      return;
   }

   mLog->error("[vulkan] Unexpected error result: {}", vk::to_string(err));
   abort();
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DecafSDLVulkan::debugMessageCallback(VkDebugReportFlagsEXT flags,
                                     VkDebugReportObjectTypeEXT objectType,
                                     uint64_t object,
                                     size_t location,
                                     int32_t messageCode,
                                     const char* pLayerPrefix,
                                     const char* pMessage,
                                     void* pUserData)
{
   bool noBreak = false;

   // TODO: Remove this once vulkan headers correctly support VkPipelineColorBlendAdvancedStateCreateInfoEXT
   if (strstr(pMessage, "VkPipelineColorBlendStateCreateInfo-pNext") != nullptr) {
      static uint64_t seenAdvancedBlendWarning = 0;
      if (seenAdvancedBlendWarning++) {
         return VK_FALSE;
      }
      noBreak = true;
   }

   auto self = reinterpret_cast<DecafSDLVulkan *>(pUserData);
   self->mLog->warn("Vulkan debug report: {}, {}, {}, {}, {}, {}, {}",
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

   if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT || flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
      if (!noBreak) {
         DebugBreak();
      }
   }
#endif

   return VK_FALSE;
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
      mLog->error("Failed to create Vulkan SDL window");
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
       VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
   };

   // Add the neccessary SDL Vulkan extensions
   uint32_t extensions_count = 0;
   if (!SDL_Vulkan_GetInstanceExtensions(mWindow, &extensions_count, NULL)) {
      mLog->error("Failed to get SDL Vulkan extension count");
      return false;
   }
   extensions.resize(extensions.size() + extensions_count);
   const char** sdlExtensions = extensions.data() + extensions.size() - extensions_count;
   if (!SDL_Vulkan_GetInstanceExtensions(mWindow, &extensions_count, sdlExtensions)) {
      mLog->error("Failed to get SDL Vulkan extensions");
      return false;
   }

   vk::InstanceCreateInfo instanceDesc;
   instanceDesc.pApplicationInfo = &appInfo;
   instanceDesc.enabledLayerCount = static_cast<uint32_t>(layers.size());
   instanceDesc.ppEnabledLayerNames = layers.data();
   instanceDesc.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   instanceDesc.ppEnabledExtensionNames = extensions.data();
   mVulkan = vk::createInstance(instanceDesc);

   // Set up our debugging callbacks
   auto vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)mVulkan.getProcAddr("vkCreateDebugReportCallbackEXT");
   auto debugReportCallbackInfo = (VkDebugReportCallbackCreateInfoEXT)vk::DebugReportCallbackCreateInfoEXT(
      vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
      debugMessageCallback,
      this);
   static VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
   auto err = (vk::Result)vkCreateDebugReportCallback(mVulkan, &debugReportCallbackInfo, nullptr, &debugCallback);
   checkVkResult(err);

   return true;
}

bool
DecafSDLVulkan::pickPhysicalDevice()
{
   auto physDevices = mVulkan.enumeratePhysicalDevices();
   mPhysDevice = physDevices[0];

   return true;
}

bool
DecafSDLVulkan::createWindowSurface()
{
   VkSurfaceKHR sdlSurface;
   if (!SDL_Vulkan_CreateSurface(mWindow, mVulkan, &sdlSurface)) {
      mLog->error("Failed to create Vulkan render surface");
      return false;
   }

   mSurface = sdlSurface;

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
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME
   };

   auto queueFamilyProps = mPhysDevice.getQueueFamilyProperties();
   uint32_t queueFamilyIndex = 0;
   for (; queueFamilyIndex < queueFamilyProps.size(); ++queueFamilyIndex) {
      auto &qfp = queueFamilyProps[queueFamilyIndex];

      if (!mPhysDevice.getSurfaceSupportKHR(queueFamilyIndex, mSurface)) {
         continue;
      }

      if (!(qfp.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute))) {
         continue;
      }

      break;
   }

   if (queueFamilyIndex >= queueFamilyProps.size()) {
      mLog->error("Failed to find a suitable queue to use");
      return false;
   }

   float queuePriority = 0.0f;
   vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(),
      queueFamilyIndex,
      1,
      &queuePriority);

   vk::PhysicalDeviceFeatures deviceFeatures;
   deviceFeatures.geometryShader = true;
   deviceFeatures.textureCompressionBC = true;
   deviceFeatures.independentBlend = true;
   deviceFeatures.fillModeNonSolid = true;

   vk::DeviceCreateInfo deviceDesc;
   deviceDesc.queueCreateInfoCount = 1;
   deviceDesc.pQueueCreateInfos = &deviceQueueCreateInfo;
   deviceDesc.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
   deviceDesc.ppEnabledLayerNames = deviceLayers.data();
   deviceDesc.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
   deviceDesc.ppEnabledExtensionNames = deviceExtensions.data();
   deviceDesc.pEnabledFeatures = &deviceFeatures;
   mDevice = mPhysDevice.createDevice(deviceDesc);

   mQueue = mDevice.getQueue(queueFamilyIndex, 0);
   mQueueFamilyIndex = queueFamilyIndex;

   mCommandPool = mDevice.createCommandPool(
      vk::CommandPoolCreateInfo(
         vk::CommandPoolCreateFlags(),
         queueFamilyIndex));

   return true;
}

bool
DecafSDLVulkan::createSwapChain()
{
   auto surfaceFormats = mPhysDevice.getSurfaceFormatsKHR(mSurface);
   auto surfaceCaps = mPhysDevice.getSurfaceCapabilitiesKHR(mSurface);

   auto selectedSurfaceFormat = surfaceFormats[0];

   auto swapChainImageFormat = selectedSurfaceFormat.format;
   mSwapChainExtents = surfaceCaps.currentExtent;

   auto presentMode = vk::PresentModeKHR::eFifo;
   if (config::display::force_sync) {
      presentMode = vk::PresentModeKHR::eMailbox;
   }

   auto presentModes = mPhysDevice.getSurfacePresentModesKHR(mSurface);
   bool foundPresentMode =
      std::find(presentModes.begin(), presentModes.end(), presentMode) != presentModes.end();
   if (!foundPresentMode) {
      mLog->error("Failed to find a suitable present mode to use");
      return false;
   }

   // Create the swap chain itself
   vk::SwapchainCreateInfoKHR swapchainDesc;
   swapchainDesc.surface = mSurface;
   swapchainDesc.minImageCount = surfaceCaps.minImageCount;
   swapchainDesc.imageFormat = swapChainImageFormat;
   swapchainDesc.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
   swapchainDesc.imageExtent = mSwapChainExtents;
   swapchainDesc.imageArrayLayers = 1;
   swapchainDesc.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
   swapchainDesc.imageSharingMode = vk::SharingMode::eExclusive;
   swapchainDesc.queueFamilyIndexCount = 0;
   swapchainDesc.pQueueFamilyIndices = nullptr;
   swapchainDesc.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
   swapchainDesc.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
   swapchainDesc.presentMode = presentMode;
   swapchainDesc.clipped = true;
   swapchainDesc.oldSwapchain = nullptr;
   mSwapchain = mDevice.createSwapchainKHR(swapchainDesc);


   // Create our render pass that targets this attachement
   vk::AttachmentDescription colorAttachmentDesc;
   colorAttachmentDesc.format = swapChainImageFormat;
   colorAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
   colorAttachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
   colorAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
   colorAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
   colorAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
   colorAttachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
   colorAttachmentDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;

   vk::AttachmentReference colorAttachmentRef;
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

   vk::SubpassDescription genericSubpass;
   genericSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
   genericSubpass.inputAttachmentCount = 0;
   genericSubpass.pInputAttachments = nullptr;
   genericSubpass.colorAttachmentCount = 1;
   genericSubpass.pColorAttachments = &colorAttachmentRef;
   genericSubpass.pResolveAttachments = 0;
   genericSubpass.pDepthStencilAttachment = nullptr;
   genericSubpass.preserveAttachmentCount = 0;
   genericSubpass.pPreserveAttachments = nullptr;

   vk::RenderPassCreateInfo renderPassDesc;
   renderPassDesc.attachmentCount = 1;
   renderPassDesc.pAttachments = &colorAttachmentDesc;
   renderPassDesc.subpassCount = 1;
   renderPassDesc.pSubpasses = &genericSubpass;
   renderPassDesc.dependencyCount = 0;
   renderPassDesc.pDependencies = nullptr;
   mRenderPass = mDevice.createRenderPass(renderPassDesc);


   // Create our framebuffers
   auto swapChainImages = mDevice.getSwapchainImagesKHR(mSwapchain);

   mSwapChainImageViews.resize(swapChainImages.size());
   mFramebuffers.resize(swapChainImages.size());

   for (auto i = 0u; i < swapChainImages.size(); ++i) {
      vk::ImageViewCreateInfo imageViewDesc;
      imageViewDesc.image = swapChainImages[i];
      imageViewDesc.viewType = vk::ImageViewType::e2D;
      imageViewDesc.format = swapChainImageFormat;
      imageViewDesc.components = vk::ComponentMapping();
      imageViewDesc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      imageViewDesc.subresourceRange.baseMipLevel = 0;
      imageViewDesc.subresourceRange.levelCount = 1;
      imageViewDesc.subresourceRange.baseArrayLayer = 0;
      imageViewDesc.subresourceRange.layerCount = 1;
      mSwapChainImageViews[i] = mDevice.createImageView(imageViewDesc);

      vk::FramebufferCreateInfo framebufferDesc;
      framebufferDesc.renderPass = mRenderPass;
      framebufferDesc.attachmentCount = 1;
      framebufferDesc.pAttachments = &mSwapChainImageViews[i];
      framebufferDesc.width = mSwapChainExtents.width;
      framebufferDesc.height = mSwapChainExtents.height;
      framebufferDesc.layers = 1;
      mFramebuffers[i] = mDevice.createFramebuffer(framebufferDesc);
   }

   return true;
}

bool
DecafSDLVulkan::createRenderPipeline()
{
   vk::SamplerCreateInfo baseSamplerDesc;
   baseSamplerDesc.magFilter = vk::Filter::eLinear;
   baseSamplerDesc.minFilter = vk::Filter::eLinear;
   baseSamplerDesc.mipmapMode = vk::SamplerMipmapMode::eNearest;
   baseSamplerDesc.addressModeU = vk::SamplerAddressMode::eRepeat;
   baseSamplerDesc.addressModeV = vk::SamplerAddressMode::eRepeat;
   baseSamplerDesc.addressModeW = vk::SamplerAddressMode::eRepeat;
   baseSamplerDesc.mipLodBias = 0.0f;
   baseSamplerDesc.anisotropyEnable = false;
   baseSamplerDesc.maxAnisotropy = 0.0f;
   baseSamplerDesc.compareEnable = false;
   baseSamplerDesc.compareOp = vk::CompareOp::eAlways;
   baseSamplerDesc.minLod = 0.0f;
   baseSamplerDesc.maxLod = 0.0f;
   baseSamplerDesc.borderColor = vk::BorderColor::eFloatTransparentBlack;
   baseSamplerDesc.unnormalizedCoordinates = false;
   mTrivialSampler = mDevice.createSampler(baseSamplerDesc);

   std::array<vk::Sampler, 1> immutableSamplers = { mTrivialSampler };

   std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, immutableSamplers.data()),
      vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
   };

   vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutDesc;
   descriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(bindings.size());
   descriptorSetLayoutDesc.pBindings = bindings.data();
   mDescriptorSetLayout = mDevice.createDescriptorSetLayout(descriptorSetLayoutDesc);

   std::array<vk::DescriptorSetLayout, 1> layoutBindings = { mDescriptorSetLayout };
   vk::PipelineLayoutCreateInfo pipelineLayoutDesc;
   pipelineLayoutDesc.setLayoutCount = static_cast<uint32_t>(layoutBindings.size());
   pipelineLayoutDesc.pSetLayouts = layoutBindings.data();
   pipelineLayoutDesc.pushConstantRangeCount = 0;
   pipelineLayoutDesc.pPushConstantRanges = nullptr;
   mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutDesc);

   auto scanbufferVertBytesSize = sizeof(scanbufferVertBytes) / sizeof(scanbufferVertBytes[0]);
   auto scanbufferVertModule = mDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, scanbufferVertBytesSize, reinterpret_cast<const uint32_t*>(scanbufferVertBytes)));

   auto scanbufferFragBytesSize = sizeof(scanbufferFragBytes) / sizeof(scanbufferFragBytes[0]);
   auto scanbufferFragModule = mDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, scanbufferFragBytesSize, reinterpret_cast<const uint32_t*>(scanbufferFragBytes)));

   std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, scanbufferVertModule.get(), "main", nullptr),
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, scanbufferFragModule.get(), "main", nullptr)
   };

   std::array<vk::VertexInputBindingDescription, 1> vtxBindings = {
      vk::VertexInputBindingDescription(0, 16, vk::VertexInputRate::eVertex)
   };

   std::array<vk::VertexInputAttributeDescription, 2> vtxAttribs = {
      vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0),
      vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, 8),
   };

   // Vertex input stage, we store all our vertices in the actual shaders
   vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
   vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vtxBindings.size());
   vertexInputInfo.pVertexBindingDescriptions = vtxBindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vtxAttribs.size());
   vertexInputInfo.pVertexAttributeDescriptions = vtxAttribs.data();

   vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
   inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
   inputAssembly.primitiveRestartEnable = false;

   vk::Viewport viewport(0.0f, 0.0f,
                         static_cast<float>(mSwapChainExtents.width),
                         static_cast<float>(mSwapChainExtents.height),
                         0.0f, 0.0f);
   vk::Rect2D scissor({ 0,0 }, mSwapChainExtents);
   vk::PipelineViewportStateCreateInfo viewportState;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   vk::PipelineRasterizationStateCreateInfo rasterizer;
   rasterizer.depthClampEnable = false;
   rasterizer.rasterizerDiscardEnable = false;
   rasterizer.polygonMode = vk::PolygonMode::eFill;
   rasterizer.cullMode = vk::CullModeFlagBits::eNone;
   rasterizer.frontFace = vk::FrontFace::eClockwise;
   rasterizer.depthBiasEnable = false;
   rasterizer.depthBiasConstantFactor = 0.0f;
   rasterizer.depthBiasClamp = 0.0f;
   rasterizer.depthBiasSlopeFactor = 0.0f;
   rasterizer.lineWidth = 1.0f;

   vk::PipelineMultisampleStateCreateInfo multisampling;
   multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
   multisampling.sampleShadingEnable = false;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = false;
   multisampling.alphaToOneEnable = false;

   vk::PipelineColorBlendAttachmentState colorBlendAttachement0;
   colorBlendAttachement0.blendEnable = false;
   colorBlendAttachement0.srcColorBlendFactor = vk::BlendFactor::eOne;
   colorBlendAttachement0.dstColorBlendFactor = vk::BlendFactor::eZero;
   colorBlendAttachement0.colorBlendOp = vk::BlendOp::eAdd;
   colorBlendAttachement0.srcAlphaBlendFactor = vk::BlendFactor::eOne;
   colorBlendAttachement0.dstAlphaBlendFactor = vk::BlendFactor::eZero;
   colorBlendAttachement0.alphaBlendOp = vk::BlendOp::eAdd;
   colorBlendAttachement0.colorWriteMask =
      vk::ColorComponentFlagBits::eR |
      vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA;

   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
      colorBlendAttachement0
   };

   vk::PipelineColorBlendStateCreateInfo colorBlendState;
   colorBlendState.logicOpEnable = false;
   colorBlendState.logicOp = vk::LogicOp::eCopy;
   colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
   colorBlendState.pAttachments = colorBlendAttachments.data();
   colorBlendState.blendConstants[0] = 0.0f;
   colorBlendState.blendConstants[1] = 0.0f;
   colorBlendState.blendConstants[2] = 0.0f;
   colorBlendState.blendConstants[3] = 0.0f;

   std::vector<vk::DynamicState> dynamicStates = {
      vk::DynamicState::eViewport
   };

   vk::PipelineDynamicStateCreateInfo dynamicState;
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();

   vk::GraphicsPipelineCreateInfo pipelineInfo;
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = nullptr;
   pipelineInfo.pColorBlendState = &colorBlendState;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = mPipelineLayout;
   pipelineInfo.renderPass = mRenderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = vk::Pipeline();
   pipelineInfo.basePipelineIndex = -1;
   mGraphicsPipeline = mDevice.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

   return true;
}

bool
DecafSDLVulkan::createDescriptorPools()
{
   std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 100)
   };

   vk::DescriptorPoolCreateInfo descriptorPoolInfo;
   descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
   descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets = static_cast<uint32_t>(descriptorPoolSizes.size() * 100);
   mDescriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

   return true;
}

bool
DecafSDLVulkan::createBuffers()
{
   auto findMemoryType = [&](uint32_t typeFilter, vk::MemoryPropertyFlags props)
   {
      auto memProps = mPhysDevice.getMemoryProperties();

      for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
         if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
         }
      }

      throw std::runtime_error("failed to find suitable memory type!");
   };

   static const std::array<float, 24> vertices = {
      -1.0f, -1.0f,  0.0f,  1.0f,
       1.0f, -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,  0.0f,

       1.0f,  1.0f,  1.0f,  0.0f,
      -1.0f,  1.0f,  0.0f,  0.0f,
      -1.0f, -1.0f,  0.0f,  1.0f,
   };

   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = static_cast<uint32_t>(sizeof(float) * vertices.size());
   bufferDesc.usage = vk::BufferUsageFlagBits::eVertexBuffer;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 1;
   bufferDesc.pQueueFamilyIndices = &mQueueFamilyIndex;
   mVertBuffer = mDevice.createBuffer(bufferDesc);

   auto bufferMemReqs = mDevice.getBufferMemoryRequirements(mVertBuffer);

   vk::MemoryAllocateInfo allocDesc;
   allocDesc.allocationSize = bufferMemReqs.size;
   allocDesc.memoryTypeIndex = findMemoryType(bufferMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
   auto bufferMem = mDevice.allocateMemory(allocDesc);

   mDevice.bindBufferMemory(mVertBuffer, bufferMem, 0);

   auto hostMem = mDevice.mapMemory(bufferMem, 0, VK_WHOLE_SIZE);
   memcpy(hostMem, vertices.data(), bufferMemReqs.size);
   mDevice.flushMappedMemoryRanges({ vk::MappedMemoryRange(bufferMem, 0, VK_WHOLE_SIZE) });
   mDevice.unmapMemory(bufferMem);

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
DecafSDLVulkan::initialise(int width, int height, bool renderDebugger)
{
   if (!createWindow(width, height)) {
      return false;
   }

   if (!createInstance()) {
      return false;
   }

   if (!pickPhysicalDevice()) {
      return false;
   }

   if (!createWindowSurface()) {
      return false;
   }

   if (!createDevice()) {
      return false;
   }

   if (!createSwapChain()) {
      return false;
   }

   if (!createRenderPipeline()) {
      return false;
   }

   if (!createDescriptorPools()) {
      return false;
   }

   if (!createBuffers()) {
      return false;
   }

   mDescriptorSets.resize(mFramebuffers.size() * 2);
   for (auto i = 0u; i < mDescriptorSets.size(); ++i) {
      vk::DescriptorSetAllocateInfo descriptorSetAllocDesc;
      descriptorSetAllocDesc.descriptorPool = mDescriptorPool;
      descriptorSetAllocDesc.descriptorSetCount = 1;
      descriptorSetAllocDesc.pSetLayouts = &mDescriptorSetLayout;
      mDescriptorSets[i] = mDevice.allocateDescriptorSets(descriptorSetAllocDesc)[0];
   }

   // Set up our fences
   mImageAvailableSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
   mRenderFinishedSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
   mRenderFence = mDevice.createFence(vk::FenceCreateInfo());

   // Setup decaf driver
   mDecafDriver = reinterpret_cast<gpu::VulkanDriver*>(gpu::createVulkanDriver());
   mDecafDriver->initialise(mPhysDevice, mDevice, mQueue, mQueueFamilyIndex);

   // Set up the debug rendering system
   if (renderDebugger) {
      mDebugUiRenderer = reinterpret_cast<decaf::VulkanUiRenderer*>(decaf::createDebugVulkanRenderer());

      decaf::VulkanUiRendererInitInfo uiInitInfo;
      uiInitInfo.physDevice = mPhysDevice;
      uiInitInfo.device = mDevice;
      uiInitInfo.queue = mQueue;
      uiInitInfo.descriptorPool = mDescriptorPool;
      uiInitInfo.renderPass = mRenderPass;
      uiInitInfo.commandPool = mCommandPool;
      mDebugUiRenderer->initialise(&uiInitInfo);
   }

   // Start graphics thread
   if (!config::display::force_sync) {
      mGraphicsThread = std::thread {
         [this]() {
            mDecafDriver->run();
         } };
   }

   return true;
}

void
DecafSDLVulkan::shutdown()
{
   // Shut down the debugger ui driver
   if (mDebugUiRenderer) {
      mDebugUiRenderer->shutdown();
   }

   // Stop the GPU
   if (!config::display::force_sync) {
      mDecafDriver->stop();
      mGraphicsThread.join();
   }

   // Shut down the gpu driver
   mDecafDriver->shutdown();
}

void
DecafSDLVulkan::windowResized()
{
   destroySwapChain();
   createSwapChain();
}

void
DecafSDLVulkan::acquireScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
   imageBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = image;
   imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
   imageBarrier.subresourceRange.baseMipLevel = 0;
   imageBarrier.subresourceRange.levelCount = 1;
   imageBarrier.subresourceRange.baseArrayLayer = 0;
   imageBarrier.subresourceRange.layerCount = 1;

   cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
                             vk::PipelineStageFlagBits::eAllGraphics,
                             vk::DependencyFlagBits::eByRegion,
                             {},
                             {},
                             { imageBarrier });

   vk::DescriptorImageInfo imageInfo;
   imageInfo.imageView = imageView;
   imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

   vk::WriteDescriptorSet descWriteDesc;
   descWriteDesc.dstSet = descriptorSet;
   descWriteDesc.dstBinding = 1;
   descWriteDesc.dstArrayElement = 0;
   descWriteDesc.descriptorCount = 1;
   descWriteDesc.descriptorType = vk::DescriptorType::eSampledImage;
   descWriteDesc.pImageInfo = &imageInfo;
   mDevice.updateDescriptorSets({ descWriteDesc }, {});
}

void
DecafSDLVulkan::renderScanBuffer(vk::Viewport viewport, vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   cmdBuffer.setViewport(0, { viewport });
   cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 0, { descriptorSet }, {});
   cmdBuffer.bindVertexBuffers(0, { mVertBuffer }, { 0 });
   cmdBuffer.draw(6, 1, 0, 0);
}

void
DecafSDLVulkan::releaseScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
   imageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = image;
   imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
   imageBarrier.subresourceRange.baseMipLevel = 0;
   imageBarrier.subresourceRange.levelCount = 1;
   imageBarrier.subresourceRange.baseArrayLayer = 0;
   imageBarrier.subresourceRange.layerCount = 1;

   cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
                             vk::PipelineStageFlagBits::eAllGraphics,
                             vk::DependencyFlagBits::eByRegion,
                             {},
                             {},
                             { imageBarrier });
}

void
DecafSDLVulkan::renderFrame(Viewport &tv, Viewport &drc)
{
   // Grab window information
   int width, height;
   SDL_GetWindowSize(mWindow, &width, &height);

   // If we are in force-sync mode, we need to poll the GPU to move
   // the command buffers ahead until the flip occurs.
   if (config::display::force_sync) {
      mDecafDriver->runUntilFlip();
   }

   auto renderCmdBuf = mDevice.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(
         mCommandPool,
         vk::CommandBufferLevel::ePrimary, 1))[0];

   uint32_t nextSwapImage;
   mDevice.acquireNextImageKHR(mSwapchain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphore, vk::Fence{}, &nextSwapImage);

   mDevice.resetCommandPool(mCommandPool, vk::CommandPoolResetFlags());
   mDevice.resetFences({ mRenderFence });

   auto descriptorSetTv = mDescriptorSets[nextSwapImage * 2 + 0];
   auto descriptorSetDrc = mDescriptorSets[nextSwapImage * 2 + 1];

   // Grab the scan buffers...
   vk::Image tvImage, drcImage;
   vk::ImageView tvView, drcView;
   mDecafDriver->getSwapBuffers(tvImage, tvView, drcImage, drcView);

   renderCmdBuf.begin(vk::CommandBufferBeginInfo({}, nullptr));
   {
      renderCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

      if (tvImage) {
         acquireScanBuffer(renderCmdBuf, descriptorSetTv, tvImage, tvView);
      }
      if (drcImage) {
         acquireScanBuffer(renderCmdBuf, descriptorSetDrc, drcImage, drcView);
      }

      vk::RenderPassBeginInfo renderPassBeginInfo;
      renderPassBeginInfo.renderPass = mRenderPass;
      renderPassBeginInfo.framebuffer = mFramebuffers[nextSwapImage];
      renderPassBeginInfo.renderArea = vk::Rect2D({ 0, 0 }, { static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
      renderPassBeginInfo.clearValueCount = 1;
      vk::ClearValue clearValue(mBackgroundColour);
      renderPassBeginInfo.pClearValues = &clearValue;
      renderCmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      // TODO: Technically, we are generating these coordinates upside down, and then
      //  'correcting' it here.  We should probably generate these accurately, and then
      //  flip them for OpenGL, which is the API with the unintuitive origin.
      vk::Viewport tvViewport(tv.x, tv.y, tv.width, tv.height);
      tvViewport.y = height - (tvViewport.y + tvViewport.height);

      vk::Viewport drcViewport(drc.x, drc.y, drc.width, drc.height);
      drcViewport.y = height - (drcViewport.y + drcViewport.height);

      if (tvImage) {
         renderScanBuffer(tvViewport, renderCmdBuf, descriptorSetTv, tvImage, tvView);
      }
      if (drcImage) {
         renderScanBuffer(drcViewport, renderCmdBuf, descriptorSetDrc, drcImage, drcView);
      }

      // Draw the debug UI
      if (mDebugUiRenderer) {
         mDebugUiRenderer->draw(width, height, renderCmdBuf);
      }

      renderCmdBuf.endRenderPass();

      if (tvImage) {
         releaseScanBuffer(renderCmdBuf, descriptorSetTv, tvImage, tvView);
      }
      if (drcImage) {
         releaseScanBuffer(renderCmdBuf, descriptorSetDrc, drcImage, drcView);
      }
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

   // Wait for the frame to complete rendering
   mDevice.waitForFences({ mRenderFence }, true, std::numeric_limits<uint64_t>::max());
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
