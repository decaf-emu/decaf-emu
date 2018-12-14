#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "latte/latte_formats.h"

namespace vulkan
{

FramebufferDesc
Driver::getFramebufferDesc()
{
   decaf_check(mCurrentDraw->renderPass);

   auto desc = FramebufferDesc {};

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
      auto cb_color_view = getRegister<latte::CB_COLORN_VIEW>(latte::Register::CB_COLOR0_VIEW + i * 4);

      if (mCurrentDraw->renderPass->colorAttachmentIndexes[i] == -1) {
         // If the RenderPass doesn't want this attachment, skip it...
         desc.colorTargets[i] = ColorBufferDesc {
            0,
            0,
            0,
            latte::CB_FORMAT::COLOR_INVALID,
            latte::CB_NUMBER_TYPE::UNORM,
            latte::BUFFER_ARRAY_MODE::LINEAR_GENERAL,
            0,
            0
         };

         continue;
      }

      decaf_check(cb_color_base.BASE_256B());

      desc.colorTargets[i] = ColorBufferDesc {
         cb_color_base.BASE_256B(),
         cb_color_size.PITCH_TILE_MAX(),
         cb_color_size.SLICE_TILE_MAX(),
         cb_color_info.FORMAT(),
         cb_color_info.NUMBER_TYPE(),
         cb_color_info.ARRAY_MODE(),
         cb_color_view.SLICE_START(),
         cb_color_view.SLICE_MAX() + 1
      };
   }

   do {
      auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
      auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
      auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);
      auto db_depth_view = getRegister<latte::DB_DEPTH_VIEW>(latte::Register::DB_DEPTH_VIEW);

      if (mCurrentDraw->renderPass->depthAttachmentIndex == -1) {
         // If the RenderPass doesn't want depth, skip it...
         desc.depthTarget = DepthStencilBufferDesc {
            0,
            0,
            0,
            latte::DB_FORMAT::DEPTH_INVALID,
            latte::BUFFER_ARRAY_MODE::LINEAR_GENERAL,
            0,
            0
         };

         break;
      }

      decaf_check(db_depth_base.BASE_256B());

      desc.depthTarget = DepthStencilBufferDesc {
         db_depth_base.BASE_256B(),
         db_depth_size.PITCH_TILE_MAX(),
         db_depth_size.SLICE_TILE_MAX(),
         db_depth_info.FORMAT(),
         db_depth_info.ARRAY_MODE(),
         db_depth_view.SLICE_START(),
         db_depth_view.SLICE_MAX() + 1
      };
   } while (false);

   return desc;
}

bool
Driver::checkCurrentFramebuffer()
{
   decaf_check(mCurrentDraw->renderPass);

   HashedDesc<FramebufferDesc> currentDesc = getFramebufferDesc();

   if (mCurrentDraw->framebuffer && mCurrentDraw->framebuffer->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundFb = mFramebuffers[currentDesc.hash()];
   if (foundFb) {
      mCurrentDraw->framebuffer = foundFb;
      mCurrentDraw->framebufferDirty = true;
      return true;
   }

   foundFb = new FramebufferObject();
   foundFb->desc = currentDesc;

   vk::Extent2D overallSize;

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto colorTarget = currentDesc->colorTargets[i];

      if (!colorTarget.base256b) {
         // If the RenderPass doesn't want this attachment, skip it...
         foundFb->colorSurfaces[i] = nullptr;
         continue;
      }

      auto surfaceView = getColorBuffer(colorTarget);
      foundFb->colorSurfaces[i] = surfaceView;

      auto surface = surfaceView->surface;
      if (overallSize.width == 0 && overallSize.height == 0) {
         overallSize.width = surface->desc.width;
         overallSize.height = surface->desc.height;
      } else {
         overallSize.width = std::min(overallSize.width, surface->desc.width);
         overallSize.height = std::min(overallSize.height, surface->desc.height);
      }
   }

   do {
      auto depthTarget = currentDesc->depthTarget;

      if (!depthTarget.base256b) {
         // If the RenderPass doesn't want this attachment, skip it...
         foundFb->depthSurface = nullptr;
         continue;
      }

      auto surfaceView = getDepthStencilBuffer(depthTarget);
      foundFb->depthSurface = surfaceView;

      auto surface = surfaceView->surface;
      if (overallSize.width == 0 && overallSize.height == 0) {
         overallSize.width = surface->desc.width;
         overallSize.height = surface->desc.height;
      } else {
         overallSize.width = std::min(overallSize.width, surface->desc.width);
         overallSize.height = std::min(overallSize.height, surface->desc.height);
      }
   } while (false);

   // TODO: This currently sets up the framebuffers size to match the first
   // actual framebuffer surface we encounter.  In reality I think we need
   // to make sure that the framebuffer is just the min of all surfaces.

   foundFb->renderArea = overallSize;

   mCurrentDraw->framebuffer = foundFb;
   mCurrentDraw->framebufferDirty = true;
   return true;
}

