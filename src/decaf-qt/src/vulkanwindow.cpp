#include "vulkanwindow.h"
#include "inputdriver.h"
#include "settings.h"

#include <common-sdl/decafsdl_vulkan_shaders.h>
#include <libgpu/gpu_graphicsdriver.h>
#include <libgpu/gpu_vulkandriver.h>
#include <libdecaf/decaf.h>
#include <libdebugui/debugui.h>

#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QPlatformSurfaceEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTouchEvent>

VulkanWindow::VulkanWindow(QVulkanInstance *instance,
                           SettingsStorage *settingsStorage,
                           DecafInterface *decafInterface,
                           InputDriver *inputDriver) :
   mDecafInterface(decafInterface),
   mInputDriver(inputDriver),
   mSettingsStorage(settingsStorage)
{
   setVulkanInstance(instance);

   auto devFeatures = VkPhysicalDeviceFeatures { };
   devFeatures.depthClamp = true;
   devFeatures.geometryShader = true;
   devFeatures.textureCompressionBC = true;
   devFeatures.independentBlend = true;
   devFeatures.fillModeNonSolid = true;
   devFeatures.samplerAnisotropy = true;
   devFeatures.wideLines = true;
   devFeatures.logicOp = true;
   setPhysicalDeviceFeatures(devFeatures);

   auto devXfbFeatures = VkPhysicalDeviceTransformFeedbackFeaturesEXT { };
   devXfbFeatures.transformFeedback = true;
   devXfbFeatures.geometryStreams = true;
   setPhysicalDeviceTransformFeedbackFeatures(devXfbFeatures);

   setPreferredColorFormats({
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_FORMAT_B8G8R8A8_SRGB
   });

   setDeviceExtensions({
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
      VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
      VK_KHR_MAINTENANCE1_EXTENSION_NAME,
      VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,
      VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
      VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
   });
}

void VulkanWindow::keyPressEvent(QKeyEvent *ev)
{
   if (mRenderer) {
      mRenderer->keyPressEvent(ev);
   }
}

void VulkanWindow::keyReleaseEvent(QKeyEvent *ev)
{
   if (mRenderer) {
      mRenderer->keyReleaseEvent(ev);
   }
}

void VulkanWindow::mousePressEvent(QMouseEvent *ev)
{
   if (mRenderer) {
      mRenderer->mousePressEvent(ev);
   }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *ev)
{
   if (mRenderer) {
      mRenderer->mouseReleaseEvent(ev);
   }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *ev)
{
   if (mRenderer) {
      mRenderer->mouseMoveEvent(ev);
   }
}

void VulkanWindow::wheelEvent(QWheelEvent *ev)
{
   if (mRenderer) {
      mRenderer->wheelEvent(ev);
   }
}

void VulkanWindow::touchEvent(QTouchEvent *ev)
{
   if (mRenderer) {
      mRenderer->touchEvent(ev);
   }
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
   mRenderer = new VulkanRenderer { this, mSettingsStorage, mDecafInterface, mInputDriver };
   return mRenderer;
}

VulkanRenderer::VulkanRenderer(QVulkanWindow2 *w,
                               SettingsStorage *settingsStorage,
                               DecafInterface *decafInterface,
                               InputDriver *inputDriver) :
   m_window(w),
   mDecafInterface(decafInterface),
   mInputDriver(inputDriver),
   mSettingsStorage(settingsStorage)
{
   QObject::connect(mDecafInterface, &DecafInterface::titleLoaded,
                    this, &VulkanRenderer::titleLoaded);
   QObject::connect(mSettingsStorage, &SettingsStorage::settingsChanged,
                    this, &VulkanRenderer::settingsChanged);
   settingsChanged();
}

