#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_clock.h"
#include "gpu_event.h"
#include "gpu_ih.h"
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

   // Give the swapchain a name so its easy to see.
   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      setVkObjectName(newSwapChain->image, fmt::format("swapchain_tv").c_str());
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
      setVkObjectName(newSwapChain->image, fmt::format("swapchain_drc").c_str());
   } else {
         decaf_abort("Unexpected decafSetBuffer target");
   }

   // Assign the new swap chain
   *swapChain = newSwapChain;

   // Only make this swapchain presentable after this frame has completed
   // (we need to run the frame at least once for setup to complete before use).
   addRetireTask([=](){
      newSwapChain->presentable = true;
   });
}

void
Driver::decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data)
{
   flushPendingDraws();

   ColorBufferDesc colorBuffer;
   colorBuffer.base256b = data.cb_color_base.BASE_256B();
   colorBuffer.pitchTileMax = data.cb_color_size.PITCH_TILE_MAX();
   colorBuffer.sliceTileMax = data.cb_color_size.SLICE_TILE_MAX();
   colorBuffer.format = data.cb_color_info.FORMAT();
   colorBuffer.numberType = data.cb_color_info.NUMBER_TYPE();
   colorBuffer.arrayMode = data.cb_color_info.ARRAY_MODE();
   colorBuffer.sliceStart = 0;
   colorBuffer.sliceEnd = 1;
   auto surfaceView = getColorBuffer(colorBuffer);
   auto surface = surfaceView->surface;

   SwapChainObject *target = nullptr;
   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      target = mTvSwapChain;
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
      target = mDrcSwapChain;
   } else {
      decaf_abort("decafCopyColorToScan called for unknown scanTarget");
   }

   transitionSurface(surface, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, { 0, 1 });

   // TODO: We actually need to call AVMSetTVScale inside of the SetBuffer functions
   // and then pass that data all the way down to here so we can scale correctly.
   auto copyWidth = target->desc.width;
   auto copyHeight = target->desc.height;
   if (surface->desc.width < target->desc.width) {
      copyWidth = surface->desc.width;
   }
   if (surface->desc.height < target->desc.height) {
      copyHeight = surface->desc.height;
   }

   vk::ImageBlit blitRegion(
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(copyWidth, copyHeight, 1) },
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
   gpu::onSyncRegisters(mRegisters.data(), static_cast<uint32_t>(mRegisters.size()));
}

void
Driver::decafClearColor(const latte::pm4::DecafClearColor &data)
{
   flushPendingDraws();

   // Find our colorbuffer to clear
   ColorBufferDesc colorBuffer;
   colorBuffer.base256b = data.cb_color_base.BASE_256B();
   colorBuffer.pitchTileMax = data.cb_color_size.PITCH_TILE_MAX();
   colorBuffer.sliceTileMax = data.cb_color_size.SLICE_TILE_MAX();
   colorBuffer.format = data.cb_color_info.FORMAT();
   colorBuffer.numberType = data.cb_color_info.NUMBER_TYPE();
   colorBuffer.arrayMode = data.cb_color_info.ARRAY_MODE();
   colorBuffer.sliceStart = data.cb_color_view.SLICE_START();
   colorBuffer.sliceEnd = data.cb_color_view.SLICE_MAX() + 1;
   auto surfaceView = getColorBuffer(colorBuffer);

   transitionSurfaceView(surfaceView, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal);

   std::array<float, 4> clearColor = { data.red, data.green, data.blue, data.alpha };
   mActiveCommandBuffer.clearColorImage(surfaceView->surface->image, vk::ImageLayout::eTransferDstOptimal, clearColor, { surfaceView->subresRange });
}

