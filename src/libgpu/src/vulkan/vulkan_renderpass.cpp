#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

namespace vulkan
{

RenderPassDesc
Driver::getRenderPassDesc()
{
   auto desc = RenderPassDesc {};

   // TODO: Maybe move the cb_shader_masking into the framebuffer creation to
   // reduce the number of renderpasses that we need to create.  SM3DW is a good
   // game to check the shader masking behaviours.

   auto cb_target_mask = getRegister<latte::CB_TARGET_MASK>(latte::Register::CB_TARGET_MASK);
   auto cb_color_control = getRegister<latte::CB_COLOR_CONTROL>(latte::Register::CB_COLOR_CONTROL);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);

   auto colorWritesEnabled = true;
   if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_OP::DISABLE) {
      colorWritesEnabled = false;
   }

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto targetMask = (cb_target_mask.value >> (i * 4)) & 0xF;
      auto shaderMask = (cb_shader_mask.value >> (i * 4)) & 0xF;

      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);

      auto isValidBuffer = true;
      isValidBuffer &= !!cb_color_base.BASE_256B();
      isValidBuffer &= (cb_color_info.FORMAT() != latte::CB_FORMAT::COLOR_INVALID);

      if (!!targetMask && !!shaderMask && colorWritesEnabled) {
         if (isValidBuffer) {
            desc.colorTargets[i] = RenderPassDesc::ColorTarget {
               true,
               cb_color_info.FORMAT(),
               cb_color_info.NUMBER_TYPE(),
               1
            };
         } else {
            // This export is valid, but the color write is ignored
            desc.colorTargets[i] = RenderPassDesc::ColorTarget {
               true,
               latte::CB_FORMAT::COLOR_INVALID,
               latte::CB_NUMBER_TYPE::UNORM,
               0
            };
         }
      } else {
         desc.colorTargets[i] = RenderPassDesc::ColorTarget {
            false,
            latte::CB_FORMAT::COLOR_INVALID,
            latte::CB_NUMBER_TYPE::UNORM,
            0
         };
      }
   }

   {
      auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);
      auto zEnable = db_depth_control.Z_ENABLE();
      auto stencilEnable = db_depth_control.STENCIL_ENABLE();
      auto depthEnabled = zEnable || stencilEnable;

      auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
      auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);

      auto isValidBuffer = true;
      isValidBuffer &= !!db_depth_base.BASE_256B();
      isValidBuffer &= (db_depth_info.FORMAT() != latte::DB_FORMAT::DEPTH_INVALID);

      if (depthEnabled) {
         if (isValidBuffer) {
            desc.depthTarget = RenderPassDesc::DepthStencilTarget {
               true,
               db_depth_info.FORMAT()
            };
         } else {
            desc.depthTarget = {
               true,
               latte::DB_FORMAT::DEPTH_INVALID
            };
         }
      } else {
         desc.depthTarget = {
            false,
            latte::DB_FORMAT::DEPTH_INVALID
         };
      }
   }

   return desc;
}

bool
Driver::checkCurrentRenderPass()
{
   HashedDesc<RenderPassDesc> currentDesc = getRenderPassDesc();

   if (mCurrentDraw->renderPass && mCurrentDraw->renderPass->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundRp = mRenderPasses[currentDesc.hash()];
   if (foundRp) {
      mCurrentDraw->renderPass = foundRp;
      return true;
   }

   foundRp = new RenderPassObject();
   foundRp->desc = currentDesc;

   std::vector<vk::AttachmentDescription> attachmentDescs;
   std::array<vk::AttachmentReference, latte::MaxRenderTargets> colorAttachmentRefs;
   vk::AttachmentReference depthAttachmentRef;

   for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
      auto &colorTarget = currentDesc->colorTargets[i];

      if (!colorTarget.isEnabled || colorTarget.format == latte::CB_FORMAT::COLOR_INVALID) {
         colorAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
         colorAttachmentRefs[i].layout = vk::ImageLayout::eColorAttachmentOptimal;

         foundRp->colorAttachmentIndexes[i] = -1;
         continue;
      }

      auto surfaceFormat = latte::getColorBufferSurfaceFormat(colorTarget.format, colorTarget.numberType);
      auto hostFormat = getVkSurfaceFormat(surfaceFormat, latte::SQ_TILE_TYPE::DEFAULT);

      vk::AttachmentDescription colorAttachmentDesc;
      colorAttachmentDesc.flags = vk::AttachmentDescriptionFlagBits::eMayAlias;
      colorAttachmentDesc.format = hostFormat;
      colorAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
      colorAttachmentDesc.loadOp = vk::AttachmentLoadOp::eLoad;
      colorAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
      colorAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      colorAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
      colorAttachmentDesc.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
      colorAttachmentDesc.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
      attachmentDescs.push_back(colorAttachmentDesc);
      auto attachmentIndex = static_cast<uint32_t>(attachmentDescs.size() - 1);

      colorAttachmentRefs[i].attachment = attachmentIndex;
      colorAttachmentRefs[i].layout = vk::ImageLayout::eColorAttachmentOptimal;

      foundRp->colorAttachmentIndexes[i] = attachmentIndex;
   }

   do {
      auto depthTarget = currentDesc->depthTarget;
      if (!depthTarget.isEnabled || depthTarget.format == latte::DB_FORMAT::DEPTH_INVALID) {
         depthAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
         depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

         foundRp->depthAttachmentIndex = -1;
         continue;
      }

      auto surfaceFormat = latte::getDepthBufferSurfaceFormat(depthTarget.format);
      auto hostFormat = getVkSurfaceFormat(surfaceFormat, latte::SQ_TILE_TYPE::DEPTH);

      vk::AttachmentDescription depthAttachmentDesc;
      depthAttachmentDesc.format = hostFormat;
      depthAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
      depthAttachmentDesc.loadOp = vk::AttachmentLoadOp::eLoad;
      depthAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
      depthAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eLoad;
      depthAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eStore;
      depthAttachmentDesc.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthAttachmentDesc.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      attachmentDescs.push_back(depthAttachmentDesc);
      auto attachmentIndex = static_cast<uint32_t>(attachmentDescs.size() - 1);

      depthAttachmentRef.attachment = attachmentIndex;
      depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

      foundRp->depthAttachmentIndex = attachmentIndex;
   } while (false);

   vk::SubpassDescription genericSubpass;
   genericSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
   genericSubpass.inputAttachmentCount = 0;
   genericSubpass.pInputAttachments = nullptr;
   genericSubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
   genericSubpass.pColorAttachments = colorAttachmentRefs.data();
   genericSubpass.pResolveAttachments = 0;
   genericSubpass.pDepthStencilAttachment = &depthAttachmentRef;
   genericSubpass.preserveAttachmentCount = 0;
   genericSubpass.pPreserveAttachments = nullptr;

   vk::RenderPassCreateInfo renderPassDesc;
   renderPassDesc.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
   renderPassDesc.pAttachments = attachmentDescs.data();
   renderPassDesc.subpassCount = 1;
   renderPassDesc.pSubpasses = &genericSubpass;
   renderPassDesc.dependencyCount = 0;
   renderPassDesc.pDependencies = nullptr;
   auto renderPass = mDevice.createRenderPass(renderPassDesc);
   foundRp->renderPass = renderPass;

   mCurrentDraw->renderPass = foundRp;
   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
