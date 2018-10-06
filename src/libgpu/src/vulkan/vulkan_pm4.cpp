#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_clock.h"
#include "gpu_event.h"
#include "gpu_memory.h"

#include "latte/latte_endian.h"

#include <common/decaf_assert.h>
#include <common/log.h>

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
   auto swapChainDesc = SwapChainDesc {
      data.buffer,
      data.width,
      data.height
   };
   auto newSwapChain = allocateSwapChain(swapChainDesc);

   // Assign the new swap chain
   *swapChain = newSwapChain;
}

void
Driver::decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data)
{
   ColorBufferDesc colorBuffer;
   colorBuffer.base256b = data.cb_color_base.BASE_256B();
   colorBuffer.pitchTileMax = data.cb_color_size.PITCH_TILE_MAX();
   colorBuffer.sliceTileMax = data.cb_color_size.SLICE_TILE_MAX();
   colorBuffer.format = data.cb_color_info.FORMAT();
   colorBuffer.numberType = data.cb_color_info.NUMBER_TYPE();
   colorBuffer.arrayMode = data.cb_color_info.ARRAY_MODE();
   auto surface = getColorBuffer(colorBuffer, false);

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
      { vk::Offset3D(0, 0, 0), vk::Offset3D(surface->desc.width, surface->desc.height, 1) },
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(target->desc.width, target->desc.height, 1) });

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
   ColorBufferDesc colorBuffer;
   colorBuffer.base256b = data.cb_color_base.BASE_256B();
   colorBuffer.pitchTileMax = data.cb_color_size.PITCH_TILE_MAX();
   colorBuffer.sliceTileMax = data.cb_color_size.SLICE_TILE_MAX();
   colorBuffer.format = data.cb_color_info.FORMAT();
   colorBuffer.numberType = data.cb_color_info.NUMBER_TYPE();
   colorBuffer.arrayMode = data.cb_color_info.ARRAY_MODE();
   auto surface = getColorBuffer(colorBuffer, true);

   transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

   std::array<float, 4> clearColor = { data.red, data.green, data.blue, data.alpha };
   mActiveCommandBuffer.clearColorImage(surface->image, vk::ImageLayout::eTransferDstOptimal, clearColor, { surface->subresRange });
}

void
Driver::decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data)
{
   // Find our depthbuffer to clear
   DepthStencilBufferDesc depthBuffer;
   depthBuffer.base256b = data.db_depth_base.BASE_256B();
   depthBuffer.pitchTileMax = data.db_depth_size.PITCH_TILE_MAX();
   depthBuffer.sliceTileMax = data.db_depth_size.SLICE_TILE_MAX();
   depthBuffer.format = data.db_depth_info.FORMAT();
   depthBuffer.arrayMode = data.db_depth_info.ARRAY_MODE();
   auto surface = getDepthStencilBuffer(depthBuffer, true);

   transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

   auto db_depth_clear = getRegister<latte::DB_DEPTH_CLEAR>(latte::Register::DB_DEPTH_CLEAR);
   auto db_stencil_clear = getRegister<latte::DB_STENCIL_CLEAR>(latte::Register::DB_STENCIL_CLEAR);

   vk::ClearDepthStencilValue clearDepthStencil;
   clearDepthStencil.depth = db_depth_clear.DEPTH_CLEAR();
   clearDepthStencil.stencil = db_stencil_clear.CLEAR();
   mActiveCommandBuffer.clearDepthStencilImage(
      surface->image, vk::ImageLayout::eTransferDstOptimal, clearDepthStencil, { surface->subresRange });
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
   drawGenericIndexed(data.count, nullptr);
}

void
Driver::drawIndex2(const latte::pm4::DrawIndex2 &data)
{
   drawGenericIndexed(data.count, phys_cast<void*>(data.addr).getRawPointer());
}

void
Driver::drawIndexImmd(const latte::pm4::DrawIndexImmd &data)
{
   drawGenericIndexed(data.count, data.indices.data());
}

void
Driver::memWrite(const latte::pm4::MemWrite &data)
{
   auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
   auto ptr = gpu::internal::translateAddress(addr);
   auto value = uint64_t { 0 };

   // Read value
   if (data.addrHi.CNTR_SEL() == latte::pm4::MW_WRITE_CLOCK) {
      value = gpu::clock::now();
   } else if (data.addrHi.DATA32()) {
      value = static_cast<uint64_t>(data.dataLo);
   } else {
      value = static_cast<uint64_t>(data.dataLo) |
              (static_cast<uint64_t>(data.dataHi) << 32);
   }

   // Swap value
   value = latte::applyEndianSwap(value, data.addrLo.ENDIAN_SWAP());

   // Write value
   if (data.addrHi.DATA32()) {
      *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
   } else {
      *reinterpret_cast<uint64_t *>(ptr) = value;
   }
}

void
Driver::eventWrite(const latte::pm4::EventWrite &data)
{
}

void
Driver::eventWriteEOP(const latte::pm4::EventWriteEOP &data)
{
   // Write event data to memory if required
   if (data.addrHi.DATA_SEL() != latte::pm4::EWP_DATA_DISCARD) {
      auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
      auto ptr = gpu::internal::translateAddress(addr);
      decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

      // Read value
      auto value = uint64_t { 0u };
      switch (data.addrHi.DATA_SEL()) {
      case latte::pm4::EWP_DATA_32:
         value = data.dataLo;
         break;
      case latte::pm4::EWP_DATA_64:
         value = static_cast<uint64_t>(data.dataLo) |
                 (static_cast<uint64_t>(data.dataHi) << 32);
         break;
      case latte::pm4::EWP_DATA_CLOCK:
         value = gpu::clock::now();
         break;
      }

      // Swap value
      value = latte::applyEndianSwap(value, data.addrLo.ENDIAN_SWAP());

      // Write value
      switch (data.addrHi.DATA_SEL()) {
      case latte::pm4::EWP_DATA_32:
         *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
         break;
      case latte::pm4::EWP_DATA_64:
      case latte::pm4::EWP_DATA_CLOCK:
         *reinterpret_cast<uint64_t *>(ptr) = value;
         break;
      }
   }

   // TODO: Generate interrupt if required
   if (data.addrHi.INT_SEL() != latte::pm4::EWP_INT_NONE) {
   }
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

   // Begin our command group (sync waiter)
   beginCommandGroup();

   // Begin preparing our command buffer
   beginCommandBuffer();

   // Execute guest PM4 command buffer
   runCommandBuffer(item.buffer.getRawPointer(), item.numWords);

   // Add a callback to retire the PM4 buffer once the host command buffer is retired
   addRetireTask(std::bind(gpu::onRetire, item.context));

   // End preparing our command buffer
   endCommandBuffer();

   // Submit the generated command buffer to the host GPU queue
   vk::SubmitInfo submitInfo;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &mActiveCommandBuffer;
   mQueue.submit({ submitInfo }, mActiveSyncWaiter->fence);

   // End our command group
   endCommandGroup();
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
