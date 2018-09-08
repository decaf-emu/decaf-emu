#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_formats.h"

#include <vulkan/vulkan.hpp>

namespace vulkan
{

enum DataFormatUsage : uint32_t
{
   FORMAT_HAS_COLOR = (1 << 0),
   FORMAT_HAS_DEPTH = (1 << 1),
   FORMAT_HAS_STENCIL = (1 << 2),
   FORMAT_ALLOW_RENDER_TARGET = (1 << 3),
};

DataFormatUsage
getDataFormatUsageFlags(latte::SQ_DATA_FORMAT format);

std::pair<vk::ImageType, uint32_t>
getSurfaceTypeInfo(latte::SQ_TEX_DIM dim, uint32_t height, uint32_t depth);

vk::Format
getSurfaceFormat(latte::SQ_DATA_FORMAT format,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma);

} // namespace vulkan

#endif // DECAF_VULKAN
