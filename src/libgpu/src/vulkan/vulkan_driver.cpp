#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_event.h"
#include "gpu_graphicsdriver.h"
#include "gpu_ringbuffer.h"

namespace vulkan
{

Driver::Driver()
{
}

Driver::~Driver()
{
}

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Vulkan;
}

void
Driver::notifyCpuFlush(phys_addr address,
                       uint32_t size)
{
}

void
Driver::notifyGpuFlush(phys_addr address,
                       uint32_t size)
{
}

void
Driver::initialise(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, uint32_t queueFamilyIndex)
{
   if (mRunState != RunState::None) {
      return;
   }

   mPhysDevice = physDevice;
   mDevice = device;
   mQueue = queue;
   mRunState = RunState::Running;

   validateDevice();

   // Initialize the dynamic loader we use for extensions
   mVkDynLoader.init(nullptr, mDevice);

   // Allocate a command pool to use
   vk::CommandPoolCreateInfo commandPoolDesc;
   commandPoolDesc.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
   commandPoolDesc.queueFamilyIndex = queueFamilyIndex;
   mCommandPool = mDevice.createCommandPool(commandPoolDesc);

   // Start our fence thread
   mFenceThread = std::thread(std::bind(&Driver::fenceWaiterThread, this));

   // Set up the VMA
   VmaAllocatorCreateInfo allocatorDesc = {};
   allocatorDesc.physicalDevice = mPhysDevice;
   allocatorDesc.device = mDevice;
   vmaCreateAllocator(&allocatorDesc, &mAllocator);

   // Set up our drawing pipeline layout
   auto makeStageSet = [&](int stageIndex)
   {
      vk::ShaderStageFlags stageFlags;
      if (stageIndex == 0) {
         stageFlags = vk::ShaderStageFlagBits::eVertex;
      } else if (stageIndex == 1) {
         stageFlags = vk::ShaderStageFlagBits::eGeometry;
      } else if (stageIndex == 2) {
         stageFlags = vk::ShaderStageFlagBits::eFragment;
      } else {
         decaf_abort("Unexpected shader stage index");
      }

      std::vector<vk::DescriptorSetLayoutBinding> bindings;

      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         vk::DescriptorSetLayoutBinding sampBindingDesc;
         sampBindingDesc.binding = i;
         sampBindingDesc.descriptorType = vk::DescriptorType::eSampler;
         sampBindingDesc.descriptorCount = 1;
         sampBindingDesc.stageFlags = stageFlags;
         sampBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(sampBindingDesc);
      }

      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         vk::DescriptorSetLayoutBinding texBindingDesc;
         texBindingDesc.binding = latte::MaxSamplers + i;
         texBindingDesc.descriptorType = vk::DescriptorType::eSampledImage;
         texBindingDesc.descriptorCount = 1;
         texBindingDesc.stageFlags = stageFlags;
         texBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(texBindingDesc);
      }

      for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
         if (i >= 15) {
            // Vulkan does not support more than 15 uniform blocks unfortunately,
            // if we ever encounter a game needing all 15, we will need to do block
            // splitting or utilize SSBO's.
            break;
         }

         vk::DescriptorSetLayoutBinding cbufferBindingDesc;
         cbufferBindingDesc.binding = latte::MaxSamplers + latte::MaxTextures + i;
         cbufferBindingDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
         cbufferBindingDesc.descriptorCount = 1;
         cbufferBindingDesc.stageFlags = stageFlags;
         cbufferBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(cbufferBindingDesc);
      }

      vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutDesc;
      descriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(bindings.size());
      descriptorSetLayoutDesc.pBindings = bindings.data();
      return mDevice.createDescriptorSetLayout(descriptorSetLayoutDesc);
   };

   mVertexDescriptorSetLayout = makeStageSet(0);
   mGeometryDescriptorSetLayout = makeStageSet(1);
   mPixelDescriptorSetLayout = makeStageSet(2);

   std::array<vk::DescriptorSetLayout, 3> descriptorLayouts = {
      mVertexDescriptorSetLayout,
      mGeometryDescriptorSetLayout,
      mPixelDescriptorSetLayout };


   std::vector<vk::PushConstantRange> pushConstants;

   vk::PushConstantRange screenSpaceConstants;
   screenSpaceConstants.stageFlags = vk::ShaderStageFlagBits::eVertex;
   screenSpaceConstants.offset = 0;
   screenSpaceConstants.size = 32;
   pushConstants.push_back(screenSpaceConstants);

   vk::PushConstantRange alphaRefConstants;
   alphaRefConstants.stageFlags = vk::ShaderStageFlagBits::eFragment;
   alphaRefConstants.offset = 32;
   alphaRefConstants.size = 12;
   pushConstants.push_back(alphaRefConstants);

   vk::PipelineLayoutCreateInfo pipelineLayoutDesc;
   pipelineLayoutDesc.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
   pipelineLayoutDesc.pSetLayouts = descriptorLayouts.data();
   pipelineLayoutDesc.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
   pipelineLayoutDesc.pPushConstantRanges = pushConstants.data();
   mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutDesc);
}

