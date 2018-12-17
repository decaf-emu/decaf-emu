#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

namespace vulkan
{

PipelineLayoutDesc
Driver::generatePipelineLayoutDesc(const PipelineDesc& pipelineDesc)
{
   PipelineLayoutDesc layoutDesc;

   if (pipelineDesc.vertexShader) {
      const auto& shaderMeta = pipelineDesc.vertexShader->shader.meta;
      if (shaderMeta.cfileUsed) {
         layoutDesc.vsBufferUsed[0] = true;
         for (auto i = 1; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.vsBufferUsed[i] = false;
         }
      } else {
         for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.vsBufferUsed[i] = shaderMeta.cbufferUsed[i];
         }
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.vsSamplerUsed[i] = shaderMeta.samplerUsed[i];
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.vsTextureUsed[i] = shaderMeta.textureUsed[i];
      }
   } else {
      for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
         layoutDesc.vsBufferUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.vsSamplerUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.vsTextureUsed[i] = false;
      }
   }

   if (pipelineDesc.geometryShader) {
      const auto& shaderMeta = pipelineDesc.geometryShader->shader.meta;
      if (shaderMeta.cfileUsed) {
         layoutDesc.vsBufferUsed[0] = true;
         for (auto i = 1; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.gsBufferUsed[i] = false;
         }
      } else {
         for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.gsBufferUsed[i] = shaderMeta.cbufferUsed[i];
         }
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.gsSamplerUsed[i] = shaderMeta.samplerUsed[i];
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.gsTextureUsed[i] = shaderMeta.textureUsed[i];
      }
   } else {
      for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
         layoutDesc.gsBufferUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.gsSamplerUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.gsTextureUsed[i] = false;
      }
   }

   if (pipelineDesc.pixelShader) {
      const auto& shaderMeta = pipelineDesc.pixelShader->shader.meta;
      if (shaderMeta.cfileUsed) {
         layoutDesc.psBufferUsed[0] = true;
         for (auto i = 1; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.psBufferUsed[i] = false;
         }
      } else {
         for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
            layoutDesc.psBufferUsed[i] = shaderMeta.cbufferUsed[i];
         }
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.psSamplerUsed[i] = shaderMeta.samplerUsed[i];
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.psTextureUsed[i] = shaderMeta.textureUsed[i];
      }
   } else {
      for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
         layoutDesc.psBufferUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxSamplers; ++i) {
         layoutDesc.psSamplerUsed[i] = false;
      }
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         layoutDesc.psTextureUsed[i] = false;
      }
   }

   // Calculate the descriptor count

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      if (layoutDesc.vsSamplerUsed[i] || layoutDesc.vsTextureUsed[i]) {
         layoutDesc.numDescriptors++;
      }
      if (layoutDesc.gsSamplerUsed[i] || layoutDesc.gsTextureUsed[i]) {
         layoutDesc.numDescriptors++;
      }
      if (layoutDesc.psSamplerUsed[i] || layoutDesc.psTextureUsed[i]) {
         layoutDesc.numDescriptors++;
      }
   }
   for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
      if (layoutDesc.vsBufferUsed[i]) {
         layoutDesc.numDescriptors++;
      }
      if (layoutDesc.gsBufferUsed[i]) {
         layoutDesc.numDescriptors++;
      }
      if (layoutDesc.psBufferUsed[i]) {
         layoutDesc.numDescriptors++;
      }
   }

   return layoutDesc;
}

