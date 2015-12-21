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

enum VGT_GS_CUT_MODE : uint32_t
{
   GS_CUT_1024                      = 0,
   GS_CUT_512                       = 1,
   GS_CUT_256                       = 2,
   GS_CUT_128                       = 3,
};

enum VGT_GS_ENABLE_MODE : uint32_t
{
   GS_OFF                           = 0,
   GS_SCENARIO_A                    = 1,
   GS_SCENARIO_B                    = 2,
   GS_SCENARIO_G                    = 3,
};

enum VGT_EVENT_TYPE : uint32_t
{
   CACHE_FLUSH_TS                   = 4,
   CONTEXT_DONE                     = 5,
   CACHE_FLUSH                      = 6,
   VIZQUERY_START                   = 7,
   VIZQUERY_END                     = 8,
   SC_WAIT_WC                       = 9,
   MPASS_PS_CP_REFETCH              = 10,
   MPASS_PS_RST_START               = 11,
   MPASS_PS_INCR_START              = 12,
   RST_PIX_CNT                      = 13,
   RST_VTX_CNT                      = 14,
   VS_PARTIAL_FLUSH                 = 15,
   PS_PARTIAL_FLUSH                 = 16,
   CACHE_FLUSH_AND_INV_TS_EVENT     = 20,
   ZPASS_DONE                       = 21,
   CACHE_FLUSH_AND_INV_EVENT        = 22,
   PERFCOUNTER_START                = 23,
   PERFCOUNTER_STOP                 = 24,
   PIPELINESTAT_START               = 25,
   PIPELINESTAT_STOP                = 26,
   PERFCOUNTER_SAMPLE               = 27,
   FLUSH_ES_OUTPUT                  = 28,
   FLUSH_GS_OUTPUT                  = 29,
   SAMPLE_PIPELINESTAT              = 30,
   SO_VGTSTREAMOUT_FLUSH            = 31,
   SAMPLE_STREAMOUTSTATS            = 32,
   RESET_VTX_CNT                    = 33,
   BLOCK_CONTEXT_DONE               = 34,
   VGT_FLUSH                        = 36,
   SQ_NON_EVENT                     = 38,
   SC_SEND_DB_VPZ                   = 39,
   BOTTOM_OF_PIPE_TS                = 40,
   FLUSH_SX_TS                      = 41,
   DB_CACHE_FLUSH_AND_INV           = 42,
   FLUSH_AND_INV_DB_DATA_TS         = 43,
   FLUSH_AND_INV_DB_META            = 44,
   FLUSH_AND_INV_CB_DATA_TS         = 45,
   FLUSH_AND_INV_CB_META            = 46,
};

enum VGT_GS_OUT_PRIMITIVE_TYPE : uint32_t
{
   VGT_GS_OUT_PT_POINTLIST          = 0,
   VGT_GS_OUT_PT_LINESTRIP          = 1,
   VGT_GS_OUT_PT_TRISTRIP           = 2,
};

enum VGT_INDEX : uint32_t
{
   VGT_INDEX_16                     = 0,
   VGT_INDEX_32                     = 1,
};

} // namespace latte