vk::DescriptorPool
Driver::allocateDescriptorPool(uint32_t numDraws)
{
   std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eSampler, latte::MaxSamplers * numDraws),
      vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, latte::MaxTextures * numDraws),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, latte::MaxAttribBuffers * numDraws),
   };

   vk::DescriptorPoolCreateInfo descriptorPoolInfo;
   descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
   descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets = static_cast<uint32_t>(numDraws);
   auto descriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

   mDescriptorPools.push_back(descriptorPool);

   return descriptorPool;
}

vk::DescriptorSet
Driver::allocateGenericDescriptorSet(vk::DescriptorSetLayout &setLayout)
{
   static const uint32_t maxPoolDraws = 32;

   if (!mActiveDescriptorPool || mActiveDescriptorPoolDrawsLeft == 0) {
      mActiveDescriptorPool = allocateDescriptorPool(maxPoolDraws);
      mActiveDescriptorPoolDrawsLeft = maxPoolDraws;
   }

   vk::DescriptorSetAllocateInfo allocInfo;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &setLayout;
   allocInfo.descriptorPool = mActiveDescriptorPool;
   mActiveDescriptorPoolDrawsLeft--;

   return mDevice.allocateDescriptorSets(allocInfo)[0];
}

vk::DescriptorSet
Driver::allocateVertexDescriptorSet()
{
   vk::DescriptorSet descriptorSet;

   if (!mVertexDescriptorSets.empty()) {
      descriptorSet = mVertexDescriptorSets.back();
      mVertexDescriptorSets.pop_back();
   }

   if (!descriptorSet) {
      descriptorSet = allocateGenericDescriptorSet(mVertexDescriptorSetLayout);
   }

   mActiveSyncWaiter->vertexDescriptorSets.push_back(descriptorSet);
   return descriptorSet;
}

vk::DescriptorSet
Driver::allocateGeometryDescriptorSet()
{
   vk::DescriptorSet descriptorSet;

   if (!mGeometryDescriptorSets.empty()) {
      descriptorSet = mGeometryDescriptorSets.back();
      mGeometryDescriptorSets.pop_back();
   }

   if (!descriptorSet) {
      descriptorSet = allocateGenericDescriptorSet(mGeometryDescriptorSetLayout);
   }

   mActiveSyncWaiter->geometryDescriptorSets.push_back(descriptorSet);
   return descriptorSet;
}

vk::DescriptorSet
Driver::allocatePixelDescriptorSet()
{
   vk::DescriptorSet descriptorSet;

   if (!mPixelDescriptorSets.empty()) {
      descriptorSet = mPixelDescriptorSets.back();
      mPixelDescriptorSets.pop_back();
   }

   if (!descriptorSet) {
      descriptorSet = allocateGenericDescriptorSet(mPixelDescriptorSetLayout);
   }

   mActiveSyncWaiter->pixelDescriptorSets.push_back(descriptorSet);
   return descriptorSet;
}

