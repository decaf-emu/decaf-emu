#pragma once
#include "types.h"
#include "modules/gx2/gx2_enum.h"

namespace latte
{

namespace Register
{
enum Value : uint32_t
{
   // Config Registers
   ConfigRegisterBase = 0x8000,
   PrimitiveType = 0x8958,
   NumIndices = 0x8970,
   ConfigRegisterEnd = 0x8a000,

   // Context Registers
   ContextRegisterBase = 0x28000,
   PrimitiveRestartIndex = 0x2840c,
   Blend0Control = 0x028780,
   Blend1Control = 0x028784,
   Blend2Control = 0x028788,
   Blend3Control = 0x02878C,
   Blend4Control = 0x028790,
   Blend5Control = 0x028794,
   Blend6Control = 0x028798,
   Blend7Control = 0x02879C,
   DepthControl = 0x28800,
   ContextRegisterEnd = 0x29000,
};
}

// CB_BLEND_CONTROL
union BlendControl
{
   uint32_t value;

   struct
   {
      GX2BlendMode::Value colorSrcBlend : 5;
      GX2BlendCombineMode::Value colorCombine : 3;
      GX2BlendMode::Value colorDstBlend : 5;
      uint32_t : 3;
      GX2BlendMode::Value alphaSrcBlend : 5;
      GX2BlendCombineMode::Value alphaCombine : 3;
      GX2BlendMode::Value alphaDstBlend : 5;
      uint32_t useAlphaBlend : 1;
      uint32_t : 2;
   };
};

// DB_DEPTH_CONTROL
union DepthControl
{
   uint32_t value;

   struct
   {
      uint32_t stencilTest : 1;
      uint32_t depthTest : 1;
      uint32_t depthWrite : 1;
      uint32_t : 1;
      GX2CompareFunction::Value depthCompare : 3;
      uint32_t backfaceStencil : 1;
      GX2CompareFunction::Value frontStencilFunc : 3;
      GX2StencilFunction::Value frontStencilFail : 3;
      GX2StencilFunction::Value frontStencilZPass : 3;
      GX2StencilFunction::Value frontStencilZFail : 3;
      GX2CompareFunction::Value backStencilFunc : 3;
      GX2StencilFunction::Value backStencilFail : 3;
      GX2StencilFunction::Value backStencilZPass : 3;
      GX2StencilFunction::Value backStencilZFail : 3;
   };
};

// VGT_DMA_INDEX_TYPE
union DmaIndexType
{
   uint32_t value;

   struct
   {
      uint32_t indexType : 2;
      uint32_t swapMode : 2;
      uint32_t : 28;
   };
};

// VGT_DRAW_INITIATOR
union DrawInitiator
{
   uint32_t value;

   struct
   {
      uint32_t sourceSelect : 2;
      uint32_t majorMode : 2;
      uint32_t spriteEnabled : 1;
      uint32_t notEndOfPacket : 1;
      uint32_t useOpaque : 1;
      uint32_t : 25;
   };
};

} // namespace latte