void VulkanRenderer::initResources()
{
   qDebug("initResources");

   mDevice = m_window->device();
   m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

   // Initialise trivial sampler
   auto baseSamplerDesc = vk::SamplerCreateInfo { };
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

   // Initialise descriptor set layout
   std::array<vk::Sampler, 1> immutableSamplers = { mTrivialSampler };
   std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, immutableSamplers.data()),
      vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
   };
   auto descriptorSetLayoutDesc = vk::DescriptorSetLayoutCreateInfo { };
   descriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(bindings.size());
   descriptorSetLayoutDesc.pBindings = bindings.data();
   mDescriptorSetLayout = mDevice.createDescriptorSetLayout(descriptorSetLayoutDesc);

   // Initialise pipeline layout
   std::array<vk::DescriptorSetLayout, 1> layoutBindings = { mDescriptorSetLayout };
   auto pipelineLayoutDesc = vk::PipelineLayoutCreateInfo { };
   pipelineLayoutDesc.setLayoutCount = static_cast<uint32_t>(layoutBindings.size());
   pipelineLayoutDesc.pSetLayouts = layoutBindings.data();
   pipelineLayoutDesc.pushConstantRangeCount = 0;
   pipelineLayoutDesc.pPushConstantRanges = nullptr;
   mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutDesc);

   // Initialise descriptor pool
   std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 100),
      vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 100),
      vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100)
   };

   auto descriptorPoolInfo = vk::DescriptorPoolCreateInfo { };
   descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
   descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets = static_cast<uint32_t>(descriptorPoolSizes.size() * 100);
   mDescriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

   // Initialise descriptor sets
   for (auto &frameResources : mFrameResources) {
      auto descriptorSetAllocDesc = vk::DescriptorSetAllocateInfo { };
      descriptorSetAllocDesc.descriptorPool = mDescriptorPool;
      descriptorSetAllocDesc.descriptorSetCount = 1;
      descriptorSetAllocDesc.pSetLayouts = &mDescriptorSetLayout;
      frameResources.tvDesc = mDevice.allocateDescriptorSets(descriptorSetAllocDesc)[0];
      frameResources.drcDesc = mDevice.allocateDescriptorSets(descriptorSetAllocDesc)[0];
   }

   // Initialise buffers
   auto findMemoryType =
      [](vk::PhysicalDevice physDevice, uint32_t typeFilter, vk::MemoryPropertyFlags props) {
         auto memProps = physDevice.getMemoryProperties();

         for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
               return i;
            }
         }

         throw std::runtime_error("failed to find suitable memory type!");
      };

   static const std::array<float, 24> vertices = {
      -1.0f,  1.0f,  0.0f,  1.0f,
       1.0f,  1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,  0.0f,

       1.0f, -1.0f,  1.0f,  0.0f,
      -1.0f, -1.0f,  0.0f,  0.0f,
      -1.0f,  1.0f,  0.0f,  1.0f,
   };

   uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(m_window->graphicsQueueFamilyIdx()) };

   auto bufferDesc = vk::BufferCreateInfo { };
   bufferDesc.size = static_cast<uint32_t>(sizeof(float) * vertices.size());
   bufferDesc.usage = vk::BufferUsageFlagBits::eVertexBuffer;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 1;
   bufferDesc.pQueueFamilyIndices = queueFamilyIndices;
   mVertexBuffer = mDevice.createBuffer(bufferDesc);

   auto bufferMemReqs = mDevice.getBufferMemoryRequirements(mVertexBuffer);

   auto allocDesc = vk::MemoryAllocateInfo { };
   allocDesc.allocationSize = bufferMemReqs.size;
   allocDesc.memoryTypeIndex = findMemoryType(m_window->physicalDevice(), bufferMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
   auto bufferMem = mDevice.allocateMemory(allocDesc);

   mDevice.bindBufferMemory(mVertexBuffer, bufferMem, 0);
   auto hostMem = mDevice.mapMemory(bufferMem, 0, VK_WHOLE_SIZE);
   memcpy(hostMem, vertices.data(), bufferMemReqs.size);
   mDevice.flushMappedMemoryRanges({ vk::MappedMemoryRange(bufferMem, 0, VK_WHOLE_SIZE) });
   mDevice.unmapMemory(bufferMem);

   // Create libgpu driver
   mDecafDriver = reinterpret_cast<gpu::VulkanDriver*>(gpu::createVulkanDriver());
   mDecafDriver->initialise(m_window->vulkanInstance()->vkInstance(),
                            m_window->physicalDevice(),
                            mDevice,
                            m_window->graphicsDriverQueue(),
                            m_window->graphicsQueueFamilyIdx());
   decaf::setGraphicsDriver(mDecafDriver);
   mGraphicsThread = std::thread { [this]() { mDecafDriver->run(); } };

   // Create debug ui renderer
   auto uiInitInfo = debugui::VulkanRendererInfo { };
   uiInitInfo.physDevice = m_window->physicalDevice();
   uiInitInfo.device = m_window->device();
   uiInitInfo.queue = m_window->graphicsQueue();
   uiInitInfo.descriptorPool = mDescriptorPool;
   uiInitInfo.renderPass = m_window->defaultRenderPass();
   uiInitInfo.commandPool = m_window->graphicsCommandPool();
   mDebugUi = debugui::createVulkanRenderer(decaf::makeConfigPath("imgui.ini"), uiInitInfo);
}

