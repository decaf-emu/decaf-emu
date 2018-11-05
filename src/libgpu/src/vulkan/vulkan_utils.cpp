#ifdef DECAF_VULKAN
#include "vulkan_utils.h"

#include <common/decaf_assert.h>
#include <common/log.h>

namespace vulkan
{

vk::Format
getVkSurfaceFormat(latte::SurfaceFormat format, latte::SQ_TILE_TYPE tileType)
{
   if (tileType == latte::SQ_TILE_TYPE::DEPTH) {
      switch (format) {
      case latte::SurfaceFormat::R16Unorm:
         return vk::Format::eD16Unorm;
      case latte::SurfaceFormat::R32Float:
         return vk::Format::eD32Sfloat;
      case latte::SurfaceFormat::D24UnormS8Uint:
         return vk::Format::eD24UnormS8Uint;
      case latte::SurfaceFormat::X24G8Uint:
         return vk::Format::eD24UnormS8Uint; // Remapped?
      case latte::SurfaceFormat::D32FloatS8UintX24:
         return vk::Format::eD32SfloatS8Uint; // Wrong Size?
      case latte::SurfaceFormat::D32G8UintX24:
         return vk::Format::eD32SfloatS8Uint; // Remapped?
      }

      decaf_abort(fmt::format("Unexpected depth surface format {}", format));
   }

   switch (format) {
   case latte::SurfaceFormat::R8Unorm:
      return vk::Format::eR8Unorm;
   case latte::SurfaceFormat::R8Uint:
      return vk::Format::eR8Uint;
   case latte::SurfaceFormat::R8Snorm:
      return vk::Format::eR8Snorm;
   case latte::SurfaceFormat::R8Sint:
      return vk::Format::eR8Sint;
   case latte::SurfaceFormat::R4G4Unorm:
      return vk::Format::eR4G4UnormPack8;
   case latte::SurfaceFormat::R16Unorm:
      return vk::Format::eR16Unorm;
   case latte::SurfaceFormat::R16Uint:
      return vk::Format::eR16Uint;
   case latte::SurfaceFormat::R16Snorm:
      return vk::Format::eR16Snorm;
   case latte::SurfaceFormat::R16Sint:
      return vk::Format::eR16Sint;
   case latte::SurfaceFormat::R16Float:
      return vk::Format::eR16Sfloat;
   case latte::SurfaceFormat::R8G8Unorm:
      return vk::Format::eR8G8Unorm;
   case latte::SurfaceFormat::R8G8Uint:
      return vk::Format::eR8G8Uint;
   case latte::SurfaceFormat::R8G8Snorm:
      return vk::Format::eR8G8Snorm;
   case latte::SurfaceFormat::R8G8Sint:
      return vk::Format::eR8G8Sint;
   case latte::SurfaceFormat::R5G6B5Unorm:
      return vk::Format::eR5G6B5UnormPack16;
   case latte::SurfaceFormat::R5G5B5A1Unorm:
      return vk::Format::eR5G5B5A1UnormPack16;
   case latte::SurfaceFormat::R4G4B4A4Unorm:
      return vk::Format::eR4G4B4A4UnormPack16;
   case latte::SurfaceFormat::A1B5G5R5Unorm:
      return vk::Format::eR5G5B5A1UnormPack16; // Reversed?
   case latte::SurfaceFormat::R32Uint:
      return vk::Format::eR32Uint;
   case latte::SurfaceFormat::R32Sint:
      return vk::Format::eR32Sint;
   case latte::SurfaceFormat::R32Float:
      return vk::Format::eR32Sfloat;
   case latte::SurfaceFormat::R16G16Unorm:
      return vk::Format::eR16G16Unorm;
   case latte::SurfaceFormat::R16G16Uint:
      return vk::Format::eR16G16Uint;
   case latte::SurfaceFormat::R16G16Snorm:
      return vk::Format::eR16G16Snorm;
   case latte::SurfaceFormat::R16G16Sint:
      return vk::Format::eR16G16Sint;
   case latte::SurfaceFormat::R16G16Float:
      return vk::Format::eR16G16Sfloat;
   case latte::SurfaceFormat::D24UnormS8Uint:
      return vk::Format::eD24UnormS8Uint;
   case latte::SurfaceFormat::X24G8Uint:
      return vk::Format::eD24UnormS8Uint; // Not sure if this is actually right...
   case latte::SurfaceFormat::R11G11B10Float:
      return vk::Format::eB10G11R11UfloatPack32; // This is the incorrect format...
      //return vk::Format::eB10G11R11UfloatPack32; // Remapped?
   case latte::SurfaceFormat::R10G10B10A2Unorm:
      return vk::Format::eA2B10G10R10UnormPack32; // Remapped?
   case latte::SurfaceFormat::R10G10B10A2Uint:
      return vk::Format::eA2B10G10R10UnormPack32; // This is the incorrect format...
      //return vk::Format::eA2B10G10R10UintPack32; // Remapped?
   case latte::SurfaceFormat::R10G10B10A2Snorm:
      return vk::Format::eA2B10G10R10UnormPack32; // This is the incorrect format...
      //return vk::Format::eA2B10G10R10SnormPack32; // Remapped?
   case latte::SurfaceFormat::R10G10B10A2Sint:
      return vk::Format::eA2B10G10R10UnormPack32; // This is the incorrect format...
      //return vk::Format::eA2B10G10R10SintPack32; // Remapped?
   case latte::SurfaceFormat::R8G8B8A8Unorm:
      return vk::Format::eR8G8B8A8Unorm;
   case latte::SurfaceFormat::R8G8B8A8Uint:
      return vk::Format::eR8G8B8A8Uint;
   case latte::SurfaceFormat::R8G8B8A8Snorm:
      return vk::Format::eR8G8B8A8Snorm;
   case latte::SurfaceFormat::R8G8B8A8Sint:
      return vk::Format::eR8G8B8A8Sint;
   case latte::SurfaceFormat::R8G8B8A8Srgb:
      return vk::Format::eR8G8B8A8Srgb;
   case latte::SurfaceFormat::A2B10G10R10Unorm:
      return vk::Format::eA2B10G10R10UnormPack32; // This is the incorrect format...
      //return vk::Format::eA2B10G10R10UnormPack32;
   case latte::SurfaceFormat::A2B10G10R10Uint:
      return vk::Format::eA2B10G10R10UnormPack32; // This is the incorrect format...
      //return vk::Format::eA2B10G10R10UintPack32;
   case latte::SurfaceFormat::D32FloatS8UintX24:
      return vk::Format::eD32SfloatS8Uint;
   case latte::SurfaceFormat::D32G8UintX24:
      return vk::Format::eD32SfloatS8Uint;
   case latte::SurfaceFormat::R32G32Uint:
      return vk::Format::eR32G32Uint;
   case latte::SurfaceFormat::R32G32Sint:
      return vk::Format::eR32G32Sint;
   case latte::SurfaceFormat::R32G32Float:
      return vk::Format::eR32G32Sfloat;
   case latte::SurfaceFormat::R16G16B16A16Unorm:
      return vk::Format::eR16G16B16A16Unorm;
   case latte::SurfaceFormat::R16G16B16A16Uint:
      return vk::Format::eR16G16B16A16Uint;
   case latte::SurfaceFormat::R16G16B16A16Snorm:
      return vk::Format::eR16G16B16A16Snorm;
   case latte::SurfaceFormat::R16G16B16A16Sint:
      return vk::Format::eR16G16B16A16Uint;
   case latte::SurfaceFormat::R16G16B16A16Float:
      return vk::Format::eR16G16B16A16Sfloat;
   case latte::SurfaceFormat::R32G32B32A32Uint:
      return vk::Format::eR32G32B32A32Uint;
   case latte::SurfaceFormat::R32G32B32A32Sint:
      return vk::Format::eR32G32B32A32Sint;
   case latte::SurfaceFormat::R32G32B32A32Float:
      return vk::Format::eR32G32B32A32Sfloat;
   case latte::SurfaceFormat::BC1Unorm:
      return vk::Format::eBc1RgbaUnormBlock;
   case latte::SurfaceFormat::BC1Srgb:
      return vk::Format::eBc1RgbaSrgbBlock;
   case latte::SurfaceFormat::BC2Unorm:
      return vk::Format::eBc2UnormBlock;
   case latte::SurfaceFormat::BC2Srgb:
      return vk::Format::eBc2SrgbBlock;
   case latte::SurfaceFormat::BC3Unorm:
      return vk::Format::eBc3UnormBlock;
   case latte::SurfaceFormat::BC3Srgb:
      return vk::Format::eBc3SrgbBlock;
   case latte::SurfaceFormat::BC4Unorm:
      return vk::Format::eBc4UnormBlock;
   case latte::SurfaceFormat::BC4Snorm:
      return vk::Format::eBc4SnormBlock;
   case latte::SurfaceFormat::BC5Unorm:
      return vk::Format::eBc5UnormBlock;
   case latte::SurfaceFormat::BC5Snorm:
      return vk::Format::eBc5SnormBlock;
   //case latte::SurfaceFormat::NV12:
      // Honestly have no clue how to support this format...
   }

   decaf_abort(fmt::format("Unexpected surface format {}", format));
}

SurfaceFormatUsage
getVkSurfaceFormatUsage(latte::SurfaceFormat format)
{
   switch (format) {
   case latte::SurfaceFormat::R8Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R4G4Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::R16Unorm:
      return SurfaceFormatUsage::TD; // TODO: Should support TCD
   case latte::SurfaceFormat::R16Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R5G6B5Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R5G5B5A1Unorm:
      return SurfaceFormatUsage::T; // TODO: Should support TC
   case latte::SurfaceFormat::R4G4B4A4Unorm:
      return SurfaceFormatUsage::T; // TODO: Should support TC
   case latte::SurfaceFormat::A1B5G5R5Unorm:
      return SurfaceFormatUsage::T; // TODO: Should support TC
   case latte::SurfaceFormat::R32Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32Float:
      return SurfaceFormatUsage::TCD;
   case latte::SurfaceFormat::R16G16Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::D24UnormS8Uint:
      return SurfaceFormatUsage::DS;
   case latte::SurfaceFormat::X24G8Uint:
      return SurfaceFormatUsage::T; // TODO: Should support TC
   case latte::SurfaceFormat::R11G11B10Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R10G10B10A2Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R10G10B10A2Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R10G10B10A2Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R10G10B10A2Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8B8A8Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8B8A8Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8B8A8Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8B8A8Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R8G8B8A8Srgb:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::A2B10G10R10Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::A2B10G10R10Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::D32FloatS8UintX24:
      return SurfaceFormatUsage::TDS;
   case latte::SurfaceFormat::D32G8UintX24:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::R32G32Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32G32Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32G32Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16B16A16Unorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16B16A16Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16B16A16Snorm:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16B16A16Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R16G16B16A16Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32G32B32A32Uint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32G32B32A32Sint:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::R32G32B32A32Float:
      return SurfaceFormatUsage::TC;
   case latte::SurfaceFormat::BC1Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC1Srgb:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC2Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC2Srgb:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC3Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC3Srgb:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC4Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC4Snorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC5Unorm:
      return SurfaceFormatUsage::T;
   case latte::SurfaceFormat::BC5Snorm:
      return SurfaceFormatUsage::T;
      //case latte::SurfaceFormat::NV12:
         // Honestly have no clue how to support this format...
   }

   decaf_abort(fmt::format("Unexpected surface format {}", format));
}

vk::ComponentSwizzle
getVkComponentSwizzle(latte::SQ_SEL sel)
{
   switch (sel) {
   case latte::SQ_SEL::SEL_X:
      return vk::ComponentSwizzle::eR;
   case latte::SQ_SEL::SEL_Y:
      return vk::ComponentSwizzle::eG;
   case latte::SQ_SEL::SEL_Z:
      return vk::ComponentSwizzle::eB;
   case latte::SQ_SEL::SEL_W:
      return vk::ComponentSwizzle::eA;
   case latte::SQ_SEL::SEL_0:
      return vk::ComponentSwizzle::eZero;
   case latte::SQ_SEL::SEL_1:
      return vk::ComponentSwizzle::eOne;
   case latte::SQ_SEL::SEL_MASK:
      return vk::ComponentSwizzle::eIdentity;
   }

   decaf_abort(fmt::format("Unexpected component swizzle {}", sel));
}

vk::BorderColor
getVkBorderColor(latte::SQ_TEX_BORDER_COLOR color)
{
   switch (color) {
   case latte::SQ_TEX_BORDER_COLOR::TRANS_BLACK:
      return vk::BorderColor::eFloatTransparentBlack;
   case latte::SQ_TEX_BORDER_COLOR::OPAQUE_BLACK:
      return vk::BorderColor::eFloatOpaqueBlack;
   case latte::SQ_TEX_BORDER_COLOR::OPAQUE_WHITE:
      return vk::BorderColor::eFloatOpaqueWhite;
   case latte::SQ_TEX_BORDER_COLOR::REGISTER:
      decaf_abort("Unsupported register-based texture border color");
   default:
      decaf_abort("Unexpected texture border color type");
   }
}

vk::Filter
getVkXyTextureFilter(latte::SQ_TEX_XY_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_XY_FILTER::POINT:
      return vk::Filter::eNearest;
   case latte::SQ_TEX_XY_FILTER::BILINEAR:
      return vk::Filter::eLinear;
   case latte::SQ_TEX_XY_FILTER::BICUBIC:
      return vk::Filter::eCubicIMG;
   default:
      gLog->warn("Unexpected texture xy filter mode");
      return vk::Filter::eNearest;
      //decaf_abort("Unexpected texture xy filter mode");
   }
}

