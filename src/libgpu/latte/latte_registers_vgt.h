#pragma once
#include "latte_enum_vgt.h"

#include <common/bitfield.h>
#include <cstdint>

namespace latte
{

// Draw Inititiator
BITFIELD_BEG(VGT_DRAW_INITIATOR, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_DI_SRC_SEL, SOURCE_SELECT)
   BITFIELD_ENTRY(2, 2, VGT_DI_MAJOR_MODE, MAJOR_MODE)
   BITFIELD_ENTRY(4, 1, bool, SPRITE_EN_R6XX)
   BITFIELD_ENTRY(5, 1, bool, NOT_EOP)
   BITFIELD_ENTRY(6, 1, bool, USE_OPAQUE)
BITFIELD_END

// VGT DMA Base Address
BITFIELD_BEG(VGT_DMA_BASE, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, BASE_ADDR)
BITFIELD_END

// VGT DMA Base Address : upper 8-bits of 40 bit address
BITFIELD_BEG(VGT_DMA_BASE_HI, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, BASE_ADDR)
BITFIELD_END

// VGT DMA Index Type and Mode
BITFIELD_BEG(VGT_DMA_INDEX_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_INDEX_TYPE, INDEX_TYPE)
   BITFIELD_ENTRY(2, 2, VGT_DMA_SWAP, SWAP_MODE)
BITFIELD_END

// VGT DMA Maximum Size
BITFIELD_BEG(VGT_DMA_MAX_SIZE, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, MAX_SIZE)
BITFIELD_END

// VGT DMA Number of Instances
BITFIELD_BEG(VGT_DMA_NUM_INSTANCES, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, NUM_INSTANCES)
BITFIELD_END

// VGT DMA Size
BITFIELD_BEG(VGT_DMA_SIZE, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, NUM_INDICES)
BITFIELD_END

// Maximum ES vertices per GS thread
BITFIELD_BEG(VGT_ES_PER_GS, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, ES_PER_GS)
BITFIELD_END

// Event Initiator
BITFIELD_BEG(VGT_EVENT_INITIATOR, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_EVENT_TYPE, EVENT_TYPE)
   BITFIELD_ENTRY(8, 4, VGT_EVENT_INDEX, EVENT_INDEX)
   BITFIELD_ENTRY(19, 8, uint32_t, ADDRESS_HI)
   BITFIELD_ENTRY(27, 1, uint32_t, EXTENDED_EVENT)
BITFIELD_END

// VGT GS Enable Mode
BITFIELD_BEG(VGT_GS_MODE, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_GS_ENABLE_MODE, MODE)
   BITFIELD_ENTRY(2, 1, bool, ES_PASSTHRU)
   BITFIELD_ENTRY(3, 2, VGT_GS_CUT_MODE, CUT_MODE)
   BITFIELD_ENTRY(8, 1, bool, MODE_HI)
   BITFIELD_ENTRY(11, 1, bool, GS_C_PACK_EN)
   BITFIELD_ENTRY(14, 1, bool, COMPUTE_MODE)
   BITFIELD_ENTRY(15, 1, bool, FAST_COMPUTE_MODE)
   BITFIELD_ENTRY(16, 1, bool, ELEMENT_INFO_EN)
   BITFIELD_ENTRY(17, 1, bool, PARTIAL_THD_AT_EOI)
BITFIELD_END

// VGT GS output primitive type
BITFIELD_BEG(VGT_GS_OUT_PRIM_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_GS_OUT_PRIMITIVE_TYPE, PRIM_TYPE)
BITFIELD_END

// Maximum GS prims per ES thread
BITFIELD_BEG(VGT_GS_PER_ES, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, GS_PER_ES)
BITFIELD_END

// Maximum GS prims per VS thread
BITFIELD_BEG(VGT_GS_PER_VS, uint32_t)
   BITFIELD_ENTRY(0, 4, uint8_t, GS_PER_VS)
BITFIELD_END

// Reuseability for GS path, it is nothing to do with number of good simd
BITFIELD_BEG(VGT_GS_VERTEX_REUSE, uint32_t)
   BITFIELD_ENTRY(0, 5, uint8_t, VERT_REUSE)
BITFIELD_END

BITFIELD_BEG(VGT_HOS_REUSE_DEPTH, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, REUSE_DEPTH)
BITFIELD_END

// For continuous and discrete tessellation modes, this register contains the tessellation level.
// For adaptive tessellation, this register contains the maximum tessellation level.
BITFIELD_BEG(VGT_HOS_MAX_TESS_LEVEL, uint32_t)
   BITFIELD_ENTRY(0, 32, float, MAX_TESS)
BITFIELD_END

// For continuous and discrete tessellation modes, this register is not applicable.
// For adaptive tessellation, this register contains the minimum tessellation level.
BITFIELD_BEG(VGT_HOS_MIN_TESS_LEVEL, uint32_t)
   BITFIELD_ENTRY(0, 32, float, MIN_TESS)
BITFIELD_END

// For components that are that are specified to be indices (see the VGT_GROUP_VECT_0_FMT_CNTL register), this register is the offset value.
BITFIELD_BEG(VGT_INDX_OFFSET, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, INDX_OFFSET)
BITFIELD_END

// For components that are that are specified to be indices (see the VGT_GROUP_VECT_0_FMT_CNTL register), this register is the maximum clamp value.
BITFIELD_BEG(VGT_MAX_VTX_INDX, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, MAX_INDX)
BITFIELD_END

// For components that are that are specified to be indices (see the VGT_GROUP_VECT_0_FMT_CNTL register), this register is the minimum clamp value.
BITFIELD_BEG(VGT_MIN_VTX_INDX, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, MIN_INDX)
BITFIELD_END

// This register enabling reseting of prim based on reset index
BITFIELD_BEG(VGT_MULTI_PRIM_IB_RESET_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, RESET_EN)
BITFIELD_END

// This register defines the index which resets primitive sets when MULTI_PRIM_IB is enabled.
BITFIELD_BEG(VGT_MULTI_PRIM_IB_RESET_INDX, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, RESET_INDX)
BITFIELD_END

// VGT Number of Indices
BITFIELD_BEG(VGT_NUM_INDICES, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, NUM_INDICES)
BITFIELD_END

// This register controls, within a process vector, when the previous process vector is de-allocated.
BITFIELD_BEG(VGT_OUT_DEALLOC_CNTL, uint32_t)
   BITFIELD_ENTRY(0, 7, uint32_t, DEALLOC_DIST)
BITFIELD_END

// This register selects which backend path will be used by the VGT block.
BITFIELD_BEG(VGT_OUTPUT_PATH_CNTL, uint32_t)
   BITFIELD_ENTRY(0, 2, VGT_OUTPUT_PATH_SELECT, PATH_SELECT)
BITFIELD_END

// Primitive ID generation is enabled
BITFIELD_BEG(VGT_PRIMITIVEID_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, PRIMITIVEID_EN)
BITFIELD_END

// VGT Primitive Type
BITFIELD_BEG(VGT_PRIMITIVE_TYPE, uint32_t)
   BITFIELD_ENTRY(0, 6, VGT_DI_PRIMITIVE_TYPE, PRIM_TYPE)
BITFIELD_END

// VGT reuse is off. This will expand strip primitives to list primitives
BITFIELD_BEG(VGT_REUSE_OFF, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, REUSE_OFF)
BITFIELD_END

// This register enables streaming out
BITFIELD_BEG(VGT_STRMOUT_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, STREAMOUT)
BITFIELD_END

// Stream out enable bits.
BITFIELD_BEG(VGT_STRMOUT_BUFFER_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, BUFFER_0_EN)
   BITFIELD_ENTRY(1, 1, bool, BUFFER_1_EN)
   BITFIELD_ENTRY(2, 1, bool, BUFFER_2_EN)
   BITFIELD_ENTRY(3, 1, bool, BUFFER_3_EN)
BITFIELD_END

// Draw opaque offset.
BITFIELD_BEG(VGT_STRMOUT_DRAW_OPAQUE_OFFSET, uint32_t)
   BITFIELD_ENTRY(0, 32, uint32_t, OFFSET)
BITFIELD_END

// This register controls the behavior of the Vertex Reuse block at the backend of the VGT.
BITFIELD_BEG(VGT_VERTEX_REUSE_BLOCK_CNTL, uint32_t)
   BITFIELD_ENTRY(0, 8, uint32_t, VTX_REUSE_DEPTH)
BITFIELD_END

// Auto-index generation is on.
BITFIELD_BEG(VGT_VTX_CNT_EN, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, VTX_CNT_EN)
BITFIELD_END

} // namespace latte