void VulkanRenderer::initSwapChainResources()
{
   qDebug("initSwapChainResources");
   createRenderPipeline();
}

bool VulkanRenderer::createRenderPipeline()
{
   auto swapChainImageSize = m_window->swapChainImageSize();

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
   auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo { };
   vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vtxBindings.size());
   vertexInputInfo.pVertexBindingDescriptions = vtxBindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vtxAttribs.size());
   vertexInputInfo.pVertexAttributeDescriptions = vtxAttribs.data();

   auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo { };
   inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
   inputAssembly.primitiveRestartEnable = false;

   auto viewport =
      vk::Viewport { 0.0f, 0.0f,
                     static_cast<float>(swapChainImageSize.width()),
                     static_cast<float>(swapChainImageSize.height()),
                     0.0f, 0.0f };
   auto scissor = vk::Rect2D {
      { 0,0 },
      { static_cast<uint32_t>(swapChainImageSize.width()),
        static_cast<uint32_t>(swapChainImageSize.height()) } };

   auto viewportState = vk::PipelineViewportStateCreateInfo { };
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   auto rasterizer = vk::PipelineRasterizationStateCreateInfo { };
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

   auto multisampling = vk::PipelineMultisampleStateCreateInfo { };
   multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
   multisampling.sampleShadingEnable = false;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = false;
   multisampling.alphaToOneEnable = false;

   auto colorBlendAttachement0 = vk::PipelineColorBlendAttachmentState { };
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

   auto colorBlendState = vk::PipelineColorBlendStateCreateInfo { };
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

   auto dynamicState = vk::PipelineDynamicStateCreateInfo { };
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();

   auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo { };

   vk::GraphicsPipelineCreateInfo pipelineInfo;
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = &depthStencilState;
   pipelineInfo.pColorBlendState = &colorBlendState;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = mPipelineLayout;
   pipelineInfo.renderPass = m_window->defaultRenderPass();
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = vk::Pipeline();
   pipelineInfo.basePipelineIndex = -1;
   mGraphicsPipeline = mDevice.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);
   return true;
}


void VulkanRenderer::releaseSwapChainResources()
{
   qDebug("releaseSwapChainResources");

   if (mGraphicsPipeline) {
      mDevice.destroyPipeline(mGraphicsPipeline);
      mGraphicsPipeline = nullptr;
   }
}