vk::SamplerMipmapMode
getVkZTextureFilter(latte::SQ_TEX_Z_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_Z_FILTER::NONE:
      return vk::SamplerMipmapMode::eNearest;
   case latte::SQ_TEX_Z_FILTER::POINT:
      return vk::SamplerMipmapMode::eNearest;
   case latte::SQ_TEX_Z_FILTER::LINEAR:
      return vk::SamplerMipmapMode::eLinear;
   default:
      decaf_abort("Unexpected texture xy filter mode");
   }
}

vk::SamplerAddressMode
getVkTextureAddressMode(latte::SQ_TEX_CLAMP clamp)
{
   switch (clamp) {
   case latte::SQ_TEX_CLAMP::WRAP:
      return vk::SamplerAddressMode::eRepeat;
   case latte::SQ_TEX_CLAMP::MIRROR:
      return vk::SamplerAddressMode::eMirroredRepeat;
   case latte::SQ_TEX_CLAMP::CLAMP_LAST_TEXEL:
      return vk::SamplerAddressMode::eClampToEdge;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_LAST_TEXEL:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   case latte::SQ_TEX_CLAMP::CLAMP_HALF_BORDER:
      return vk::SamplerAddressMode::eClampToBorder;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_HALF_BORDER:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   case latte::SQ_TEX_CLAMP::CLAMP_BORDER:
      return vk::SamplerAddressMode::eClampToBorder;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_BORDER:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   default:
      decaf_abort("Unexpected texture clamp mode");
   }
}

