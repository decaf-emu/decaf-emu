#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SwapChainObject *
Driver::allocateSwapChain(const SwapChainDesc &desc)
{
   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = desc.baseAddress.getAddress();
   surfaceDesc.pitch = desc.width;
   surfaceDesc.width = desc.width;
   surfaceDesc.height = desc.height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 1u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = latte::SurfaceFormat::R8G8B8A8Srgb;
   surfaceDesc.tileType = latte::SQ_TILE_TYPE::DEFAULT;
   surfaceDesc.tileMode = latte::SQ_TILE_MODE::LINEAR_ALIGNED;

   SurfaceViewDesc surfaceViewDesc;
   surfaceViewDesc.sliceStart = 0;
   surfaceViewDesc.sliceEnd = 1;
   surfaceViewDesc.surfaceDesc = surfaceDesc;
   surfaceViewDesc.channels = {
      latte::SQ_SEL::SEL_X,
      latte::SQ_SEL::SEL_Y,
      latte::SQ_SEL::SEL_Z,
      latte::SQ_SEL::SEL_W };

   // TODO: The swap buffer manager inside libgpu should not be managing the ImageView
   // that is being used by the host application...
   auto surfaceView = getSurfaceView(surfaceViewDesc);
   auto surface = surfaceView->surface;

   // We have to transition the view not the surface to ensure the imageView is created.
   transitionSurfaceView(surfaceView, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal);

   std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
   mActiveCommandBuffer.clearColorImage(surface->image, vk::ImageLayout::eTransferDstOptimal, clearColor, { surface->subresRange });

   auto swapChain = new SwapChainObject();
   swapChain->_surface = surface;
   swapChain->desc = desc;
   swapChain->presentable = false;
   swapChain->imageView = surfaceView->imageView;
   swapChain->image = surface->image;
   swapChain->subresRange = surface->subresRange;
   return swapChain;
}

void
Driver::releaseSwapChain(SwapChainObject *swapChain)
{
   // TODO: Implement releasing of vulkan swap chains.
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
