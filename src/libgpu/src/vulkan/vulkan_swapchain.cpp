#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SwapChainObject *
Driver::allocateSwapChain(const SwapChainInfo &info)
{
   auto surfaceInfo = SurfaceInfo {
      info.baseAddress,
      info.width,
      info.width,
      info.height,
      1,
      1,
      latte::SQ_TEX_DIM::DIM_2D,
      latte::SQ_DATA_FORMAT::FMT_8_8_8_8,
      latte::SQ_NUM_FORMAT::NORM,
      latte::SQ_FORMAT_COMP::UNSIGNED,
      0,
      false,
      latte::SQ_TILE_MODE::LINEAR_ALIGNED
   };
   auto surface = getSurface(surfaceInfo, true);

   transitionSurface(surface, vk::ImageLayout::eGeneral);

   std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
   mActiveCommandBuffer.clearColorImage(surface->image, vk::ImageLayout::eGeneral, clearColor, { surface->subresRange });

   transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

   auto swapChain = new SwapChainObject();
   swapChain->_surface = surface;
   swapChain->info = info;
   swapChain->image = surface->image;
   swapChain->imageView = surface->imageView;
   swapChain->subresRange = surface->subresRange;
   return swapChain;
}

void
Driver::releaseSwapChain(SwapChainObject *swapChain)
{
   // TODO: Implement actual swap chain releases
}

} // namespace vulkan

#endif // DECAF_VULKAN