bool
getVkAnisotropyEnabled(latte::SQ_TEX_ANISO aniso)
{
   return aniso != latte::SQ_TEX_ANISO::ANISO_1_TO_1;
}

float
getVkMaxAnisotropy(latte::SQ_TEX_ANISO aniso)
{
   switch (aniso) {
   case latte::SQ_TEX_ANISO::ANISO_1_TO_1:
      return 1.0f;
   case latte::SQ_TEX_ANISO::ANISO_2_TO_1:
      return 2.0f;
   case latte::SQ_TEX_ANISO::ANISO_4_TO_1:
      return 4.0f;
   case latte::SQ_TEX_ANISO::ANISO_8_TO_1:
      return 8.0f;
   case latte::SQ_TEX_ANISO::ANISO_16_TO_1:
      return 16.0f;
   default:
      decaf_abort("Unexpected texture anisotropy mode");
   }
}

bool
getVkCompareOpEnabled(latte::REF_FUNC refFunc)
{
   return refFunc != latte::REF_FUNC::NEVER;
}

vk::CompareOp
getVkCompareOp(latte::REF_FUNC func)
{
   switch (func) {
   case latte::REF_FUNC::NEVER:
      return vk::CompareOp::eNever;
   case latte::REF_FUNC::LESS:
      return vk::CompareOp::eLess;
   case latte::REF_FUNC::EQUAL:
      return vk::CompareOp::eEqual;
   case latte::REF_FUNC::LESS_EQUAL:
      return vk::CompareOp::eLessOrEqual;
   case latte::REF_FUNC::GREATER:
      return vk::CompareOp::eGreater;
   case latte::REF_FUNC::NOT_EQUAL:
      return vk::CompareOp::eNotEqual;
   case latte::REF_FUNC::GREATER_EQUAL:
      return vk::CompareOp::eGreaterOrEqual;
   case latte::REF_FUNC::ALWAYS:
      return vk::CompareOp::eAlways;
   }

   decaf_abort(fmt::format("Unexpected compare op {}", func));
}