void
Driver::prepareCurrentFramebuffer()
{
   decaf_check(mCurrentDraw->renderPass);
   decaf_check(mCurrentDraw->framebuffer);

   auto& fb = mCurrentDraw->framebuffer;

   if (!mCurrentDraw->framebufferDirty) {
      // If the framebuffer is the same as the last frame, it is considered
      // clean, and we only need to perform barriers in order to make sure
      // that the image layout is appropriate.
      for (auto &surfaceView : fb->colorSurfaces) {
         if (surfaceView) {
            transitionSurfaceView(surfaceView, ResourceUsage::ColorAttachment, vk::ImageLayout::eColorAttachmentOptimal, true);
         }
      }
      if (fb->depthSurface) {
         auto &surfaceView = fb->depthSurface;
         transitionSurfaceView(surfaceView, ResourceUsage::DepthStencilAttachment, vk::ImageLayout::eDepthStencilAttachmentOptimal, true);
      }
      return;
   }

   mCurrentDraw->framebufferDirty = false;


   // First we need to transition all the surfaces to their appropriate places.
   for (auto &surfaceView : fb->colorSurfaces) {
      if (surfaceView) {
         transitionSurfaceView(surfaceView, ResourceUsage::ColorAttachment, vk::ImageLayout::eColorAttachmentOptimal);
      }
   }
   if (fb->depthSurface) {
      auto &surfaceView = fb->depthSurface;
      transitionSurfaceView(surfaceView, ResourceUsage::DepthStencilAttachment, vk::ImageLayout::eDepthStencilAttachmentOptimal);
   }

   // Next lets grab all the appropriate attachments we are using
   uint32_t numAttachments = 0;
   std::array<vk::ImageView, 9> attachments;
   bool needsRefresh = false;

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto& surfaceView = fb->colorSurfaces[i];
      if (!surfaceView) {
         // nothing bound here
         continue;
      }

      auto attachmentIndex = static_cast<uint32_t>(mCurrentDraw->renderPass->colorAttachmentIndexes[i]);
      numAttachments = std::max(numAttachments, attachmentIndex + 1);

      attachments[attachmentIndex] = surfaceView->imageView;

      if (fb->boundViews[attachmentIndex] != surfaceView->imageView) {
         needsRefresh = true;
      }
   }

   do {
      auto& surfaceView = fb->depthSurface;
      if (!surfaceView) {
         // nothing bound here
         continue;
      }

      auto attachmentIndex = static_cast<uint32_t>(mCurrentDraw->renderPass->depthAttachmentIndex);
      numAttachments = std::max(numAttachments, attachmentIndex + 1);

      attachments[attachmentIndex] = surfaceView->imageView;

      if (fb->boundViews[attachmentIndex] != surfaceView->imageView) {
         needsRefresh = true;
      }
   } while (false);

   if (!needsRefresh) {
      return;
   }

   // If we have an existing framebuffer, we can destroy it on the
   // next frame, once we are confident that nobody is using it
   if (fb->framebuffer) {
      auto oldFramebuffer = fb->framebuffer;
      addRetireTask([=](){
         mDevice.destroyFramebuffer(oldFramebuffer);
      });
      fb->framebuffer = nullptr;
   }

   vk::FramebufferCreateInfo framebufferDesc;
   framebufferDesc.renderPass = mCurrentDraw->renderPass->renderPass;
   framebufferDesc.attachmentCount = numAttachments;
   framebufferDesc.pAttachments = attachments.data();
   framebufferDesc.width = fb->renderArea.width;
   framebufferDesc.height = fb->renderArea.height;
   framebufferDesc.layers = 1;
   auto framebuffer = mDevice.createFramebuffer(framebufferDesc);
   fb->framebuffer = framebuffer;
   fb->boundViews = attachments;
}

