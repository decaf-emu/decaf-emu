#pragma once
#ifdef DECAF_VULKAN

#include "latte/latte_formats.h"

#include <vulkan/vulkan.hpp>

namespace vulkan
{

vk::CompareOp
getVkCompareOp(latte::REF_FUNC func);

vk::BlendFactor
getVkBlendFactor(latte::CB_BLEND_FUNC func);

vk::BlendOp
getVkBlendOp(latte::CB_COMB_FUNC func);


enum DataFormatUsage : uint32_t
{
   FORMAT_MAYBE_COLOR = (1 << 0),
   FORMAT_MAYBE_DEPTH = (1 << 1),
   FORMAT_MAYBE_STENCIL = (1 << 2),
   FORMAT_ALLOW_RENDER_TARGET = (1 << 3),
};

DataFormatUsage
getDataFormatUsageFlags(latte::SQ_DATA_FORMAT format);

vk::SampleCountFlags
getVkSampleCount(uint32_t samples);

vk::Format
getSurfaceFormat(latte::SQ_DATA_FORMAT format,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma,
                 bool forDepthBuffer);

} // namespace vulkan

#endif // DECAF_VULKAN
