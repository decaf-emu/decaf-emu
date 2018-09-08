#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_event.h"

namespace vulkan
{

void
Driver::decafSetBuffer(const latte::pm4::DecafSetBuffer &data)
{
   SwapChainObject **swapChain;
   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      swapChain = &mTvSwapChain;
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
      swapChain = &mDrcSwapChain;
   } else {
      decaf_abort("Unexpected decafSetBuffer target");
   }

   // Release the existing swap chain if we had it...
   if (*swapChain) {
      releaseSwapChain(*swapChain);
      *swapChain = nullptr;
   }

   // Allocate a new swap chain
   auto swapChainInfo = SwapChainInfo {
      data.buffer,
      data.width,
      data.height
   };
   auto newSwapChain = allocateSwapChain(swapChainInfo);

   // Assign the new swap chain
   *swapChain = newSwapChain;
}

void
Driver::decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data)
{
   auto surface = getColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info, false);

   SwapChainObject *target = nullptr;
   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      target = mTvSwapChain;
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
      target = mDrcSwapChain;
   } else {
      decaf_abort("decafCopyColorToScan called for unknown scanTarget");
   }

   transitionSurface(surface, vk::ImageLayout::eTransferSrcOptimal);

   vk::ImageBlit blitRegion(
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(surface->info.width, surface->info.height, 1) },
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(target->info.width, target->info.height, 1) });

   mActiveCommandBuffer.blitImage(
      surface->image,
      vk::ImageLayout::eTransferSrcOptimal,
      target->image,
      vk::ImageLayout::eTransferDstOptimal,
      { blitRegion },
      vk::Filter::eNearest);
}

void
Driver::decafSwapBuffers(const latte::pm4::DecafSwapBuffers &data)
{
   static const auto weight = 0.9;

   addRetireTask([=](){
      // Send out the flip event
      gpu::onFlip();

      // Update our frametime and last swap times
      auto now = std::chrono::system_clock::now();

      if (mLastSwap.time_since_epoch().count()) {
         mAverageFrameTime = weight * mAverageFrameTime + (1.0 - weight) * (now - mLastSwap);
      }

      mLastSwap = now;

      // Update our debugging info every flip
      updateDebuggerInfo();
   });
}

void
Driver::decafCapSyncRegisters(const latte::pm4::DecafCapSyncRegisters &data)
{
}

void
Driver::decafClearColor(const latte::pm4::DecafClearColor &data)
{
   // Find our colorbuffer to clear
   auto surface = getColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info, true);

   transitionSurface(surface, vk::ImageLayout::eGeneral);

   std::array<float, 4> clearColor = { data.red, data.green, data.blue, data.alpha };
   mActiveCommandBuffer.clearColorImage(surface->image, vk::ImageLayout::eGeneral, clearColor, { surface->subresRange });
}

void
Driver::decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data)
{
}

void
Driver::decafDebugMarker(const latte::pm4::DecafDebugMarker &data)
{
}

void
Driver::decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data)
{
}

void
Driver::decafCopySurface(const latte::pm4::DecafCopySurface &data)
{
}

void
Driver::decafSetSwapInterval(const latte::pm4::DecafSetSwapInterval &data)
{
}

void
Driver::drawIndexAuto(const latte::pm4::DrawIndexAuto &data)
{
}

void
Driver::drawIndex2(const latte::pm4::DrawIndex2 &data)
{
}

void
Driver::drawIndexImmd(const latte::pm4::DrawIndexImmd &data)
{
}

void
Driver::memWrite(const latte::pm4::MemWrite &data)
{
}

void
Driver::eventWrite(const latte::pm4::EventWrite &data)
{
}

void
Driver::eventWriteEOP(const latte::pm4::EventWriteEOP &data)
{
}

void
Driver::pfpSyncMe(const latte::pm4::PfpSyncMe &data)
{
}

void
Driver::streamOutBaseUpdate(const latte::pm4::StreamOutBaseUpdate &data)
{
}

void
Driver::streamOutBufferUpdate(const latte::pm4::StreamOutBufferUpdate &data)
{
}

void
Driver::surfaceSync(const latte::pm4::SurfaceSync &data)
{
}

void
Driver::applyRegister(latte::Register reg)
{
   // Vulkan driver never directly applies register values
}


void
Driver::executeBuffer(const gpu::ringbuffer::Item &item)
{
   decaf_check(!mActiveSyncWaiter);

   mActiveSyncWaiter = allocateSyncWaiter();
   mActiveCommandBuffer = mActiveSyncWaiter->cmdBuffer;

   // Begin recording our host command buffer
   mActiveCommandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

   // Execute guest PM4 command buffer
   runCommandBuffer(item.buffer.getRawPointer(), item.numWords);

   // Add a callback to retire the PM4 buffer once the host command buffer is retired
   addRetireTask(std::bind(gpu::onRetire, item.context));

   // Stop recording this host command buffer
   mActiveCommandBuffer.end();

   // Submit the generated command buffer to the host GPU queue
   vk::SubmitInfo submitInfo;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &mActiveCommandBuffer;
   mQueue.submit({ submitInfo }, mActiveSyncWaiter->fence);

   // Submit the active waiter to the queue
   submitSyncWaiter(mActiveSyncWaiter);

   // Clear our state in between command buffers for safety
   mActiveCommandBuffer = nullptr;
   mActiveSyncWaiter = nullptr;
}

} // namespace vulkan

#endif // DECAF_VULKAN
