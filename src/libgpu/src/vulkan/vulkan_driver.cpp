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

   // Allocate a command pool to use
   vk::CommandPoolCreateInfo commandPoolDesc;
   commandPoolDesc.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
   commandPoolDesc.queueFamilyIndex = queueFamilyIndex;
   mCommandPool = mDevice.createCommandPool(commandPoolDesc);

   // Start our fence thread
   mFenceThread = std::thread(std::bind(&Driver::fenceWaiterThread, this));
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
   if (mTvSwapChain) {
      tvImage = mTvSwapChain->image;
      tvView = mTvSwapChain->imageView;
   } else {
      tvImage = vk::Image();
      tvView = vk::ImageView();
   }
   
   if (mDrcSwapChain) {
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
      // Grab the next item
      auto item = gpu::ringbuffer::waitForItem();

      // Check for any fences completing
      checkSyncFences();

      // Process the buffer if there is anything new
      if (item.numWords) {
         executeBuffer(item);
      }
   }
}

void
Driver::stop()
{
   mRunState = RunState::Stopped;
   gpu::ringbuffer::awaken();
}

void
Driver::runUntilFlip()
{
   auto startingSwap = mLastSwap;

   while (mRunState == RunState::Running) {
      // Grab the next item
      auto item = gpu::ringbuffer::waitForItem();

      // Check for any fences completing
      checkSyncFences();

      // Process the buffer if there is anything new
      if (item.numWords) {
         executeBuffer(item);
      }

      if (mLastSwap > startingSwap) {
         break;
      }
   }
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

#endif // DECAF_VULKAN
