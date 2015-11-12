#pragma once
#include "types.h"

namespace latte
{

namespace Register
{

enum Value : uint32_t
{
   // Config Registers
   ConfigRegisterBase         = 0x08000,
   PrimitiveType              = 0x08958,
   NumIndices                 = 0x08970,
   ConfigRegisterEnd          = 0x08a00,

   // Context Registers
   ContextRegisterBase        = 0x28000,
   PrimitiveResetIndex        = 0x2840c, // VGT_MULTI_PRIM_IB_RESET_INDX
   BlendColorRed              = 0x28414, // CB_BLEND_RED
   BlendColorGreen            = 0x28418, // CB_BLEND_GREEN
   BlendColorBlue             = 0x2841C, // CB_BLEND_BLUE
   BlendColorAlpha            = 0x28420, // CB_BLEND_ALPHA
   Blend0Control              = 0x28780, // CB_BLEND0_CONTROL
   Blend1Control              = 0x28784, // CB_BLEND1_CONTROL
   Blend2Control              = 0x28788, // CB_BLEND2_CONTROL
   Blend3Control              = 0x2878C, // CB_BLEND3_CONTROL
   Blend4Control              = 0x28790, // CB_BLEND4_CONTROL
   Blend5Control              = 0x28794, // CB_BLEND5_CONTROL
   Blend6Control              = 0x28798, // CB_BLEND6_CONTROL
   Blend7Control              = 0x2879C, // CB_BLEND7_CONTROL
   DrawInitiator              = 0x287f0, // VGT_DRAW_INITIATOR
   DepthControl               = 0x28800, // DB_DEPTH_CONTROL
   BlendControl               = 0x28804, // CB_BLEND_CONTROL
   ColorControl               = 0x28808, // CB_COLOR_CONTROL
   DmaBaseHi                  = 0x287e4, // VGT_DMA_BASE_HI
   DmaBase                    = 0x287e8, // VGT_DMA_BASE
   DmaSize                    = 0x28a74, // VGT_DMA_SIZE
   DmaMaxSize                 = 0x28a78, // VGT_DMA_MAX_SIZE
   DmaIndexType               = 0x28a7c, // VGT_DMA_INDEX_TYPE
   DmaNumInstances            = 0x28a88, // VGT_DMA_NUM_INSTANCES
   PrimitiveResetEnable       = 0x28a94, // VGT_MULTI_PRIM_IB_RESET_EN
   ContextRegisterEnd         = 0x29000,
};
}

enum BLEND_FUNC
{
   BLEND_ZERO = 0,
   BLEND_ONE = 1,
   BLEND_SRC_COLOR = 2,
   BLEND_ONE_MINUS_SRC_COLOR = 3,
   BLEND_SRC_ALPHA = 4,
   BLEND_ONE_MINUS_SRC_ALPHA = 5,
   BLEND_DST_ALPHA = 6,
   BLEND_ONE_MINUS_DST_ALPHA = 7,
   BLEND_DST_COLOR = 8,
   BLEND_ONE_MINUS_DST_COLOR = 9,
   BLEND_SRC_ALPHA_SATURATE = 10,
   BLEND_BOTH_SRC_ALPHA = 11,
   BLEND_BOTH_INV_SRC_ALPHA = 12,
   BLEND_CONSTANT_COLOR = 13,
   BLEND_ONE_MINUS_CONSTANT_COLOR = 14,
   BLEND_SRC1_COLOR = 15,
   BLEND_ONE_MINUS_SRC1_COLOR = 16,
   BLEND_SRC1_ALPHA = 17,
   BLEND_ONE_MINUS_SRC1_ALPHA = 18,
   BLEND_CONSTANT_ALPHA = 19,
   BLEND_ONE_MINUS_CONSTANT_ALPHA = 20,
};

enum COMB_FUNC
{
   COMB_DST_PLUS_SRC = 0,
   COMB_SRC_MINUS_DST = 1,
   COMB_MIN_DST_SRC = 2,
   COMB_MAX_DST_SRC = 3,
   COMB_DST_MINUS_SRC = 4,
};

struct CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::BlendControl;

   union
   {
      uint32_t value;

      struct
      {
         BLEND_FUNC colorSrcBlend : 5;
         COMB_FUNC colorCombine : 3;
         BLEND_FUNC colorDstBlend : 5;
         uint32_t : 3;
         BLEND_FUNC alphaSrcBlend : 5;
         COMB_FUNC alphaCombine : 3;
         BLEND_FUNC alphaDstBlend : 5;
         uint32_t useAlphaBlend : 1;
         uint32_t : 2;
      };
   };
};

