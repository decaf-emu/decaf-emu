#pragma once
#include "types.h"
#include "latte_enum_vgt.h"

namespace latte
{

// Draw Inititiator
union VGT_DRAW_INITIATOR
{
   uint32_t value;

   struct
   {
      VGT_DI_SRC_SEL SOURCE_SELECT : 2;
      VGT_DI_MAJOR_MODE MAJOR_MODE : 2;
      uint32_t SPRITE_EN_R6XX : 1;
      uint32_t NOT_EOP : 1;
      uint32_t USE_OPAQUE : 1;
      uint32_t : 25;
   };
};

// VGT DMA Base Address
struct VGT_DMA_BASE
{
   uint32_t BASE_ADDR;
};

// VGT DMA Base Address : upper 8-bits of 40 bit address
union VGT_DMA_BASE_HI
{
   uint32_t value;

   struct
   {
      uint32_t BASE_ADDR : 8;
      uint32_t : 24;
   };
};

// VGT DMA Index Type and Mode
union VGT_DMA_INDEX_TYPE
{
   uint32_t value;

   struct
   {
      VGT_INDEX INDEX_TYPE : 2;
      VGT_DMA_SWAP SWAP_MODE : 2;
      uint32_t : 28;
   };
};

// VGT DMA Maximum Size
struct VGT_DMA_MAX_SIZE
{
   uint32_t MAX_SIZE;
};

// VGT DMA Number of Instances
struct VGT_DMA_NUM_INSTANCES
{
   uint32_t NUM_INSTANCES;
};

// VGT DMA Size
struct VGT_DMA_SIZE
{
   uint32_t NUM_INDICES;
};

// Event Initiator
union VGT_EVENT_INITIATOR
{
   uint32_t value;

   struct
   {
      VGT_EVENT_TYPE EVENT_TYPE : 6;
      uint32_t : 13;
      uint32_t ADDRESS_HI : 8;
      uint32_t EXTENDED_EVENT : 1;
      uint32_t : 4;
   };
};

// VGT GS Enable Mode
union VGT_GS_MODE
{
   uint32_t value;

   struct
   {
      VGT_GS_ENABLE_MODE MODE : 2;
      uint32_t ES_PASSTHRU : 1;
      VGT_GS_CUT_MODE CUT_MODE : 2;
      uint32_t : 3;
      uint32_t MODE_HI : 1;
      uint32_t : 2;
      uint32_t GS_C_PACK_EN : 1;
      uint32_t : 2;
      uint32_t COMPUTE_MODE : 1;
      uint32_t FAST_COMPUTE_MODE : 1;
      uint32_t ELEMENT_INFO_EN : 1;
      uint32_t PARTIAL_THD_AT_EOI : 1;
      uint32_t : 14;
   };
};

// VGT GS output primitive type
union VGT_GS_OUT_PRIM_TYPE
{
   uint32_t value;

   struct
   {
      VGT_GS_OUT_PRIMITIVE_TYPE PRIM_TYPE : 6;
      uint32_t : 26;
   };
};

union VGT_HOS_REUSE_DEPTH
{
   uint32_t value;

   struct
   {
      uint32_t REUSE_DEPTH : 8;
      uint32_t : 24;
   };
};

// For continuous and discrete tessellation modes, this register contains the tessellation level.
// For adaptive tessellation, this register contains the maximum tessellation level.
union VGT_HOS_MAX_TESS_LEVEL
{
   uint32_t value;
   float MAX_TESS;
};

// For continuous and discrete tessellation modes, this register is not applicable.
// For adaptive tessellation, this register contains the minimum tessellation level.
union VGT_HOS_MIN_TESS_LEVEL
{
   uint32_t value;
   float MIN_TESS;
};

// This register enabling reseting of prim based on reset index
union VGT_MULTI_PRIM_IB_RESET_EN
{
   uint32_t value;

   struct
   {
      uint32_t RESET_EN :1;
      uint32_t : 30;
   };
};

// This register defines the index which resets primitive sets when MULTI_PRIM_IB is enabled.
struct VGT_MULTI_PRIM_IB_RESET_INDX
{
   uint32_t RESET_INDX;
};

// VGT Number of Indices
struct VGT_NUM_INDICES
{
   uint32_t NUM_INDICES;
};

// Primitive ID generation is enabled
union VGT_PRIMITIVEID_EN
{
   uint32_t value;

   struct
   {
      uint32_t PRIMITIVEID_EN : 1;
      uint32_t : 31;
   };
};

// VGT Primitive Type
union VGT_PRIMITIVE_TYPE
{
   uint32_t value;

   struct
   {
      VGT_DI_PRIMITIVE_TYPE PRIM_TYPE : 6;
      uint32_t : 26;
   };
};

// This register enables streaming out
union VGT_STRMOUT_EN
{
   uint32_t value;

   struct
   {
      uint32_t STREAMOUT : 1;
      uint32_t : 31;
   };
};

// Stream out enable bits.
union VGT_STRMOUT_BUFFER_EN
{
   uint32_t value;

   struct
   {
      uint32_t BUFFER_0_EN : 1;
      uint32_t BUFFER_1_EN : 1;
      uint32_t BUFFER_2_EN : 1;
      uint32_t BUFFER_3_EN : 1;
      uint32_t: 28;
   };
};

// This register controls the behavior of the Vertex Reuse block at the backend of the VGT.
union VGT_VERTEX_REUSE_BLOCK_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t VTX_REUSE_DEPTH : 8;
      uint32_t: 24;
   };
};

} // namespace latte