SurfaceViewObject *
Driver::getColorBuffer(const ColorBufferDesc& info)
{
   auto baseAddress = phys_addr(info.base256b << 8);
   auto pitch_tile_max = info.pitchTileMax;
   auto slice_tile_max = info.sliceTileMax;

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto surfaceFormat = latte::getColorBufferSurfaceFormat(info.format, info.numberType);
   auto tileMode = latte::getArrayModeTileMode(info.arrayMode);

   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = static_cast<uint32_t>(baseAddress);
   surfaceDesc.pitch = pitch;
   surfaceDesc.width = pitch;
   surfaceDesc.height = height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 1u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = surfaceFormat;
   surfaceDesc.tileType = latte::SQ_TILE_TYPE::DEFAULT;
   surfaceDesc.tileMode = tileMode;

   if (info.sliceEnd > 1) {
      surfaceDesc.depth = info.sliceEnd;
      surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D_ARRAY;
   }

   SurfaceViewDesc surfaceViewDesc;
   surfaceViewDesc.surfaceDesc = surfaceDesc;
   surfaceViewDesc.sliceStart = info.sliceStart;
   surfaceViewDesc.sliceEnd = info.sliceEnd;
   surfaceViewDesc.channels = {
      latte::SQ_SEL::SEL_X,
      latte::SQ_SEL::SEL_Y,
      latte::SQ_SEL::SEL_Z,
      latte::SQ_SEL::SEL_W };

   return getSurfaceView(surfaceViewDesc);
}

SurfaceViewObject *
Driver::getDepthStencilBuffer(const DepthStencilBufferDesc& info)
{
   auto baseAddress = phys_addr(info.base256b << 8);
   auto pitch_tile_max = info.pitchTileMax;
   auto slice_tile_max = info.sliceTileMax;

   auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   auto surfaceFormat = latte::getDepthBufferSurfaceFormat(info.format);
   auto tileMode = latte::getArrayModeTileMode(info.arrayMode);

   SurfaceDesc surfaceDesc;
   surfaceDesc.baseAddress = static_cast<uint32_t>(baseAddress);
   surfaceDesc.pitch = pitch;
   surfaceDesc.width = pitch;
   surfaceDesc.height = height;
   surfaceDesc.depth = 1;
   surfaceDesc.samples = 1u;
   surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D;
   surfaceDesc.format = surfaceFormat;
   surfaceDesc.tileType = latte::SQ_TILE_TYPE::DEPTH;
   surfaceDesc.tileMode = tileMode;

   if (info.sliceEnd > 1) {
      surfaceDesc.depth = info.sliceEnd;
      surfaceDesc.dim = latte::SQ_TEX_DIM::DIM_2D_ARRAY;
   }

   SurfaceViewDesc surfaceViewDesc;
   surfaceViewDesc.sliceStart = info.sliceStart;
   surfaceViewDesc.sliceEnd = info.sliceEnd;
   surfaceViewDesc.surfaceDesc = surfaceDesc;
   surfaceViewDesc.channels = {
      latte::SQ_SEL::SEL_X,
      latte::SQ_SEL::SEL_Y,
      latte::SQ_SEL::SEL_Z,
      latte::SQ_SEL::SEL_W };

   return getSurfaceView(surfaceViewDesc);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
