#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SwapChainObject *
Driver::allocateSwapChain(const SwapChainDesc &desc)
{
   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = desc.baseAddress;
   surfaceDesc.pitch = desc.width;
   surfaceDesc.width = desc.width;
   surfaceDesc.height = desc.height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 0u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = latte::SQ_DATA_FORMAT::FMT_8_8_8_8;
   surfaceDesc.numFormat = latte::SQ_NUM_FORMAT::NORM;
   surfaceDesc.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
   surfaceDesc.degamma = 0;
   surfaceDesc.isDepthBuffer = false;
   surfaceDesc.tileMode = latte::SQ_TILE_MODE::LINEAR_ALIGNED;
   auto surface = getSurface(surfaceDesc, true);

   transitionSurface(surface, vk::ImageLayout::eGeneral);

   std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
   mActiveCommandBuffer.clearColorImage(surface->image, vk::ImageLayout::eGeneral, clearColor, { surface->subresRange });

   transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

   auto swapChain = new SwapChainObject();
   swapChain->_surface = surface;
   swapChain->desc = desc;
   swapChain->image = surface->image;
   swapChain->imageView = surface->imageView;
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