vk::StencilOp
getVkStencilOp(latte::DB_STENCIL_FUNC func)
{
   switch (func) {
   case latte::DB_STENCIL_FUNC::KEEP:
      return vk::StencilOp::eKeep;
   case latte::DB_STENCIL_FUNC::ZERO:
      return vk::StencilOp::eZero;
   case latte::DB_STENCIL_FUNC::REPLACE:
      return vk::StencilOp::eReplace;
   case latte::DB_STENCIL_FUNC::INCR_CLAMP:
      return vk::StencilOp::eIncrementAndClamp;
   case latte::DB_STENCIL_FUNC::DECR_CLAMP:
      return vk::StencilOp::eDecrementAndClamp;
   case latte::DB_STENCIL_FUNC::INVERT:
      return vk::StencilOp::eInvert;
   case latte::DB_STENCIL_FUNC::INCR_WRAP:
      return vk::StencilOp::eIncrementAndWrap;
   case latte::DB_STENCIL_FUNC::DECR_WRAP:
      return vk::StencilOp::eDecrementAndWrap;
   }

   decaf_abort(fmt::format("Unexpected stencil op {}", func));
}

vk::BlendFactor
getVkBlendFactor(latte::CB_BLEND_FUNC func)
{
   switch (func) {
   case latte::CB_BLEND_FUNC::ZERO:
      return vk::BlendFactor::eZero;
   case latte::CB_BLEND_FUNC::ONE:
      return vk::BlendFactor::eOne;
   case latte::CB_BLEND_FUNC::SRC_COLOR:
      return vk::BlendFactor::eSrcColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_COLOR:
      return vk::BlendFactor::eOneMinusSrcColor;
   case latte::CB_BLEND_FUNC::SRC_ALPHA:
      return vk::BlendFactor::eSrcAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_ALPHA:
      return vk::BlendFactor::eOneMinusSrcAlpha;
   case latte::CB_BLEND_FUNC::DST_ALPHA:
      return vk::BlendFactor::eDstAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_ALPHA:
      return vk::BlendFactor::eOneMinusDstAlpha;
   case latte::CB_BLEND_FUNC::DST_COLOR:
      return vk::BlendFactor::eDstColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_COLOR:
      return vk::BlendFactor::eOneMinusDstColor;
   case latte::CB_BLEND_FUNC::SRC_ALPHA_SATURATE:
      return vk::BlendFactor::eSrcAlphaSaturate;
   case latte::CB_BLEND_FUNC::BOTH_SRC_ALPHA:
      decaf_abort("Unsupported BOTH_SRC_ALPHA blend function");
   case latte::CB_BLEND_FUNC::BOTH_INV_SRC_ALPHA:
      decaf_abort("Unsupported BOTH_INV_SRC_ALPHA blend function");
   case latte::CB_BLEND_FUNC::CONSTANT_COLOR:
      return vk::BlendFactor::eConstantColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_COLOR:
      return vk::BlendFactor::eOneMinusConstantColor;
   case latte::CB_BLEND_FUNC::SRC1_COLOR:
      return vk::BlendFactor::eSrc1Color;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_COLOR:
      return vk::BlendFactor::eOneMinusSrc1Color;
   case latte::CB_BLEND_FUNC::SRC1_ALPHA:
      return vk::BlendFactor::eSrc1Alpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_ALPHA:
      return vk::BlendFactor::eOneMinusSrc1Alpha;
   case latte::CB_BLEND_FUNC::CONSTANT_ALPHA:
      return vk::BlendFactor::eConstantAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_ALPHA:
      return vk::BlendFactor::eOneMinusConstantAlpha;
   }

   decaf_abort(fmt::format("Unexpected blend factor {}", func));
}

