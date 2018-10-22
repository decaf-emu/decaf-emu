#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SwapChainObject *
Driver::allocateSwapChain(const SwapChainDesc &desc)
{
   SurfaceDataDesc surfaceDataDesc;
   surfaceDataDesc.baseAddress = desc.baseAddress.getAddress();
   surfaceDataDesc.pitch = desc.width;
   surfaceDataDesc.width = desc.width;
   surfaceDataDesc.height = desc.height;
   surfaceDataDesc.depth = 1;
   surfaceDataDesc.samples = 1u;
   surfaceDataDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDataDesc.format = latte::SurfaceFormat::R8G8B8A8Unorm;
   surfaceDataDesc.tileType = latte::SQ_TILE_TYPE::DEFAULT;
   surfaceDataDesc.tileMode = latte::SQ_TILE_MODE::LINEAR_ALIGNED;

   SurfaceDesc surfaceDesc;
   surfaceDesc.dataDesc = surfaceDataDesc;
   surfaceDesc.channels = {
      latte::SQ_SEL::SEL_X,
      latte::SQ_SEL::SEL_Y,
      latte::SQ_SEL::SEL_Z,
      latte::SQ_SEL::SEL_W };

   auto surface = getSurface(surfaceDesc, true);

   transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

   std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
   mActiveCommandBuffer.clearColorImage(surface->data->image, vk::ImageLayout::eTransferDstOptimal, clearColor, { surface->data->subresRange });

   auto swapChain = new SwapChainObject();
   swapChain->_surface = surface;
   swapChain->desc = desc;
   swapChain->image = surface->data->image;
   swapChain->imageView = surface->imageView;
   swapChain->subresRange = surface->data->subresRange;
   return swapChain;
}

void
Driver::releaseSwapChain(SwapChainObject *swapChain)
{
   // TODO: Implement releasing of vulkan swap chains.
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
