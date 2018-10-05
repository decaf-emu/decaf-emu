#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "latte/latte_formats.h"

namespace vulkan
{

FramebufferDesc
Driver::getFramebufferDesc()
{
   auto desc = FramebufferDesc {};

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);

      desc.colorTargets[i] = ColorBufferDesc {
         cb_color_base.BASE_256B(),
         cb_color_size.PITCH_TILE_MAX(),
         cb_color_size.SLICE_TILE_MAX(),
         cb_color_info.FORMAT(),
         cb_color_info.NUMBER_TYPE(),
         cb_color_info.ARRAY_MODE()
      };
   }

   auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
   auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
   auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);

   desc.depthTarget = DepthStencilBufferDesc {
      db_depth_base.BASE_256B(),
      db_depth_size.PITCH_TILE_MAX(),
      db_depth_size.SLICE_TILE_MAX(),
      db_depth_info.FORMAT(),
      db_depth_info.ARRAY_MODE()
   };

   return desc;
}

void
Driver::checkCurrentFramebuffer()
{
   decaf_check(mCurrentRenderPass);

   HashedDesc<FramebufferDesc> currentDesc = getFramebufferDesc();

   if (mCurrentFramebuffer && mCurrentFramebuffer->desc == currentDesc) {
      // Already active, nothing to do.
      return;
   }

   auto& foundFb = mFramebuffers[currentDesc.hash()];
   if (foundFb) {
      mCurrentFramebuffer = foundFb;
      return;
   }

   foundFb = new FramebufferObject();
   foundFb->desc = currentDesc;

   vk::Extent2D overallSize;
   uint32_t numAttachments = 0;
   std::array<vk::ImageView, 9> attachments;

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto colorTarget = currentDesc->colorTargets[i];

      if (mCurrentRenderPass->colorAttachmentIndexes[i] == -1) {
         // If the RenderPass doesn't want this attachment, skip it...
         continue;
      }

      auto attachmentIndex = static_cast<uint32_t>(mCurrentRenderPass->colorAttachmentIndexes[i]);
      numAttachments = std::max(numAttachments, attachmentIndex + 1);

      if (!colorTarget.base256b) {
         attachments[attachmentIndex] = nullptr;
         break;
      }

      auto colorBufferDesc = ColorBufferDesc {
         colorTarget.base256b,
         colorTarget.pitchTileMax,
         colorTarget.sliceTileMax,
         colorTarget.format,
         colorTarget.numberType,
         colorTarget.arrayMode
      };
      auto surface = getColorBuffer(colorBufferDesc, false);
      foundFb->colorSurfaces.push_back(surface);

      attachments[attachmentIndex] = surface->imageView;

      if (overallSize.width == 0 && overallSize.height == 0) {
         overallSize.width = surface->desc.width;
         overallSize.height = surface->desc.height;
      } else {
         decaf_check(overallSize.width == surface->desc.width);
         decaf_check(overallSize.height == surface->desc.height);
      }
   }

   do {
      auto depthTarget = currentDesc->depthTarget;

      if (mCurrentRenderPass->depthAttachmentIndex == -1) {
         // If the RenderPass doesn't want this attachment, skip it...
         continue;
      }

      auto attachmentIndex = static_cast<uint32_t>(mCurrentRenderPass->depthAttachmentIndex);
      numAttachments = std::max(numAttachments, attachmentIndex + 1);

      if (!depthTarget.base256b) {
         attachments[attachmentIndex] = nullptr;
         break;
      }

      auto depthStencilBufferDesc = DepthStencilBufferDesc {
         depthTarget.base256b,
         depthTarget.pitchTileMax,
         depthTarget.sliceTileMax,
         depthTarget.format,
         depthTarget.arrayMode
      };
      auto surface = getDepthStencilBuffer(depthStencilBufferDesc, false);
      foundFb->depthSurface = surface;

      attachments[attachmentIndex] = surface->imageView;

      if (overallSize.width == 0 && overallSize.height == 0) {
         overallSize.width = surface->desc.width;
         overallSize.height = surface->desc.height;
      } else {
         decaf_check(overallSize.width == surface->desc.width);
         decaf_check(overallSize.height == surface->desc.height);
      }
   } while (false);

   // TODO: This currently sets up the framebuffers size to match the first
   // actual framebuffer surface we encounter.  In reality I think we need
   // to make sure that the framebuffer is just the min of all surfaces.

   vk::FramebufferCreateInfo framebufferDesc;
   framebufferDesc.renderPass = mCurrentRenderPass->renderPass;
   framebufferDesc.attachmentCount = numAttachments;
   framebufferDesc.pAttachments = attachments.data();
   framebufferDesc.width = overallSize.width;
   framebufferDesc.height = overallSize.height;
   framebufferDesc.layers = 1;
   auto framebuffer = mDevice.createFramebuffer(framebufferDesc);
   foundFb->framebuffer = framebuffer;

   mCurrentFramebuffer = foundFb;
}

SurfaceObject *
Driver::getColorBuffer(const ColorBufferDesc& info,
                       bool discardData)
{
   auto baseAddress = phys_addr((info.base256b << 8) & 0xFFFFF800);
   auto pitch_tile_max = info.pitchTileMax;
   auto slice_tile_max = info.sliceTileMax;

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto format = latte::getColorBufferDataFormat(info.format, info.numberType);
   auto tileMode = latte::getArrayModeTileMode(info.arrayMode);

   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = baseAddress;
   surfaceDesc.pitch = pitch;
   surfaceDesc.width = pitch;
   surfaceDesc.height = height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 0u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = format.format;
   surfaceDesc.numFormat = format.numFormat;
   surfaceDesc.formatComp = format.formatComp;
   surfaceDesc.degamma = format.degamma;
   surfaceDesc.isDepthBuffer = false;
   surfaceDesc.tileMode = tileMode;
   return getSurface(surfaceDesc, discardData);
}

SurfaceObject *
Driver::getDepthStencilBuffer(const DepthStencilBufferDesc& info,
                              bool discardData)
{
   auto baseAddress = phys_addr((info.base256b << 8) & 0xFFFFF800);
   auto pitch_tile_max = info.pitchTileMax;
   auto slice_tile_max = info.sliceTileMax;

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto format = latte::getDepthBufferDataFormat(info.format);
   auto tileMode = latte::getArrayModeTileMode(info.arrayMode);

   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = baseAddress;
   surfaceDesc.pitch = pitch;
   surfaceDesc.width = pitch;
   surfaceDesc.height = height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 0u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = format.format;
   surfaceDesc.numFormat = format.numFormat;
   surfaceDesc.formatComp = format.formatComp;
   surfaceDesc.degamma = format.degamma;
   surfaceDesc.isDepthBuffer = true;
   surfaceDesc.tileMode = tileMode;
   return getSurface(surfaceDesc, discardData);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