vk::BlendOp
getVkBlendOp(latte::CB_COMB_FUNC func)
{
   switch (func) {
   case latte::CB_COMB_FUNC::DST_PLUS_SRC:
      return vk::BlendOp::eAdd;
   case latte::CB_COMB_FUNC::SRC_MINUS_DST:
      return vk::BlendOp::eSubtract;
   case latte::CB_COMB_FUNC::MIN_DST_SRC:
      return vk::BlendOp::eMin;
   case latte::CB_COMB_FUNC::MAX_DST_SRC:
      return vk::BlendOp::eMax;
   case latte::CB_COMB_FUNC::DST_MINUS_SRC:
      return vk::BlendOp::eReverseSubtract;
   }

   decaf_abort(fmt::format("Unexpected blend op {}", func));
}

vk::SampleCountFlags
getVkSampleCount(uint32_t samples)
{
   switch (samples) {
   case 1:
      return vk::SampleCountFlagBits::e1;
   case 2:
      return vk::SampleCountFlagBits::e2;
   case 4:
      return vk::SampleCountFlagBits::e4;
   case 8:
      return vk::SampleCountFlagBits::e8;
   case 16:
      return vk::SampleCountFlagBits::e16;
   case 32:
      return vk::SampleCountFlagBits::e32;
   case 64:
      return vk::SampleCountFlagBits::e64;
   }
   decaf_abort("Unexpected surface samples value");
}

static vk::Format
getSurfaceFormat(latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma,
                 vk::Format unorm,
                 vk::Format snorm,
                 vk::Format uint,
                 vk::Format sint,
                 vk::Format srgb,
                 vk::Format scaled)
{
   if (!degamma) {
      if (numFormat == latte::SQ_NUM_FORMAT::NORM) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return snorm;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return unorm;
         } else {
            return vk::Format::eUndefined;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::INT) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return sint;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return uint;
         } else {
            return vk::Format::eUndefined;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return scaled;
         } else {
            return vk::Format::eUndefined;
         }
      } else {
         return vk::Format::eUndefined;
      }
   } else {
      if (numFormat == 0 && formatComp == 0) {
         return srgb;
      } else {
         return vk::Format::eUndefined;
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
