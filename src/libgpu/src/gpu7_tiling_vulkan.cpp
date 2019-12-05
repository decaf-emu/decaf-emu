#ifdef DECAF_VULKAN
#include "gpu7_tiling_vulkan.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <vulkan_shaders_bin/gpu7_tiling.comp.spv.h>

namespace gpu7::tiling::vulkan
{

struct GeneralPushConstants
{
   uint32_t firstSliceIndex;
   uint32_t maxTiles;
};

struct MicroTilePushConstants : GeneralPushConstants
{
   uint32_t numTilesPerRow;
   uint32_t numTilesPerSlice;
   uint32_t thinMicroTileBytes;
   uint32_t thickSliceBytes;
};

struct MacroTilePushConstants : MicroTilePushConstants
{
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
   uint32_t bankSwapWidth;
};

struct TileShaderSpecialisation
{
   uint32_t isUntiling;
   uint32_t microTileThickness;
   uint32_t macroTileWidth;
   uint32_t macroTileHeight;
   uint32_t isMacro3X;
   uint32_t isBankSwapped;
   uint32_t bpp;
   uint32_t isDepth;
   uint32_t subGroupSize;
};

struct BppDepthInfo
{
   uint32_t bpp;
   bool isDepth;
};

std::array<BppDepthInfo, 8> ValidBppDepths = { {
   { 8, false }, { 16, false }, { 32, false }, { 64, false }, { 128, false },
   { 16, true }, { 32, true }, { 64, true } } };

struct TileModeInfo
{
   TileMode tileMode;
   uint32_t microTileThickness;
   uint32_t macroTileWidth;
   uint32_t macroTileHeight;
   bool isMacro3X;
   bool isBankSwapped;
};

std::array<TileModeInfo, 14> ValidTileConfigs = { {
   { TileMode::Micro1DTiledThin1, 1, 1, 1, false, false },
   { TileMode::Micro1DTiledThick, 4, 1, 1, false, false },
   { TileMode::Macro2DTiledThin1, 1, 4, 2, false, false },
   { TileMode::Macro2DTiledThin2, 1, 2, 4, false, false },
   { TileMode::Macro2DTiledThin4, 1, 1, 8, false, false },
   { TileMode::Macro2DTiledThick, 4, 4, 2, false, false },
   { TileMode::Macro2BTiledThin1, 1, 4, 2, false, true },
   { TileMode::Macro2BTiledThin2, 1, 2, 4, false, true },
   { TileMode::Macro2BTiledThin4, 1, 1, 8, false, true },
   { TileMode::Macro2BTiledThick, 4, 4, 2, false, true },
   { TileMode::Macro3DTiledThin1, 1, 4, 2, true, false },
   { TileMode::Macro3DTiledThick, 4, 4, 2, true, false },
   { TileMode::Macro3BTiledThin1, 1, 4, 2, true, true },
   { TileMode::Macro3BTiledThick, 4, 4, 2, true, true },
} };

// TODO: This should be based on the GPU in use
static const uint32_t GpuSubGroupSize = 32;

static inline uint32_t
getRetileSpecKey(uint32_t bpp, bool isDepth, TileMode tileMode, bool isUntiling)
{
   uint32_t fmtKey = bpp + (isDepth ? 100 : 0);
   uint32_t tileModeKey = static_cast<uint32_t>(tileMode);
   uint32_t isUntilingKey = isUntiling ? 1 : 0;

   uint32_t key = 0;
   key |= (fmtKey << 0) & 0x0000FFFF;
   key |= (tileModeKey << 16) & 0x0FFF0000;
   key |= (isUntilingKey << 28) & 0xF0000000;
   return key;
}

void
Retiler::initialise(vk::Device device)
{
   mDevice = device;

   std::array<vk::DescriptorSetLayoutBinding, 2> descriptorSetLayoutBinding = {};

   descriptorSetLayoutBinding[0].binding = 0;
   descriptorSetLayoutBinding[0].descriptorType = vk::DescriptorType::eStorageBuffer;
   descriptorSetLayoutBinding[0].descriptorCount = 1;
   descriptorSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eCompute;

   descriptorSetLayoutBinding[1].binding = 1;
   descriptorSetLayoutBinding[1].descriptorType = vk::DescriptorType::eStorageBuffer;
   descriptorSetLayoutBinding[1].descriptorCount = 1;
   descriptorSetLayoutBinding[1].stageFlags = vk::ShaderStageFlagBits::eCompute;

   vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
   descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBinding.size());
   descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBinding.data();

   mDescriptorSetLayout = mDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

   // We create a push constant block that is the size of the largest push
   // constants block we could use, saves us from having multiple layouts.
   vk::PushConstantRange pushConstant = {};
   pushConstant.stageFlags = vk::ShaderStageFlagBits::eCompute;
   pushConstant.offset = 0;
   pushConstant.size = sizeof(MacroTilePushConstants);