void VulkanRenderer::releaseResources()
{
   qDebug("releaseResources");

   if (mVertexBuffer) {
      mDevice.destroyBuffer(mVertexBuffer);
      mVertexBuffer = nullptr;
   }

#if 0 // Apparently we do not need to free descriptor sets?
   for (auto &frameResources : mFrameResources) {
      if (frameResources.tvDesc) {
         mDevice.freeDescriptorSets(mDescriptorPool, { frameResources.tvDesc });
         frameResources.tvDesc = nullptr;
      }

      if (frameResources.drcDesc) {
         mDevice.freeDescriptorSets(mDescriptorPool, { frameResources.drcDesc });
         frameResources.drcDesc = nullptr;
      }
   }
#endif

   if (mDescriptorPool) {
      mDevice.destroyDescriptorPool(mDescriptorPool);
      mDescriptorPool = nullptr;
   }

   if (mPipelineLayout) {
      mDevice.destroyPipelineLayout(mPipelineLayout);
      mPipelineLayout = nullptr;
   }

   if (mDescriptorSetLayout) {
      mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout);
      mDescriptorSetLayout = nullptr;
   }

   if (mTrivialSampler) {
      mDevice.destroySampler(mTrivialSampler);
      mTrivialSampler = nullptr;
   }

   // Shut down the debugui
   if (mDebugUi) {
      delete mDebugUi;
      mDebugUi = nullptr;
   }

   // Stop the GPU
   mDecafDriver->stop();
   mGraphicsThread.join();

   // Shut down the gpu driver
   mDecafDriver->shutdown();
}

void
VulkanRenderer::calculateScreenViewports(vk::Viewport &tv, vk::Viewport &drc)
{
   // Sizes for calculating aspect ratio
   constexpr auto TvWidth = 1280.0f;
   constexpr auto TvHeight = 720.0f;
   constexpr auto DrcWidth = 854.0f;
   constexpr auto DrcHeight = 480.0f;
   constexpr auto SplitCombinedWidth = std::max(TvWidth, DrcWidth);
   constexpr auto SplitCombinedHeight = TvHeight + DrcHeight;
   const auto splitScreenSeparation = static_cast<float>(mDisplaySettings.splitSeperation);

   const auto swapChainImageSize = m_window->swapChainImageSize();
   const auto windowWidth = static_cast<float>(swapChainImageSize.width());
   const auto windowHeight = static_cast<float>(swapChainImageSize.height());

   auto maintainAspectRatio =
      [&](vk::Viewport &viewport,
          float width, float height,
          float referenceWidth, float referenceHeight)
      {
         const auto widthRatio = static_cast<float>(width) / referenceWidth;
         const auto heightRatio = static_cast<float>(height) / referenceHeight;
         const auto aspectRatio = std::min(widthRatio, heightRatio);
         viewport.width = aspectRatio * referenceWidth;
         viewport.height = aspectRatio * referenceHeight;
      };

   drc = vk::Viewport { };
   tv = vk::Viewport { };

   if (mDisplaySettings.viewMode == DisplaySettings::TV) {
      if (mDisplaySettings.maintainAspectRatio) {
         maintainAspectRatio(tv, windowWidth, windowHeight, TvWidth, TvHeight);
         tv.x = (windowWidth - tv.width) / 2.0f;
         tv.y = (windowHeight - tv.height) / 2.0f;
      } else {
         tv.width = windowWidth;
         tv.height = windowHeight;
      }
   } else if (mDisplaySettings.viewMode == DisplaySettings::Gamepad1) {
      if (mDisplaySettings.maintainAspectRatio) {
         maintainAspectRatio(drc, windowWidth, windowHeight, DrcWidth, DrcHeight);
         drc.x = (windowWidth - drc.width) / 2.0f;
         drc.y = (windowHeight - drc.height) / 2.0f;
      } else {
         drc.width = windowWidth;
         drc.height = windowHeight;
      }
   } else if (mDisplaySettings.viewMode == DisplaySettings::Split) {
      if (mDisplaySettings.maintainAspectRatio) {
         auto combined = vk::Viewport { };
         maintainAspectRatio(combined, windowWidth, windowHeight,
                             SplitCombinedWidth,
                             SplitCombinedHeight + splitScreenSeparation);

         tv.width = combined.width * (TvWidth / SplitCombinedWidth);
         tv.height = combined.height * (TvHeight / SplitCombinedHeight) - (splitScreenSeparation / 2.0f);
         tv.x = (windowWidth - tv.width) / 2.0f;
         tv.y = (windowHeight - combined.height) / 2.0f;

         drc.width = combined.width * (DrcWidth / SplitCombinedWidth);
         drc.height = combined.height * (DrcHeight / SplitCombinedHeight) - (splitScreenSeparation / 2.0f);
         drc.x = (windowWidth - drc.width) / 2.0f;
         drc.y = tv.y + tv.height + splitScreenSeparation;
      } else {
         tv.width = windowWidth;
         tv.height = windowHeight * (TvHeight / SplitCombinedHeight);
         drc.width = windowWidth;
         drc.height = windowHeight * (DrcHeight / SplitCombinedHeight);
      }
   }
}


