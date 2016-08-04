#pragma once
#include "common/types.h"
#include "common/bitfield.h"
#include "latte_enum_vgt.h"

namespace latte
{

// Draw Inititiator
BITFIELD(VGT_DRAW_INITIATOR, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_DI_SRC_SEL, SOURCE_SELECT);
   BITFIELD_ENTRY(2, 2, VGT_DI_MAJOR_MODE, MAJOR_MODE);
   BITFIELD_ENTRY(4, 1, bool, SPRITE_EN_R6XX);
   BITFIELD_ENTRY(5, 1, bool, NOT_EOP);
   BITFIELD_ENTRY(6, 1, bool, USE_OPAQUE);
BITFIELD_END

// VGT DMA Base Address
union VGT_DMA_BASE
{
   uint32_t value;
   uint32_t BASE_ADDR;
};

// VGT DMA Base Address : upper 8-bits of 40 bit address
BITFIELD(VGT_DMA_BASE_HI, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, BASE_ADDR);
BITFIELD_END

// VGT DMA Index Type and Mode
BITFIELD(VGT_DMA_INDEX_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_INDEX, INDEX_TYPE);
   BITFIELD_ENTRY(2, 2, VGT_DMA_SWAP, SWAP_MODE);
BITFIELD_END

// VGT DMA Maximum Size
union VGT_DMA_MAX_SIZE
{
   uint32_t value;
   uint32_t MAX_SIZE;
};

// VGT DMA Number of Instances
union VGT_DMA_NUM_INSTANCES
{
   uint32_t value;
   uint32_t NUM_INSTANCES;
};

// VGT DMA Size
union VGT_DMA_SIZE
{
   uint32_t value;
   uint32_t NUM_INDICES;
};

// Event Initiator
BITFIELD(VGT_EVENT_INITIATOR, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_EVENT_TYPE, EVENT_TYPE);
   BITFIELD_ENTRY(8, 4, VGT_EVENT_INDEX, EVENT_INDEX)
   BITFIELD_ENTRY(19, 8, uint32_t, ADDRESS_HI);
   BITFIELD_ENTRY(27, 1, uint32_t, EXTENDED_EVENT);
BITFIELD_END

// VGT GS Enable Mode
BITFIELD(VGT_GS_MODE, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_GS_ENABLE_MODE, MODE);
   BITFIELD_ENTRY(2, 1, bool, ES_PASSTHRU);
   BITFIELD_ENTRY(3, 2, VGT_GS_CUT_MODE, CUT_MODE);
   BITFIELD_ENTRY(8, 1, bool, MODE_HI);
   BITFIELD_ENTRY(11, 1, bool, GS_C_PACK_EN);
   BITFIELD_ENTRY(14, 1, bool, COMPUTE_MODE);
   BITFIELD_ENTRY(15, 1, bool, FAST_COMPUTE_MODE);
   BITFIELD_ENTRY(16, 1, bool, ELEMENT_INFO_EN);
   BITFIELD_ENTRY(17, 1, bool, PARTIAL_THD_AT_EOI);
BITFIELD_END

// VGT GS output primitive type
BITFIELD(VGT_GS_OUT_PRIM_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_GS_OUT_PRIMITIVE_TYPE, PRIM_TYPE);
BITFIELD_END

BITFIELD(VGT_HOS_REUSE_DEPTH, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, REUSE_DEPTH);
BITFIELD_END

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
BITFIELD(VGT_MULTI_PRIM_IB_RESET_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, RESET_EN);
BITFIELD_END

// This register defines the index which resets primitive sets when MULTI_PRIM_IB is enabled.
union VGT_MULTI_PRIM_IB_RESET_INDX
{
   uint32_t value;
   uint32_t RESET_INDX;
};

// VGT Number of Indices
union VGT_NUM_INDICES
{
   uint32_t value;
   uint32_t NUM_INDICES;
};

// Primitive ID generation is enabled
BITFIELD(VGT_PRIMITIVEID_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, PRIMITIVEID_EN);
BITFIELD_END

// VGT Primitive Type
BITFIELD(VGT_PRIMITIVE_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_DI_PRIMITIVE_TYPE, PRIM_TYPE);
BITFIELD_END

// This register enables streaming out
BITFIELD(VGT_STRMOUT_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, STREAMOUT);
BITFIELD_END

// Stream out enable bits.
BITFIELD(VGT_STRMOUT_BUFFER_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, BUFFER_0_EN);
   BITFIELD_ENTRY(1, 1, bool, BUFFER_1_EN);
   BITFIELD_ENTRY(2, 1, bool, BUFFER_2_EN);
   BITFIELD_ENTRY(3, 1, bool, BUFFER_3_EN);
BITFIELD_END

// This register controls the behavior of the Vertex Reuse block at the backend of the VGT.
BITFIELD(VGT_VERTEX_REUSE_BLOCK_CNTL, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, VTX_REUSE_DEPTH);
BITFIELD_END

} // namespace latte