   vk::PipelineLayoutCreateInfo pipelineLayoutDesc;
   pipelineLayoutDesc.setLayoutCount = 1;
   pipelineLayoutDesc.pSetLayouts = &mDescriptorSetLayout;
   pipelineLayoutDesc.pushConstantRangeCount = 1;
   pipelineLayoutDesc.pPushConstantRanges = &pushConstant;
   mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutDesc);

   vk::ShaderModuleCreateInfo shaderDesc;
   shaderDesc.pCode = reinterpret_cast<const uint32_t*>(gpu7_tiling_comp_spv);
   shaderDesc.codeSize = gpu7_tiling_comp_spv_size;
   mShader = mDevice.createShaderModule(shaderDesc);

   static const std::array<vk::SpecializationMapEntry, 9> specEntries = { {
      { 0, offsetof(TileShaderSpecialisation, isUntiling), sizeof(uint32_t) },
      { 1, offsetof(TileShaderSpecialisation, microTileThickness), sizeof(uint32_t) },
      { 2, offsetof(TileShaderSpecialisation, macroTileWidth), sizeof(uint32_t) },
      { 3, offsetof(TileShaderSpecialisation, macroTileHeight), sizeof(uint32_t) },
      { 4, offsetof(TileShaderSpecialisation, isMacro3X), sizeof(uint32_t) },
      { 5, offsetof(TileShaderSpecialisation, isBankSwapped), sizeof(uint32_t) },
      { 6, offsetof(TileShaderSpecialisation, bpp), sizeof(uint32_t) },
      { 7, offsetof(TileShaderSpecialisation, isDepth), sizeof(uint32_t) },
      { 8, offsetof(TileShaderSpecialisation, subGroupSize), sizeof(uint32_t) }
   } };

   for (auto tileConfig : ValidTileConfigs) {
      for (auto bppDepth : ValidBppDepths) {
         for (auto tileOrUntile : { true, false }) {
            uint32_t specKey = getRetileSpecKey(bppDepth.bpp,
                                                bppDepth.isDepth,
                                                tileConfig.tileMode,
                                                tileOrUntile);

            TileShaderSpecialisation specValues;
            specValues.isUntiling = tileOrUntile ? 1 : 0;
            specValues.microTileThickness = tileConfig.microTileThickness;
            specValues.macroTileWidth = tileConfig.macroTileWidth;
            specValues.macroTileHeight = tileConfig.macroTileHeight;
            specValues.isMacro3X = tileConfig.isMacro3X ? 1 : 0;
            specValues.isBankSwapped = tileConfig.isBankSwapped ? 1 : 0;
            specValues.bpp = bppDepth.bpp;
            specValues.isDepth = bppDepth.isDepth ? 1 : 0;
            specValues.subGroupSize = GpuSubGroupSize;

            vk::SpecializationInfo specInfo;
            specInfo.mapEntryCount = static_cast<uint32_t>(specEntries.size());
            specInfo.pMapEntries = specEntries.data();
            specInfo.dataSize = sizeof(TileShaderSpecialisation);
            specInfo.pData = &specValues;

            vk::PipelineShaderStageCreateInfo shaderStageDesc = {};
            shaderStageDesc.stage = vk::ShaderStageFlagBits::eCompute;
            shaderStageDesc.module = mShader;
            shaderStageDesc.pName = "main";
            shaderStageDesc.pSpecializationInfo = &specInfo;

            vk::ComputePipelineCreateInfo pipelineDesc = {};
            pipelineDesc.stage = shaderStageDesc;
            pipelineDesc.layout = mPipelineLayout;

            auto pipeline = mDevice.createComputePipeline(vk::PipelineCache(), pipelineDesc);
            mPipelines.insert({ specKey, pipeline });
         }
      }
   }
}