struct CB_BLEND0_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend0Control;
};

struct CB_BLEND1_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend1Control;
};

struct CB_BLEND2_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend2Control;
};

struct CB_BLEND3_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend3Control;
};

struct CB_BLEND4_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend4Control;
};

struct CB_BLEND5_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend5Control;
};

struct CB_BLEND6_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend6Control;
};

struct CB_BLEND7_CONTROL : public CB_BLEND_CONTROL
{
   static const auto RegisterID = Register::Blend7Control;
};

enum SPECIAL_OP
{
   SPECIAL_NORMAL = 0,
   SPECIAL_DISABLE = 1,
   SPECIAL_FAST_CLEAR = 2,
   SPECIAL_FORCE_CLEAR = 3,
   SPECIAL_EXPAND_COLOR = 4,
   SPECIAL_EXPAND_TEXTURE = 5,
   SPECIAL_EXPAND_SAMPLES = 6,
   SPECIAL_RESOLVE_BOX = 7,
};

union CB_COLOR_CONTROL
{
   static const auto RegisterID = Register::ColorControl;
   uint32_t value;

   struct
   {
      uint32_t fogEnable : 1;
      uint32_t multiwriteEnable : 1;
      uint32_t ditherEnable : 1;
      uint32_t degammaEnable : 1;
      uint32_t specialOp : 3;
      uint32_t perMrtBlend : 1;
      uint32_t targetBlendEnable : 8;
      uint32_t rop3 : 8;
      uint32_t : 8;
   };
};

enum FRAG_FUNC : uint32_t
{
   FRAG_NEVER = 0,
   FRAG_LESS = 1,
   FRAG_EQUAL = 2,
   FRAG_LEQUAL = 3,
   FRAG_GREATER = 4,
   FRAG_NOTEQUAL = 5,
   FRAG_GEQUAL = 6,
   FRAG_ALWAYS = 7,
};

enum REF_FUNC : uint32_t
{
   REF_NEVER = 0,
   REF_LESS = 1,
   REF_EQUAL = 2,
   REF_LEQUAL = 3,
   REF_GREATER = 4,
   REF_NOTEQUAL = 5,
   REF_GEQUAL = 6,
   REF_ALWAYS = 7,
};

enum STENCIL_FUNC : uint32_t
{
   STENCIL_KEEP = 0,
   STENCIL_ZERO = 1,
   STENCIL_REPLACE = 2,
   STENCIL_INCR_CLAMP = 3,
   STENCIL_DECR_CLAMP = 4,
   STENCIL_INVERT = 5,
   STENCIL_INCR_WRAP = 6,
   STENCIL_DECR_WRAP = 7,
};

union DB_DEPTH_CONTROL
{
   static const auto RegisterID = Register::DepthControl;
   uint32_t value;

   struct
   {
      uint32_t stencilTest : 1;
      uint32_t depthTest : 1;
      uint32_t depthWrite : 1;
      uint32_t : 1;
      FRAG_FUNC depthCompare : 3;
      uint32_t backfaceStencil : 1;
      REF_FUNC frontStencilFunc : 3;
      STENCIL_FUNC frontStencilFail : 3;
      STENCIL_FUNC frontStencilZPass : 3;
      STENCIL_FUNC frontStencilZFail : 3;
      REF_FUNC backStencilFunc : 3;
      STENCIL_FUNC backStencilFail : 3;
      STENCIL_FUNC backStencilZPass : 3;
      STENCIL_FUNC backStencilZFail : 3;
   };
};

enum VGT_INDEX : uint32_t
{
   VGT_INDEX_16 = 0,
   VGT_INDEX_32 = 1,
};

enum VGT_DMA_SWAP : uint32_t
{
   VGT_DMA_SWAP_NONE = 0,
   VGT_DMA_SWAP_16_BIT = 1,
   VGT_DMA_SWAP_32_BIT = 2,
   VGT_DMA_SWAP_WORD = 3,
};

union VGT_DMA_INDEX_TYPE
{
   uint32_t value;

   struct
   {
      VGT_INDEX indexType : 2;
      VGT_DMA_SWAP swapMode : 2;
      uint32_t : 28;
   };
};