void
Driver::decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data)
{
   flushPendingDraws();

   // Find our depthbuffer to clear
   DepthStencilBufferDesc depthBuffer;
   depthBuffer.base256b = data.db_depth_base.BASE_256B();
   depthBuffer.pitchTileMax = data.db_depth_size.PITCH_TILE_MAX();
   depthBuffer.sliceTileMax = data.db_depth_size.SLICE_TILE_MAX();
   depthBuffer.format = data.db_depth_info.FORMAT();
   depthBuffer.arrayMode = data.db_depth_info.ARRAY_MODE();
   depthBuffer.sliceStart = data.db_depth_view.SLICE_START();
   depthBuffer.sliceEnd = data.db_depth_view.SLICE_MAX() + 1;
   auto surfaceView = getDepthStencilBuffer(depthBuffer);

   transitionSurfaceView(surfaceView, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal);

   auto db_depth_clear = getRegister<latte::DB_DEPTH_CLEAR>(latte::Register::DB_DEPTH_CLEAR);
   auto db_stencil_clear = getRegister<latte::DB_STENCIL_CLEAR>(latte::Register::DB_STENCIL_CLEAR);

   vk::ClearDepthStencilValue clearDepthStencil;
   clearDepthStencil.depth = db_depth_clear.DEPTH_CLEAR();
   clearDepthStencil.stencil = db_stencil_clear.CLEAR();
   mActiveCommandBuffer.clearDepthStencilImage(
      surfaceView->surface->image, vk::ImageLayout::eTransferDstOptimal, clearDepthStencil, { surfaceView->subresRange });
}

void
Driver::decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data)
{
   decaf_abort("Unsupported pm4 decafOSScreenFlip");
}

void
Driver::decafCopySurface(const latte::pm4::DecafCopySurface &data)
{
   flushPendingDraws();

   if (data.dstImage.getAddress() == 0 || data.srcImage.getAddress() == 0) {
      return;
   }
   //decaf_check(data.dstPitch <= data.srcPitch);
   //decaf_check(data.dstWidth == data.srcWidth);
   //decaf_check(data.dstHeight == data.srcHeight);
   //decaf_check(data.dstDepth == data.srcDepth);
   //decaf_check(data.dstDim == data.srcDim);
   // Commented the above because slice-wise accross different things is fine...

   if (data.dstLevel > 0) {
      // We do not currently support mip mapping levels.
      return;
   }

   // Fetch the source surface
   SurfaceDesc sourceDataDesc;
   sourceDataDesc.baseAddress = static_cast<uint32_t>(data.srcImage);
   sourceDataDesc.pitch = data.srcPitch;
   sourceDataDesc.width = data.srcWidth;
   sourceDataDesc.height = data.srcHeight;
   sourceDataDesc.depth = data.srcDepth;
   sourceDataDesc.samples = 1;
   sourceDataDesc.dim = data.srcDim;
   sourceDataDesc.format = latte::getSurfaceFormat(
      data.srcFormat,
      data.srcNumFormat,
      data.srcFormatComp,
      data.srcForceDegamma);
   sourceDataDesc.tileType = data.srcTileType;
   sourceDataDesc.tileMode = data.srcTileMode;
   auto sourceImage = getSurface(sourceDataDesc);

   // Fetch the destination surface
   SurfaceDesc destDataDesc;
   destDataDesc.baseAddress = static_cast<uint32_t>(data.dstImage);
   destDataDesc.pitch = data.dstPitch;
   destDataDesc.width = data.dstWidth;
   destDataDesc.height = data.dstHeight;
   destDataDesc.depth = data.dstDepth;
   destDataDesc.samples = 1;
   destDataDesc.dim = data.dstDim;
   destDataDesc.format = latte::getSurfaceFormat(
      data.dstFormat,
      data.dstNumFormat,
      data.dstFormatComp,
      data.dstForceDegamma);
   destDataDesc.tileType = data.dstTileType;
   destDataDesc.tileMode = data.dstTileMode;
   auto destImage = getSurface(destDataDesc);

   // Transition the surfaces to the appropriate layouts
   transitionSurface(sourceImage, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, { data.srcSlice, 1 });
   transitionSurface(destImage, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal, { data.dstSlice, 1 });

   // Calculate the bounds of the copy
   auto copyWidth = data.srcWidth;
   auto copyHeight = data.srcHeight;
   auto copyDepth = data.srcDepth;
   if (data.srcDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      copyDepth *= 6;
   }

   // Perform the copy, note that CopySurface only supports 1:1 copies
   vk::ImageBlit blitRegion(
      vk::ImageSubresourceLayers(sourceImage->subresRange.aspectMask, 0, data.srcSlice, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(copyWidth, copyHeight, copyDepth) },
      vk::ImageSubresourceLayers(sourceImage->subresRange.aspectMask, 0, data.dstSlice, 1),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(copyWidth, copyHeight, copyDepth) });

   mActiveCommandBuffer.blitImage(
      sourceImage->image,
      vk::ImageLayout::eTransferSrcOptimal,
      destImage->image,
      vk::ImageLayout::eTransferDstOptimal,
      { blitRegion },
      vk::Filter::eNearest);
}