void
VulkanRenderer::acquireScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   auto imageBarrier = vk::ImageMemoryBarrier { };
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

   auto imageInfo = vk::DescriptorImageInfo { };
   imageInfo.imageView = imageView;
   imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

   auto descWriteDesc = vk::WriteDescriptorSet { };
   descWriteDesc.dstSet = descriptorSet;
   descWriteDesc.dstBinding = 1;
   descWriteDesc.dstArrayElement = 0;
   descWriteDesc.descriptorCount = 1;
   descWriteDesc.descriptorType = vk::DescriptorType::eSampledImage;
   descWriteDesc.pImageInfo = &imageInfo;
   mDevice.updateDescriptorSets({ descWriteDesc }, {});
}

void
VulkanRenderer::renderScanBuffer(vk::Viewport viewport, vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   cmdBuffer.setViewport(0, { viewport });
   cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 0, { descriptorSet }, {});
   cmdBuffer.bindVertexBuffers(0, { mVertexBuffer }, { 0 });
   cmdBuffer.draw(6, 1, 0, 0);
}

void
VulkanRenderer::releaseScanBuffer(vk::CommandBuffer cmdBuffer, vk::DescriptorSet descriptorSet, vk::Image image, vk::ImageView imageView)
{
   auto imageBarrier = vk::ImageMemoryBarrier { };
   imageBarrier.srcAccessMask = vk::AccessFlags { };
   imageBarrier.dstAccessMask = vk::AccessFlags { };
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

void VulkanRenderer::titleLoaded(quint64 id, const QString &name)
{
   mGameLoaded = true;
}

void VulkanRenderer::settingsChanged()
{
   auto settings = mSettingsStorage->get();
   mDisplaySettings = settings->display;

   // Setup background colour
   mBackgroundColour.float32[0] = static_cast<float>(mDisplaySettings.backgroundColour.redF());
   mBackgroundColour.float32[1] = static_cast<float>(mDisplaySettings.backgroundColour.greenF());
   mBackgroundColour.float32[2] = static_cast<float>(mDisplaySettings.backgroundColour.blueF());

   // TODO: Only do this for SRGB surface
   // Apply some gamma correction
   mBackgroundColour.float32[0] = pow(mBackgroundColour.float32[0], 2.2f);
   mBackgroundColour.float32[1] = pow(mBackgroundColour.float32[1], 2.2f);
   mBackgroundColour.float32[2] = pow(mBackgroundColour.float32[2], 2.2f);
}

void VulkanRenderer::startNextFrame()
{
   const QSize sz = m_window->swapChainImageSize();
   auto renderCmdBuf = vk::CommandBuffer { m_window->currentCommandBuffer() };
   renderCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

   auto renderPassBeginInfo = vk::RenderPassBeginInfo { };
   renderPassBeginInfo.renderPass = m_window->defaultRenderPass();
   renderPassBeginInfo.framebuffer = m_window->currentFramebuffer();
   renderPassBeginInfo.renderArea = vk::Rect2D({ 0, 0 }, { static_cast<uint32_t>(sz.width()), static_cast<uint32_t>(sz.height()) });
   vk::ClearValue clearValues[] = {
      mBackgroundColour, vk::ClearDepthStencilValue { }
   };
   renderPassBeginInfo.clearValueCount = 2;
   renderPassBeginInfo.pClearValues = clearValues;

   vk::Image tvImage, drcImage;
   vk::ImageView tvView, drcView;

   // Select some descriptors to use
   auto &frameResources = mFrameResources[mFrameIndex];
   auto descriptorSetTv = frameResources.tvDesc;
   auto descriptorSetDrc = frameResources.drcDesc;

   if (mGameLoaded) {
      // Update viewport, TODO: Maybe only update on resizeEvent?
      calculateScreenViewports(mTvViewport, mGamepad1Viewport);

      // Grab the scan buffers...
      mDecafDriver->getSwapBuffers(tvImage, tvView, drcImage, drcView);

      if (tvImage) {
         acquireScanBuffer(renderCmdBuf, descriptorSetTv, tvImage, tvView);
      }
      if (drcImage) {
         acquireScanBuffer(renderCmdBuf, descriptorSetDrc, drcImage, drcView);
      }
      renderCmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      if (tvImage) {
         renderScanBuffer(mTvViewport, renderCmdBuf, descriptorSetTv, tvImage, tvView);
      }

      if (drcImage) {
         renderScanBuffer(mGamepad1Viewport, renderCmdBuf, descriptorSetDrc, drcImage, drcView);
      }

      // Draw the debug UI
      if (mDebugUi) {
         mDebugUi->draw(renderCmdBuf, sz.width(), sz.height());
      }
   } else {
      renderCmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
   }

   renderCmdBuf.endRenderPass();

   if (tvImage) {
      releaseScanBuffer(renderCmdBuf, descriptorSetTv, tvImage, tvView);
   }
   if (drcImage) {
      releaseScanBuffer(renderCmdBuf, descriptorSetDrc, drcImage, drcView);
   }

   // Increment our frame index counter
   mFrameIndex = (mFrameIndex + 1) % mFrameResources.size();

   m_window->frameReady();
   m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

/*

// TODO: Clipboard shite

using ClipboardTextGetCallback = const char *(*)(void *);
using ClipboardTextSetCallback = void (*)(void *, const char*);

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter);
*/

static debugui::KeyboardKey
translateKeyCode(QKeyEvent *ev)
{
   auto key = ev->key();
   switch (key) {
   case Qt::Key_Tab:
      return debugui::KeyboardKey::Tab;
   case Qt::Key_Left:
      return debugui::KeyboardKey::LeftArrow;
   case Qt::Key_Right:
      return debugui::KeyboardKey::RightArrow;
   case Qt::Key_Up:
      return debugui::KeyboardKey::UpArrow;
   case Qt::Key_Down:
      return debugui::KeyboardKey::DownArrow;
   case Qt::Key_PageUp:
      return debugui::KeyboardKey::PageUp;
   case Qt::Key_PageDown:
      return debugui::KeyboardKey::PageDown;
   case Qt::Key_Home:
      return debugui::KeyboardKey::Home;
   case Qt::Key_End:
      return debugui::KeyboardKey::End;
   case Qt::Key_Delete:
      return debugui::KeyboardKey::Delete;
   case Qt::Key_Backspace:
      return debugui::KeyboardKey::Backspace;
   case Qt::Key_Return:
      return debugui::KeyboardKey::Enter;
   case Qt::Key_Escape:
      return debugui::KeyboardKey::Escape;
   case Qt::Key_Control:
      return debugui::KeyboardKey::LeftControl;
   case Qt::Key_Shift:
      return debugui::KeyboardKey::LeftShift;
   case Qt::Key_Alt:
      return debugui::KeyboardKey::LeftAlt;
   case Qt::Key_AltGr:
      return debugui::KeyboardKey::RightAlt;
   case Qt::Key_Super_L:
      return debugui::KeyboardKey::LeftSuper;
   case Qt::Key_Super_R:
      return debugui::KeyboardKey::RightSuper;
   default:
      if (key >= Qt::Key_A && key <= Qt::Key_Z) {
         auto id = (key - Qt::Key_A) + static_cast<int>(debugui::KeyboardKey::A);
         return static_cast<debugui::KeyboardKey>(id);
      } else if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
         auto id = (key - Qt::Key_F1) + static_cast<int>(debugui::KeyboardKey::F1);
         return static_cast<debugui::KeyboardKey>(id);
      }

      return debugui::KeyboardKey::Unknown;
   }
}

void VulkanRenderer::keyPressEvent(QKeyEvent *ev)
{
   if (mDebugUi && mDebugUi->onKeyAction(translateKeyCode(ev), debugui::KeyboardAction::Press)) {
      if (auto text = ev->text(); !text.isEmpty()) {
         mDebugUi->onText(text.toUtf8().data());
      }

      return;
   }

   mInputDriver->keyPressEvent(ev->key());
}

void VulkanRenderer::keyReleaseEvent(QKeyEvent *ev)
{
   if (mDebugUi && mDebugUi->onKeyAction(translateKeyCode(ev), debugui::KeyboardAction::Release)) {
      return;
   }

   mInputDriver->keyReleaseEvent(ev->key());
}

void VulkanRenderer::mousePressEvent(QMouseEvent *ev)
{
   auto button = debugui::MouseButton::Left;

   if (ev->button() == Qt::MouseButton::RightButton) {
      button = debugui::MouseButton::Right;
   } else if (ev->button() == Qt::MouseButton::MiddleButton) {
      button = debugui::MouseButton::Middle;
   }

   if (mDebugUi && mDebugUi->onMouseAction(button, debugui::MouseAction::Press)) {
      return;
   }

   auto windowX = static_cast<float>(ev->x());
   auto windowY = static_cast<float>(ev->y());
   if (windowX >= mGamepad1Viewport.x &&
       windowX < mGamepad1Viewport.x + mGamepad1Viewport.width &&
       windowY >= mGamepad1Viewport.y &&
       windowY < mGamepad1Viewport.y + mGamepad1Viewport.height) {
      auto x = (windowX - mGamepad1Viewport.x) / mGamepad1Viewport.width;
      auto y = (windowY - mGamepad1Viewport.y) / mGamepad1Viewport.height;
      mInputDriver->gamepadTouchEvent(true, x, y);
   }
}

void VulkanRenderer::mouseReleaseEvent(QMouseEvent *ev)
{
   auto button = debugui::MouseButton::Left;

   if (ev->button() == Qt::MouseButton::RightButton) {
      button = debugui::MouseButton::Right;
   } else if (ev->button() == Qt::MouseButton::MiddleButton) {
      button = debugui::MouseButton::Middle;
   }

   if (mDebugUi && mDebugUi->onMouseAction(button, debugui::MouseAction::Release)) {
      return;
   }

   auto windowX = static_cast<float>(ev->x());
   auto windowY = static_cast<float>(ev->y());
   if (windowX >= mGamepad1Viewport.x &&
       windowX < mGamepad1Viewport.x + mGamepad1Viewport.width &&
       windowY >= mGamepad1Viewport.y &&
       windowY < mGamepad1Viewport.y + mGamepad1Viewport.height) {
      auto x = (windowX - mGamepad1Viewport.x) / mGamepad1Viewport.width;
      auto y = (windowY - mGamepad1Viewport.y) / mGamepad1Viewport.height;
      mInputDriver->gamepadTouchEvent(false, x, y);
   }
}

void VulkanRenderer::mouseMoveEvent(QMouseEvent *ev)
{
   auto windowX = static_cast<float>(ev->x());
   auto windowY = static_cast<float>(ev->y());
   if (mDebugUi && mDebugUi->onMouseMove(windowX, windowY)) {
      return;
   }

   if (ev->buttons() & Qt::LeftButton) {
      if (windowX >= mGamepad1Viewport.x &&
          windowX < mGamepad1Viewport.x + mGamepad1Viewport.width &&
          windowY >= mGamepad1Viewport.y &&
          windowY < mGamepad1Viewport.y + mGamepad1Viewport.height) {
         auto x = (windowX - mGamepad1Viewport.x) / mGamepad1Viewport.width;
         auto y = (windowY - mGamepad1Viewport.y) / mGamepad1Viewport.height;
         mInputDriver->gamepadTouchEvent(true, x, y);
      }
   }
}

void VulkanRenderer::wheelEvent(QWheelEvent *ev)
{
   auto delta = ev->angleDelta();
   if (mDebugUi && mDebugUi->onMouseScroll(delta.x() / 120.0f, delta.y() / 120.0f)) {
      return;
   }
}

void VulkanRenderer::touchEvent(QTouchEvent *ev)
{
   // TODO: Translate touch events?
}
