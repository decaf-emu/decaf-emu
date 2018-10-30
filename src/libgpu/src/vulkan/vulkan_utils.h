#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_formats.h"

#include <vulkan/vulkan.hpp>

namespace vulkan
{

enum SurfaceFormatUsage : uint32_t
{
   TEXTURE = (1 << 0),
   COLOR = (1 << 1),
   DEPTH = (1 << 2),
   STENCIL = (1 << 3),

   T = TEXTURE,
   TC = TEXTURE | COLOR,
   TD = TEXTURE | DEPTH,
   TCD = TEXTURE | COLOR | DEPTH,
   TDS = TEXTURE | DEPTH | STENCIL,
   D = DEPTH,
   DS = DEPTH | STENCIL
};

vk::Format
getVkSurfaceFormat(latte::SurfaceFormat format, latte::SQ_TILE_TYPE tileType);

vk::ComponentSwizzle
getVkComponentSwizzle(latte::SQ_SEL sel);

vk::BorderColor
getVkBorderColor(latte::SQ_TEX_BORDER_COLOR color);

vk::Filter
getVkXyTextureFilter(latte::SQ_TEX_XY_FILTER filter);

vk::SamplerMipmapMode
getVkZTextureFilter(latte::SQ_TEX_Z_FILTER filter);

vk::SamplerAddressMode
getVkTextureAddressMode(latte::SQ_TEX_CLAMP clamp);

bool
getVkAnisotropyEnabled(latte::SQ_TEX_ANISO aniso);

float
getVkMaxAnisotropy(latte::SQ_TEX_ANISO aniso);

bool
getVkCompareOpEnabled(latte::REF_FUNC func);

vk::CompareOp
getVkCompareOp(latte::REF_FUNC func);

vk::StencilOp
getVkStencilOp(latte::DB_STENCIL_FUNC func);

vk::BlendFactor
getVkBlendFactor(latte::CB_BLEND_FUNC func);

vk::BlendOp
getVkBlendOp(latte::CB_COMB_FUNC func);

SurfaceFormatUsage
getVkSurfaceFormatUsage(latte::SurfaceFormat format);

vk::SampleCountFlags
getVkSampleCount(uint32_t samples);

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
