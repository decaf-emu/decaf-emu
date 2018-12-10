#ifdef DECAF_VULKAN
#include "gpu7_tiling_vulkan.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <vulkan_shaders_bin/gpu7_tiling.comp.spv.h>

namespace gpu7::tiling::vulkan
{

struct MicroTilePushConstants
{
   uint32_t dstStride;
   uint32_t numTilesPerRow;
   uint32_t numTileRows;
   uint32_t tiledSliceIndex;
   uint32_t sliceIndex;
   uint32_t numSlices;
   uint32_t sliceBytes;
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
   uint32_t isMacro3D;
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
   AddrTileMode tileMode;
   uint32_t microTileThickness;
   uint32_t macroTileWidth;
   uint32_t macroTileHeight;
   bool isMacro3D;
   bool isBankSwapped;
};

std::array<TileModeInfo, 14> ValidTileConfigs = { {
   { ADDR_TM_1D_TILED_THIN1, 1, 1, 1, false, false },
   { ADDR_TM_1D_TILED_THICK, 4, 1, 1, false, false },
   { ADDR_TM_2D_TILED_THIN1, 1, 4, 2, false, false },
   { ADDR_TM_2D_TILED_THIN2, 1, 2, 4, false, false },
   { ADDR_TM_2D_TILED_THIN4, 1, 1, 8, false, false },
   { ADDR_TM_2D_TILED_THICK, 4, 4, 2, false, false },
   { ADDR_TM_2B_TILED_THIN1, 1, 4, 2, false, true },
   { ADDR_TM_2B_TILED_THIN2, 1, 2, 4, false, true },
   { ADDR_TM_2B_TILED_THIN4, 1, 1, 8, false, true },
   { ADDR_TM_2B_TILED_THICK, 4, 4, 2, false, true },
   { ADDR_TM_3D_TILED_THIN1, 1, 4, 2, true, false },
   { ADDR_TM_3D_TILED_THICK, 4, 4, 2, true, false },
   { ADDR_TM_3B_TILED_THIN1, 1, 4, 2, true, true },
   { ADDR_TM_3B_TILED_THICK, 4, 4, 2, true, true },
} };

// TODO: This should be based on the GPU in use
static const uint32_t GpuSubGroupSize = 6;

static inline uint32_t
getRetileSpecKey(uint32_t bpp, bool isDepth, AddrTileMode tileMode, bool isUntiling)
{
   uint32_t isDepthUint = isDepth ? 1 : 0;
   uint32_t tileModeUint = static_cast<uint32_t>(tileMode);
   uint32_t isUntilingUint = isUntiling ? 1 : 0;

   uint32_t key = 0;
   key |= (bpp            << 0)  & 0x0000FFFF;
   key |= (tileModeUint   << 16) & 0x0FFF0000;
   key |= (isUntilingUint << 28) & 0xF0000000;
   return key;
}

static inline RetileInfo
calculateLinearRetileInfo(const SurfaceDescription &desc,
                          const ADDR_COMPUTE_SURFACE_INFO_OUTPUT &info,
                          uint32_t firstSlice,
                          uint32_t numSlices)
{
   RetileInfo retileInfo;
   retileInfo.isTiled = false;
   return retileInfo;
}

static inline RetileInfo
calculateMicroRetileInfo(const SurfaceDescription &desc,
                         const ADDR_COMPUTE_SURFACE_INFO_OUTPUT &info,
                         uint32_t firstSlice,
                         uint32_t numSlices)
{
   const auto bytesPerElement = info.bpp / 8;

   const auto microTileThickness = getMicroTileThickness(info.tileMode);
   const auto microTileBytes =
      MicroTileWidth * MicroTileHeight * microTileThickness
      * bytesPerElement * desc.numSamples;

   const auto pitch = info.pitch;
   const auto height = info.height;
   const auto microTilesPerRow = pitch / MicroTileWidth;
   const auto microTilesNumRows = height / MicroTileHeight;

   const auto microTileIndexZ = firstSlice / microTileThickness;
   const  auto sliceBytes =
      pitch * height * microTileThickness * bytesPerElement;
   const auto sliceOffset = microTileIndexZ * sliceBytes;

   const auto dstStrideBytes = pitch * bytesPerElement;

   RetileInfo retileInfo;
   retileInfo.firstSlice = firstSlice;
   retileInfo.numSlices = numSlices;
   retileInfo.isTiled = true;
   retileInfo.isMacroTiled = false;
   retileInfo.isDepth = !!desc.flags.depth;
   retileInfo.bitsPerElement = info.bpp;
   retileInfo.macroTileWidth = 1;
   retileInfo.macroTileHeight = 1;
   retileInfo.dstStride = dstStrideBytes;
   retileInfo.microTileBytes = microTileBytes;
   retileInfo.numTilesPerRow = microTilesPerRow;
   retileInfo.numTileRows = microTilesNumRows;
   retileInfo.sliceBytes = sliceBytes;
   retileInfo.microTileThickness = microTileThickness;
   retileInfo.sliceOffset = sliceOffset;
   retileInfo.sampleOffset = 0;
   retileInfo.tileMode = info.tileMode;
   retileInfo.macroTileBytes = 0;
   retileInfo.bankSwizzle = 0;
   retileInfo.pipeSwizzle = 0;
   retileInfo.bankSwapWidth = 0;

   return retileInfo;
}


static inline RetileInfo
calculateMacroRetileInfo(const SurfaceDescription &desc,
                         const ADDR_COMPUTE_SURFACE_INFO_OUTPUT &info,
                         uint32_t firstSlice,
                         uint32_t numSlices)
{
   const auto bytesPerElement = info.bpp / 8;

   const auto microTileThickness = getMicroTileThickness(info.tileMode);
   const auto microTileBytes =
      MicroTileWidth * MicroTileHeight * microTileThickness
      * bytesPerElement * desc.numSamples;

   const auto macroTileWidth = getMacroTileWidth(info.tileMode);
   const auto macroTileHeight = getMacroTileHeight(info.tileMode);
   const auto macroTileBytes =
      macroTileWidth * macroTileHeight * microTileBytes;

   const auto pitch = info.pitch;
   const auto height = info.height;
   const auto macroTilesPerRow = pitch / (macroTileWidth * MicroTileWidth);
   const auto macroTilesNumRows = height / (macroTileHeight * MicroTileHeight);
   const auto dstStrideBytes = pitch * bytesPerElement;

   const auto macroTilesPerSlice = macroTilesPerRow * macroTilesNumRows;
   const auto sliceOffset =
      (firstSlice / microTileThickness) * macroTilesPerSlice * macroTileBytes;

   // Depth tiling is different for samples, not yet implemented
   auto sample = 0;
   const auto sampleOffset = sample * (microTileBytes / desc.numSamples);

   auto bankSwapWidth =
      gpu7::tiling::computeSurfaceBankSwappedWidth(
         info.tileMode, info.bpp, desc.numSamples, info.pitch);

   const auto sliceBytes = pitch * height * microTileThickness * bytesPerElement;

   RetileInfo retileInfo;
   retileInfo.firstSlice = firstSlice;
   retileInfo.numSlices = numSlices;
   retileInfo.isTiled = true;
   retileInfo.isMacroTiled = true;
   retileInfo.isDepth = !!desc.flags.depth;
   retileInfo.bitsPerElement = info.bpp;
   retileInfo.macroTileWidth = macroTileWidth;
   retileInfo.macroTileHeight = macroTileHeight;
   retileInfo.numTilesPerRow = macroTilesPerRow;
   retileInfo.numTileRows = macroTilesNumRows;
   retileInfo.dstStride = dstStrideBytes;
   retileInfo.microTileBytes = microTileBytes;
   retileInfo.sliceBytes = sliceBytes;
   retileInfo.microTileThickness = microTileThickness;
   retileInfo.sampleOffset = sampleOffset;
   retileInfo.sliceOffset = sliceOffset;
   retileInfo.tileMode = info.tileMode;
   retileInfo.macroTileBytes = macroTileBytes;
   retileInfo.bankSwizzle = desc.bankSwizzle;
   retileInfo.pipeSwizzle = desc.pipeSwizzle;
   retileInfo.bankSwapWidth = bankSwapWidth;
   return retileInfo;
}

RetileInfo
calculateRetileInfo(const SurfaceDescription &desc,
                    uint32_t firstSlice,
                    uint32_t numSlices)
{
   const auto info = computeSurfaceInfo(desc, 0, firstSlice);

   switch (desc.tileMode) {
   case ADDR_TM_LINEAR_ALIGNED:
   case ADDR_TM_LINEAR_GENERAL:
      return calculateLinearRetileInfo(desc, info, firstSlice, numSlices);
   case ADDR_TM_1D_TILED_THIN1:
   case ADDR_TM_1D_TILED_THICK:
      return calculateMicroRetileInfo(desc, info, firstSlice, numSlices);
   case ADDR_TM_2D_TILED_THIN1:
   case ADDR_TM_2D_TILED_THIN2:
   case ADDR_TM_2D_TILED_THIN4:
   case ADDR_TM_2D_TILED_THICK:
   case ADDR_TM_2B_TILED_THIN1:
   case ADDR_TM_2B_TILED_THIN2:
   case ADDR_TM_2B_TILED_THIN4:
   case ADDR_TM_2B_TILED_THICK:
   case ADDR_TM_3D_TILED_THIN1:
   case ADDR_TM_3D_TILED_THICK:
   case ADDR_TM_3B_TILED_THIN1:
   case ADDR_TM_3B_TILED_THICK:
      return calculateMacroRetileInfo(desc, info, firstSlice, numSlices);
   default:
      decaf_abort("Invalid tile mode");
   }
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
      { 4, offsetof(TileShaderSpecialisation, isMacro3D), sizeof(uint32_t) },
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
            specValues.isMacro3D = tileConfig.isMacro3D ? 1 : 0;
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
RetileHandle
Retiler::retile(bool wantsUntile,
                vk::CommandBuffer &commandBuffer,
                vk::Buffer dstBuffer, uint32_t dstOffset,
                vk::Buffer srcBuffer, uint32_t srcOffset,
                const RetileInfo& retileInfo)
{
   auto handle = allocateHandle();
   auto& descriptorSet = handle->descriptorSet;

   // Would be odd to dispatch a retile when we are not tiled...
   decaf_check(retileInfo.isTiled);

   // Calcualate the spec key for this retiler configuration
   // Due to know known issues with depth, we instead retile it normally.
   auto specKey = getRetileSpecKey(retileInfo.bitsPerElement,
                                   false, // retileInfo.isDepth,
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
   auto alignedFirstSlice = align_down(retileInfo.firstSlice, retileInfo.microTileThickness);
   auto alignedLastSlice = align_up(retileInfo.firstSlice + retileInfo.numSlices, retileInfo.microTileThickness);
   auto alignedNumSlices = alignedLastSlice - alignedFirstSlice;
   uint32_t srcSize, dstSize;
   if (wantsUntile) {
      srcSize = alignedNumSlices / retileInfo.microTileThickness * retileInfo.sliceBytes;
      dstSize = retileInfo.numSlices * retileInfo.sliceBytes / retileInfo.microTileThickness;
   } else {
      srcSize = retileInfo.numSlices * retileInfo.sliceBytes / retileInfo.microTileThickness;
      dstSize = alignedNumSlices / retileInfo.microTileThickness * retileInfo.sliceBytes;
   }

   std::array<vk::DescriptorBufferInfo, 2> descriptorBufferDescs;
   descriptorBufferDescs[0].buffer = srcBuffer;
   descriptorBufferDescs[0].offset = srcOffset;
   descriptorBufferDescs[0].range = srcSize;
   descriptorBufferDescs[1].buffer = dstBuffer;
   descriptorBufferDescs[1].offset = dstOffset;
   descriptorBufferDescs[1].range = dstSize;

   vk::WriteDescriptorSet setWriteDesc;
   setWriteDesc.dstSet = descriptorSet;
   setWriteDesc.dstBinding = 0;
   setWriteDesc.descriptorCount = static_cast<uint32_t>(descriptorBufferDescs.size());
   setWriteDesc.descriptorType = vk::DescriptorType::eStorageBuffer;
   setWriteDesc.pBufferInfo = descriptorBufferDescs.data();
   mDevice.updateDescriptorSets({ setWriteDesc }, {});

   // Bind our new descriptor set
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, mPipelineLayout, 0, { descriptorSet }, {});

   // Calculate the number of groups we need to dispatch!
   uint32_t numDispatchGroups =
      ((retileInfo.macroTileWidth * retileInfo.macroTileHeight *
        retileInfo.numTilesPerRow * retileInfo.numTileRows *
        retileInfo.numSlices) + GpuSubGroupSize - 1) / GpuSubGroupSize;

   // Actually dispatch the shader
   if (!retileInfo.isMacroTiled) {
      MicroTilePushConstants pushConstants;
      pushConstants.dstStride = retileInfo.dstStride;
      pushConstants.numTilesPerRow = retileInfo.numTilesPerRow;
      pushConstants.numTileRows = retileInfo.numTileRows;
      pushConstants.tiledSliceIndex = alignedFirstSlice;
      pushConstants.sliceIndex = retileInfo.firstSlice;
      pushConstants.numSlices = retileInfo.numSlices;
      pushConstants.sliceBytes = retileInfo.sliceBytes;

      commandBuffer.pushConstants<MicroTilePushConstants>(
         mPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pushConstants });
      commandBuffer.dispatch(numDispatchGroups, 1, 1);
   } else {
      MacroTilePushConstants pushConstants;
      pushConstants.dstStride = retileInfo.dstStride;
      pushConstants.numTilesPerRow = retileInfo.numTilesPerRow;
      pushConstants.numTileRows = retileInfo.numTileRows;
      pushConstants.tiledSliceIndex = alignedFirstSlice;
      pushConstants.sliceIndex = retileInfo.firstSlice;
      pushConstants.numSlices = retileInfo.numSlices;
      pushConstants.sliceBytes = retileInfo.sliceBytes;

      pushConstants.bankSwizzle = retileInfo.bankSwizzle;
      pushConstants.pipeSwizzle = retileInfo.pipeSwizzle;
      pushConstants.bankSwapWidth = retileInfo.bankSwapWidth;

      commandBuffer.pushConstants<MacroTilePushConstants>(
         mPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pushConstants });
      commandBuffer.dispatch(numDispatchGroups, 1, 1);
   }

   return handle;
}

Retiler::HandleImpl *
Retiler::allocateHandle()
{
   Retiler::HandleImpl * handle = nullptr;

   if (!mHandlesPool.empty()) {
      handle = mHandlesPool.back();
      mHandlesPool.pop_back();
   }

   if (!handle) {
      std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2),
      };

      vk::DescriptorPoolCreateInfo descriptorPoolInfo;
      descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
      descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
      descriptorPoolInfo.maxSets = static_cast<uint32_t>(1);
      auto descriptorPool = mDevice.createDescriptorPool(descriptorPoolInfo);

      handle = new Retiler::HandleImpl();
      handle->descriptorPool = descriptorPool;
   }

   vk::DescriptorSetAllocateInfo allocInfo;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &mDescriptorSetLayout;
   allocInfo.descriptorPool = handle->descriptorPool;
   handle->descriptorSet = mDevice.allocateDescriptorSets(allocInfo)[0];

   return handle;
}

void
Retiler::releaseHandle(Retiler::HandleImpl *handle)
{
   mDevice.resetDescriptorPool(handle->descriptorPool);
   handle->descriptorSet = vk::DescriptorSet();
   mHandlesPool.push_back(handle);
}

}

#endif // DECAF_VULKAN