/*
When retiling THICK tile modes, this algorithm assumes that you've aligned the tiled
buffer to the edge of a group of 4 slices and the untiled buffer directly to the slice.
*/
void
Retiler::retile(bool wantsUntile,
                const RetileInfo& retileInfo,
                vk::DescriptorSet& descriptorSet,
                vk::CommandBuffer& commandBuffer,
                vk::Buffer tiledBuffer, uint32_t tiledOffset,
                vk::Buffer untiledBuffer, uint32_t untiledOffset,
                uint32_t firstSlice, uint32_t numSlices)
{
   // Would be odd to dispatch a retile when we are not tiled...
   decaf_check(retileInfo.isTiled);

   // Calcualate the spec key for this retiler configuration
   // Due to know known issues with depth, we instead retile it normally.
   auto specKey = getRetileSpecKey(retileInfo.bitsPerElement,
                                   retileInfo.isDepth,
                                   retileInfo.tileMode,
                                   wantsUntile);

   // Find the specific pipeline for this configuration
   auto pipelineIter = mPipelines.find(specKey);
   if (pipelineIter == mPipelines.end()) {
      decaf_abort("Attempted to retile an unsupported surface configuration");
   }

   // Bind the pipeline for execution
   commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineIter->second);

   // Calculate the sizing for our retiling space.  Note that we have to make sure
   // we are correctly aligning the location when the user is doing partial groups
   // of slices on thickness>1
   auto alignedFirstSlice = align_down(firstSlice, retileInfo.microTileThickness);
   auto alignedLastSlice = align_up(firstSlice + numSlices, retileInfo.microTileThickness);
   auto alignedNumSlices = alignedLastSlice - alignedFirstSlice;
   uint32_t tiledSize = alignedNumSlices * retileInfo.thinSliceBytes;
   uint32_t untiledSize = numSlices * retileInfo.thinSliceBytes;

   std::array<vk::DescriptorBufferInfo, 2> descriptorBufferDescs;
   descriptorBufferDescs[0].buffer = tiledBuffer;
   descriptorBufferDescs[0].offset = tiledOffset;
   descriptorBufferDescs[0].range = tiledSize;
   descriptorBufferDescs[1].buffer = untiledBuffer;
   descriptorBufferDescs[1].offset = untiledOffset;
   descriptorBufferDescs[1].range = untiledSize;

   vk::WriteDescriptorSet setWriteDesc;
   setWriteDesc.dstSet = descriptorSet;
   setWriteDesc.dstBinding = 0;
   setWriteDesc.descriptorCount = static_cast<uint32_t>(descriptorBufferDescs.size());
   setWriteDesc.descriptorType = vk::DescriptorType::eStorageBuffer;
   setWriteDesc.pBufferInfo = descriptorBufferDescs.data();
   mDevice.updateDescriptorSets({ setWriteDesc }, {});

   // Bind our new descriptor set
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, mPipelineLayout, 0, { descriptorSet }, {});

   // Calculate the number of tiles we need to process!
   uint32_t numDispatchTiles = numSlices * retileInfo.numTilesPerSlice;

   // Setup our arguments
   if (!retileInfo.isMacroTiled) {
      MicroTilePushConstants pushConstants;
      pushConstants.firstSliceIndex = firstSlice;
      pushConstants.maxTiles = numDispatchTiles;

      pushConstants.numTilesPerRow = retileInfo.numTilesPerRow;
      pushConstants.numTilesPerSlice = retileInfo.numTilesPerSlice;
      pushConstants.thinMicroTileBytes = retileInfo.thickMicroTileBytes / retileInfo.microTileThickness;
      pushConstants.thickSliceBytes = retileInfo.thinSliceBytes * retileInfo.microTileThickness;

      commandBuffer.pushConstants<MicroTilePushConstants>(
         mPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pushConstants });
   } else {
      MacroTilePushConstants pushConstants;
      pushConstants.firstSliceIndex = firstSlice;
      pushConstants.maxTiles = numDispatchTiles;

      pushConstants.numTilesPerRow = retileInfo.numTilesPerRow;
      pushConstants.numTilesPerSlice = retileInfo.numTilesPerSlice;
      pushConstants.thinMicroTileBytes = retileInfo.thickMicroTileBytes / retileInfo.microTileThickness;
      pushConstants.thickSliceBytes = retileInfo.thinSliceBytes * retileInfo.microTileThickness;

      pushConstants.bankSwizzle = retileInfo.bankSwizzle;
      pushConstants.pipeSwizzle = retileInfo.pipeSwizzle;
      pushConstants.bankSwapWidth = retileInfo.bankSwapWidth;

      commandBuffer.pushConstants<MacroTilePushConstants>(
         mPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pushConstants });
   }

   // Calculate the number of groups we need to dispatch!
   uint32_t numDispatchGroups =
      align_up(numDispatchTiles, GpuSubGroupSize) / GpuSubGroupSize;

   // Actually dispatch the work to the GPU
   commandBuffer.dispatch(numDispatchGroups, 1, 1);
}

RetileHandle
Retiler::retile(bool wantsUntile,
                const RetileInfo& retileInfo,
                   vk::CommandBuffer& commandBuffer,
                   vk::Buffer dstBuffer, uint32_t dstOffset,
                   vk::Buffer srcBuffer, uint32_t srcOffset,
                uint32_t firstSlice, uint32_t numSlices)
{
   auto handle = allocateHandle();

   vk::DescriptorSetAllocateInfo allocInfo;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &mDescriptorSetLayout;
   allocInfo.descriptorPool = handle->descriptorPool;
   auto descriptorSets = mDevice.allocateDescriptorSets(allocInfo);

   retile(wantsUntile,
          retileInfo,
          descriptorSets[0],
          commandBuffer,
          dstBuffer, dstOffset,
          srcBuffer, srcOffset,
          firstSlice, numSlices);

   return handle;
}

Retiler::HandleImpl*
Retiler::allocateHandle()
{
   Retiler::HandleImpl* handle = nullptr;

   if (!mHandlesPool.empty()) {
      handle = mHandlesPool.back();
      mHandlesPool.pop_back();
   }

   if (!handle) {
      std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2 * 13),
      };

      vk::DescriptorPoolCreateInfo descriptorPoolInfo;
      descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
      descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
      descriptorPoolInfo.maxSets = static_cast<uint32_t>(13);
      auto descriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

      handle = new Retiler::HandleImpl();
      handle->descriptorPool = descriptorPool;
   }

   return handle;
}

void
Retiler::releaseHandle(Retiler::HandleImpl* handle)
{
   mDevice.resetDescriptorPool(handle->descriptorPool);
   mHandlesPool.push_back(handle);
}

}

#endif // DECAF_VULKAN
