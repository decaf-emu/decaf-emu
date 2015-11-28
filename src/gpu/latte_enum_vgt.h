#pragma once
#include "types.h"

namespace latte
{

enum VGT_DI_MAJOR_MODE : uint32_t
{
   VGT_DI_MAJOR_MODE0               = 0,
   VGT_DI_MAJOR_MODE1               = 1,
};

enum VGT_DI_PRIMITIVE_TYPE
{
   VGT_DI_PT_NONE                   = 0,
   VGT_DI_PT_POINTLIST              = 1,
   VGT_DI_PT_LINELIST               = 2,
   VGT_DI_PT_LINESTRIP              = 3,
   VGT_DI_PT_TRILIST                = 4,
   VGT_DI_PT_TRIFAN                 = 5,
   VGT_DI_PT_TRISTRIP               = 6,
   VGT_DI_PT_UNUSED_0               = 7,
   VGT_DI_PT_UNUSED_1               = 8,
   VGT_DI_PT_UNUSED_2               = 9,
   VGT_DI_PT_LINELIST_ADJ           = 10,
   VGT_DI_PT_LINESTRIP_ADJ          = 11,
   VGT_DI_PT_TRILIST_ADJ            = 12,
   VGT_DI_PT_TRISTRIP_ADJ           = 13,
   VGT_DI_PT_UNUSED_3               = 14,
   VGT_DI_PT_UNUSED_4               = 15,
   VGT_DI_PT_TRI_WITH_WFLAGS        = 16,
   VGT_DI_PT_RECTLIST               = 17,
   VGT_DI_PT_LINELOOP               = 18,
   VGT_DI_PT_QUADLIST               = 19,
   VGT_DI_PT_QUADSTRIP              = 20,
   VGT_DI_PT_POLYGON                = 21,
   VGT_DI_PT_2D_COPY_RECT_LIST_V0   = 22,
   VGT_DI_PT_2D_COPY_RECT_LIST_V1   = 23,
   VGT_DI_PT_2D_COPY_RECT_LIST_V2   = 24,
   VGT_DI_PT_2D_COPY_RECT_LIST_V3   = 25,
   VGT_DI_PT_2D_FILL_RECT_LIST      = 26,
   VGT_DI_PT_2D_LINE_STRIP          = 27,
   VGT_DI_PT_2D_TRI_STRIP           = 28,
};

enum VGT_DI_SRC_SEL : uint32_t
{
   VGT_DI_SRC_SEL_DMA               = 0,
   VGT_DI_SRC_SEL_IMMEDIATE         = 1,
   VGT_DI_SRC_SEL_AUTO_INDEX        = 2,
   VGT_DI_SRC_SEL_RESERVED          = 3,
};

enum VGT_DMA_SWAP : uint32_t
{
   VGT_DMA_SWAP_NONE                = 0,
   VGT_DMA_SWAP_16_BIT              = 1,
   VGT_DMA_SWAP_32_BIT              = 2,
   VGT_DMA_SWAP_WORD                = 3,
};

enum VGT_INDEX : uint32_t
{
   VGT_INDEX_16                     = 0,
   VGT_INDEX_32                     = 1,
};

} // namespace latte