void
Driver::decafExpandColorBuffer(const latte::pm4::DecafExpandColorBuffer &data)
{
   flushPendingDraws();

   // We do not actually support MSAA in our Vulkan backend, so we simply
   // need to translate this to a series of surface copies.

   for (auto i = 0u; i < data.numSlices; ++i) {
      decafCopySurface({
         data.dstImage,
         data.dstMipmaps,
         data.dstLevel,
         data.dstSlice + i,
         data.dstPitch,
         data.dstWidth,
         data.dstHeight,
         data.dstDepth,
         data.dstSamples,
         data.dstDim,
         data.dstFormat,
         data.dstNumFormat,
         data.dstFormatComp,
         data.dstForceDegamma,
         data.dstTileType,
         data.dstTileMode,
         data.srcImage,
         data.srcMipmaps,
         data.srcLevel,
         data.srcSlice + i,
         data.srcPitch,
         data.srcWidth,
         data.srcHeight,
         data.srcDepth,
         data.srcSamples,
         data.srcDim,
         data.srcFormat,
         data.srcNumFormat,
         data.srcFormatComp,
         data.srcForceDegamma,
         data.srcTileType,
         data.srcTileMode
      });
   }
}

void
Driver::drawIndexAuto(const latte::pm4::DrawIndexAuto &data)
{
   drawGenericIndexed(data.drawInitiator, data.count, nullptr);
}

void
Driver::drawIndex2(const latte::pm4::DrawIndex2 &data)
{
   drawGenericIndexed(data.drawInitiator, data.count, phys_cast<void*>(data.addr).getRawPointer());
}

void
Driver::drawIndexImmd(const latte::pm4::DrawIndexImmd &data)
{
   drawGenericIndexed(data.drawInitiator, data.count, data.indices.data());
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
   addRetireTask([=](){
      if (data.addrHi.DATA32()) {
         *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
      } else {
         *reinterpret_cast<uint64_t *>(ptr) = value;
      }
   });
}

void
Driver::eventWrite(const latte::pm4::EventWrite &data)
{
   auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
   auto dataPtr = phys_cast<uint64_t*>(addr).getRawPointer();

   if (data.eventInitiator.EVENT_TYPE() == latte::VGT_EVENT_TYPE::ZPASS_DONE) {
      // Check if this is the first query, or a second query (as part
      // of a single occlusion query grouping).
      if (mLastOccQueryAddr == 0) {
         auto dstBuffer = getDataMemCache(addr, 8);
         transitionMemCache(dstBuffer, ResourceUsage::TransferDst);

         // Write `0` into the beginning zpass count asynchronously.
         mActiveCommandBuffer.fillBuffer(dstBuffer->buffer, 0, 8, 0);

         // Start our occlusion query
         auto queryPool = allocateOccQueryPool();
         mActiveCommandBuffer.beginQuery(queryPool, 0, vk::QueryControlFlagBits::ePrecise);

         mLastOccQuery = queryPool;
         mLastOccQueryAddr = dataPtr;
      } else {
         // Make sure this is the result value of the same query
         decaf_check(dataPtr == mLastOccQueryAddr + 1);

         auto dstBuffer = getDataMemCache(addr, 8);
         transitionMemCache(dstBuffer, ResourceUsage::TransferDst);

         mActiveCommandBuffer.endQuery(mLastOccQuery, 0);
         mActiveCommandBuffer.copyQueryPoolResults(mLastOccQuery, 0, 1,
                                                   dstBuffer->buffer, 0, 8,
                                                   vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);

         mLastOccQuery = vk::QueryPool();
         mLastOccQueryAddr = nullptr;
      }
   } else {
      decaf_abort("Unexpected eventWrite event type.");
   }
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

      addRetireTask([=](){
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
      });
   }

   // Generate interrupt if required
   if (data.addrHi.INT_SEL() != latte::pm4::EWP_INT_NONE) {
      addRetireTask([=](){
         auto interrupt = gpu::ih::Entry { };
         interrupt.word0 = latte::CP_INT_SRC_ID::CP_EOP_EVENT;
         gpu::ih::write(interrupt);
      });
   }
}