PipelineLayoutObject *
Driver::getPipelineLayout(const HashedDesc<PipelineLayoutDesc>& desc, bool forPush)
{
   const auto& currentDesc = desc;

   // We do not check if this pipeline layout matches the currently bound one
   // since that is pretty rare to happen, and this is only invoked whenever
   // a pipeline is being generated anyways...

   auto& foundPl = mPipelineLayouts[currentDesc.hash()];
   if (foundPl) {
      return foundPl;
   }

   foundPl = new PipelineLayoutObject();
   foundPl->desc = currentDesc;

   // -- Descriptor Layout
   std::vector<vk::DescriptorSetLayoutBinding> bindings;

   // We combine the samplers and textures into a single descriptor in the
   // pipeline layout, so we need to make sure this always matches!
   static_assert(latte::MaxTextures == latte::MaxSamplers);


   for (auto i = 0; i < latte::MaxTextures; ++i) {
      if (currentDesc->vsTextureUsed[i] || currentDesc->vsSamplerUsed[i]) {
         vk::DescriptorSetLayoutBinding texSampBindingDesc;
         texSampBindingDesc.binding = (0 * 32) + 0 + i;
         if (currentDesc->vsSamplerUsed[i] && currentDesc->vsTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eCombinedImageSampler;
         } else if (currentDesc->vsSamplerUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampler;
         } else if (currentDesc->vsTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampledImage;
         }

         texSampBindingDesc.descriptorCount = 1;
         texSampBindingDesc.stageFlags = vk::ShaderStageFlagBits::eVertex;
         texSampBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(texSampBindingDesc);
      }

      if (currentDesc->gsTextureUsed[i] || currentDesc->gsSamplerUsed[i]) {
         vk::DescriptorSetLayoutBinding texSampBindingDesc;
         texSampBindingDesc.binding = (1 * 32) + 0 + i;
         if (currentDesc->gsSamplerUsed[i] && currentDesc->gsTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eCombinedImageSampler;
         } else if (currentDesc->gsSamplerUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampler;
         } else if (currentDesc->gsTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampledImage;
         }
         texSampBindingDesc.descriptorCount = 1;
         texSampBindingDesc.stageFlags = vk::ShaderStageFlagBits::eGeometry;
         texSampBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(texSampBindingDesc);
      }

      if (currentDesc->psTextureUsed[i] || currentDesc->psSamplerUsed[i]) {
         vk::DescriptorSetLayoutBinding texSampBindingDesc;
         texSampBindingDesc.binding = (2 * 32) + 0 + i;
         if (currentDesc->psSamplerUsed[i] && currentDesc->psTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eCombinedImageSampler;
         } else if (currentDesc->psSamplerUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampler;
         } else if (currentDesc->psTextureUsed[i]) {
            texSampBindingDesc.descriptorType = vk::DescriptorType::eSampledImage;
         }
         texSampBindingDesc.descriptorCount = 1;
         texSampBindingDesc.stageFlags = vk::ShaderStageFlagBits::eFragment;
         texSampBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(texSampBindingDesc);
      }
   }

   for (auto i = 0; i < latte::MaxUniformBlocks; ++i) {
      if (i >= 15) {
         // Vulkan does not support more than 15 uniform blocks unfortunately,
         // if we ever encounter a game needing all 15, we will need to do block
         // splitting or utilize SSBO's.
         break;
      }

      if (currentDesc->vsBufferUsed[i]) {
         vk::DescriptorSetLayoutBinding cbufferBindingDesc;
         cbufferBindingDesc.binding = (0 * 32) + 16 + i;
         cbufferBindingDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
         cbufferBindingDesc.descriptorCount = 1;
         cbufferBindingDesc.stageFlags = vk::ShaderStageFlagBits::eVertex;
         cbufferBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(cbufferBindingDesc);
      }

      if (currentDesc->gsBufferUsed[i]) {
         vk::DescriptorSetLayoutBinding cbufferBindingDesc;
         cbufferBindingDesc.binding = (1 * 32) + 16 + i;
         cbufferBindingDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
         cbufferBindingDesc.descriptorCount = 1;
         cbufferBindingDesc.stageFlags = vk::ShaderStageFlagBits::eGeometry;
         cbufferBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(cbufferBindingDesc);
      }

      if (currentDesc->psBufferUsed[i]) {
         vk::DescriptorSetLayoutBinding cbufferBindingDesc;
         cbufferBindingDesc.binding = (2 * 32) + 16 + i;
         cbufferBindingDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
         cbufferBindingDesc.descriptorCount = 1;
         cbufferBindingDesc.stageFlags = vk::ShaderStageFlagBits::eFragment;
         cbufferBindingDesc.pImmutableSamplers = nullptr;
         bindings.push_back(cbufferBindingDesc);
      }
   }

   if (forPush) {
      decaf_check(bindings.size() == currentDesc->numDescriptors);
   }

   vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutDesc;

   if (forPush) {
      descriptorSetLayoutDesc.flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
   }

   descriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(bindings.size());
   descriptorSetLayoutDesc.pBindings = bindings.data();
   auto descriptorLayout = mDevice.createDescriptorSetLayout(descriptorSetLayoutDesc);

   // -- Push Descriptors
   std::vector<vk::PushConstantRange> pushConstants;

   vk::PushConstantRange screenSpaceConstants;
   screenSpaceConstants.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry;
   screenSpaceConstants.offset = 0;
   screenSpaceConstants.size = 32;
   pushConstants.push_back(screenSpaceConstants);

   vk::PushConstantRange alphaRefConstants;
   alphaRefConstants.stageFlags = vk::ShaderStageFlagBits::eFragment;
   alphaRefConstants.offset = 32;
   alphaRefConstants.size = 12;
   pushConstants.push_back(alphaRefConstants);

   // -- Pipeline Layout
   vk::PipelineLayoutCreateInfo pipelineLayoutDesc;
   pipelineLayoutDesc.setLayoutCount = 1;
   pipelineLayoutDesc.pSetLayouts = &descriptorLayout;
   pipelineLayoutDesc.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
   pipelineLayoutDesc.pPushConstantRanges = pushConstants.data();
   auto pipelineLayout = mDevice.createPipelineLayout(pipelineLayoutDesc);

   foundPl->descriptorLayout = descriptorLayout;
   foundPl->pipelineLayout = pipelineLayout;

   return foundPl;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