struct VGT_DMA_MAX_SIZE
{
   static const auto RegisterID = Register::DmaMaxSize;
   uint32_t maxSize;
};

struct VGT_DMA_BASE
{
   static const auto RegisterID = Register::DmaBase;
   uint32_t baseAddr;
};

struct VGT_DMA_NUM_INSTANCES
{
   static const auto RegisterID = Register::DmaNumInstances;
   uint32_t numInstances;
};

union VGT_DMA_BASE_HI
{
   static const auto RegisterID = Register::DmaBaseHi;
   uint32_t value;

   struct
   {
      uint32_t baseAddrHi : 8;
      uint32_t : 24;
   };
};

union VGT_MULTI_PRIM_IB_RESET_EN
{
   static const auto RegisterID = Register::PrimitiveResetEnable;
   uint32_t value;

   struct
   {
      uint32_t enable : 8;
      uint32_t : 24;
   };
};

union VGT_MULTI_PRIM_IB_RESET_INDX
{
   static const auto RegisterID = Register::PrimitiveResetIndex;
   uint32_t index;
};

struct VGT_DMA_SIZE
{
   static const auto RegisterID = Register::DmaSize;
   uint32_t numIndices;
};

enum DI_SRC_SEL : uint32_t
{
   DI_SRC_SEL_DMA = 0,
   DI_SRC_SEL_IMMEDIATE = 1,
   DI_SRC_SEL_AUTO_INDEX = 2,
   DI_SRC_SEL_RESERVED = 3,
};

enum DI_MAJOR_MODE : uint32_t
{
   DI_MAJOR_MODE0 = 0,
   DI_MAJOR_MODE1 = 1,
};

union VGT_DRAW_INITIATOR
{
   static const auto RegisterID = Register::DrawInitiator;
   uint32_t value;

   struct
   {
      DI_SRC_SEL sourceSelect : 2;
      DI_MAJOR_MODE majorMode : 2;
      uint32_t spriteEnabled : 1;
      uint32_t notEndOfPacket : 1;
      uint32_t useOpaque : 1;
      uint32_t : 25;
   };
};

enum SQ_VTX_CLAMP : uint32_t
{
   SQ_VTX_CLAMP_ZERO = 0,
   SQ_VTX_CLAMP_NAN = 1,
};

enum SQ_NUM_FORMAT : uint32_t
{
   SQ_NUM_FORMAT_NORM = 0,
   SQ_NUM_FORMAT_INT = 1,
   SQ_NUM_FORMAT_SCALED = 2,
};

enum SQ_FORMAT_COMP : uint32_t
{
   SQ_FORMAT_COMP_UNSIGNED = 0,
   SQ_FORMAT_COMP_SIGNED = 1,
};

enum SQ_SRF_MODE : uint32_t
{
   SQ_SRF_MODE_ZERO_CLAMP_MINUS_ONE = 0,
   SQ_SRF_MODE_NO_ZERO = 1,
};

enum SQ_ENDIAN : uint32_t
{
   SQ_ENDIAN_NONE = 0,
   SQ_ENDIAN_8IN16 = 1,
   SQ_ENDIAN_8IN32 = 2,
};

union SQ_VTX_CONSTANT_WORD2_0
{
   uint32_t value;

   struct
   {
      uint32_t baseAddressHi : 8;
      uint32_t stride : 11;
      SQ_VTX_CLAMP clampX : 1;
      uint32_t dataFormat : 6;
      SQ_NUM_FORMAT numFormatAll : 2;
      SQ_FORMAT_COMP formatCompAll : 1;
      SQ_SRF_MODE srfModeAll : 1;
      SQ_ENDIAN endianSwap : 2;
   };
};

union SQ_VTX_CONSTANT_WORD3_0
{
   uint32_t value;

   struct
   {
      uint32_t memRequestSize : 2;
      uint32_t uncached : 1;
      uint32_t : 29;
   };
};

enum SQ_TEX_VTX_TYPE : uint32_t
{
   SQ_TEX_VTX_INVALID_TEXTURE = 0,
   SQ_TEX_VTX_INVALID_BUFFER = 1,
   SQ_TEX_VTX_VALID_TEXTURE = 2,
   SQ_TEX_VTX_VALID_BUFFER = 3,
};

union SQ_VTX_CONSTANT_WORD6_0
{
   uint32_t value;

   struct
   {
      uint32_t : 30;
      SQ_TEX_VTX_TYPE type : 2;
   };
};

} // namespace latte