void
Driver::retireVertexDescriptorSet(vk::DescriptorSet descriptorSet)
{
   mVertexDescriptorSets.push_back(descriptorSet);
}

void
Driver::retireGeometryDescriptorSet(vk::DescriptorSet descriptorSet)
{
   mGeometryDescriptorSets.push_back(descriptorSet);
}

void
Driver::retirePixelDescriptorSet(vk::DescriptorSet descriptorSet)
{
   mPixelDescriptorSets.push_back(descriptorSet);
}

void
Driver::shutdown()
{
   mFenceSignal.notify_all();
   mFenceThread.join();
}

void
Driver::getSwapBuffers(vk::Image &tvImage, vk::ImageView &tvView, vk::Image &drcImage, vk::ImageView &drcView)
{
   if (mTvSwapChain && mTvSwapChain->presentable) {
      tvImage = mTvSwapChain->image;
      tvView = mTvSwapChain->imageView;
   } else {
      tvImage = vk::Image();
      tvView = vk::ImageView();
   }

   if (mDrcSwapChain && mDrcSwapChain->presentable) {
      drcImage = mDrcSwapChain->image;
      drcView = mDrcSwapChain->imageView;
   } else {
      drcImage = vk::Image();
      drcView = vk::ImageView();
   }
}

void
Driver::run()
{
   while (mRunState == RunState::Running) {
      // Grab the next buffer
      gpu::ringbuffer::wait();
      auto buffer = gpu::ringbuffer::read();

      // Check for any fences completing
      checkSyncFences();

      // Process the buffer if there is anything new
      if (!buffer.empty()) {
         executeBuffer(buffer);
      }
   }
}

void
Driver::stop()
{
   mRunState = RunState::Stopped;
   gpu::ringbuffer::wake();
}

void
Driver::runUntilFlip()
{
   auto startingSwap = mLastSwap;

   while (mRunState == RunState::Running) {
      // Grab the next item
      gpu::ringbuffer::wait();

      // Check for any fences completing
      checkSyncFences();

      // Process the buffer if there is anything new
      auto buffer = gpu::ringbuffer::read();
      if (!buffer.empty()) {
         executeBuffer(buffer);
      }

      if (mLastSwap > startingSwap) {
         break;
      }
   }
}

void
Driver::beginCommandGroup()
{
   mActivePm4BufferIndex++;

   mActiveSyncWaiter = allocateSyncWaiter();
   mActiveCommandBuffer = mActiveSyncWaiter->cmdBuffer;
}

void
Driver::endCommandGroup()
{
   // Submit the active waiter to the queue
   submitSyncWaiter(mActiveSyncWaiter);

   // Clear our state in between command buffers for safety
   mActiveCommandBuffer = nullptr;
   mActiveSyncWaiter = nullptr;
   mActiveDescriptorPool = vk::DescriptorPool();
   mActivePipeline = nullptr;
   mActiveRenderPass = nullptr;
}

void
Driver::beginCommandBuffer()
{
   // Begin recording our host command buffer
   mActiveCommandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
}

void
Driver::endCommandBuffer()
{
   // We have to force our memcache objects to be downloaded at the
   // end of every PM4 buffer.
   downloadPendingMemCache();

   // Stop recording this host command buffer
   mActiveCommandBuffer.end();
}

int32_t
Driver::findMemoryType(uint32_t memTypeBits, vk::MemoryPropertyFlags requestProps)
{
   auto memoryProps = mPhysDevice.getMemoryProperties();

   const uint32_t memoryCount = memoryProps.memoryTypeCount;
   for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
      const uint32_t memoryTypeBits = (1 << memoryIndex);
      const bool isRequiredMemoryType = memTypeBits & memoryTypeBits;

      const auto properties = memoryProps.memoryTypes[memoryIndex].propertyFlags;
      const bool hasRequiredProperties = (properties & requestProps) == requestProps;

      if (isRequiredMemoryType && hasRequiredProperties)
         return static_cast<int32_t>(memoryIndex);
   }

   throw std::logic_error("failed to find suitable memory type");
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