void
Driver::pfpSyncMe(const latte::pm4::PfpSyncMe &data)
{
   // Due to the specialized implementation of queries, there is
   // no need to implement any special behaviours here.
}

void
Driver::setPredication(const latte::pm4::SetPredication &data)
{
   // We do not currently implement GPU-side predication of drawing.  Instead
   // we simply allow all draws to proceed as if they were successful.
}

void
Driver::streamOutBaseUpdate(const latte::pm4::StreamOutBaseUpdate &data)
{
   // This is ignored as we don't need to do anything special when the base is
   // updated.  Instead we detect that the buffer registers have changed and
   // use that as the indication to switch.
}

void
Driver::streamOutBufferUpdate(const latte::pm4::StreamOutBufferUpdate &data)
{
   auto bufferIdx = data.control.SELECT_BUFFER();

   if (data.control.STORE_BUFFER_FILLED_SIZE()) {
      auto stream = mStreamOutContext[bufferIdx];

      decaf_check(data.dstLo);
      decaf_check(stream);

      readbackStreamContext(stream, data.dstLo);
   }

   StreamContextObject *newStreamOut = nullptr;
   if (data.control.OFFSET_SOURCE() == STRMOUT_OFFSET_SOURCE::STRMOUT_OFFSET_FROM_MEM) {
      auto srcPtr = phys_cast<uint32_t*>(data.srcLo);
      decaf_check(srcPtr);
      newStreamOut = allocateStreamContext(*srcPtr);
   } else if (data.control.OFFSET_SOURCE() == STRMOUT_OFFSET_SOURCE::STRMOUT_OFFSET_FROM_PACKET) {
      auto offset = static_cast<uint32_t>(data.srcLo);
      newStreamOut = allocateStreamContext(offset);
   } else if (data.control.OFFSET_SOURCE() == STRMOUT_OFFSET_SOURCE::STRMOUT_OFFSET_NONE) {
      // Nothing to do here, as they didn't want to load the offset from anywhere...
   } else {
      decaf_abort("Unexpected offset source during stream out buffer update");
   }

   if (newStreamOut) {
      auto oldStreamOut = mStreamOutContext[bufferIdx];
      mStreamOutContext[bufferIdx] = newStreamOut;

      if (oldStreamOut) {
         // We have to defer the destruction of this buffer until the end of
         // the current context or we may destroy it while a pending read is
         // in the contexts callback list.

         addRetireTask([=](){
            releaseStreamContext(oldStreamOut);
         });
      }
   }
}

void
Driver::surfaceSync(const latte::pm4::SurfaceSync &data)
{
   // TODO: Handle surface syncs when using non-coherent surface optimizations.
}

void
Driver::applyRegister(latte::Register reg)
{
   // Vulkan driver never directly applies register values
}


void
Driver::executeBuffer(const gpu::ringbuffer::Buffer &buffer)
{
   decaf_check(!mActiveSyncWaiter);

   // Begin our command group (sync waiter)
   beginCommandGroup();

   // Begin preparing our command buffer
   beginCommandBuffer();

   // Execute guest PM4 command buffer
   runCommandBuffer(buffer);

   // End preparing our command buffer
   endCommandBuffer();

   // Submit the generated command buffer to the host GPU queue
   vk::SubmitInfo submitInfo;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &mActiveCommandBuffer;
   mQueue.submit({ submitInfo }, mActiveSyncWaiter->fence);

   // End our command group
   endCommandGroup();

   // Optimize the memory layout of our segments every 10 frames.
   if (mActiveBatchIndex % 10 == 0) {
      mMemTracker.optimize();
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
