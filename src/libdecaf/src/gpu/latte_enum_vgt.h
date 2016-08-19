#pragma once
#include "common/types.h"

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
   VGT_EVENT_TYPE_CACHE_FLUSH_TS                   = 4,
   VGT_EVENT_TYPE_CONTEXT_DONE                     = 5,
   VGT_EVENT_TYPE_CACHE_FLUSH                      = 6,
   VGT_EVENT_TYPE_VIZQUERY_START                   = 7,
   VGT_EVENT_TYPE_VIZQUERY_END                     = 8,
   VGT_EVENT_TYPE_SC_WAIT_WC                       = 9,
   VGT_EVENT_TYPE_MPASS_PS_CP_REFETCH              = 10,
   VGT_EVENT_TYPE_MPASS_PS_RST_START               = 11,
   VGT_EVENT_TYPE_MPASS_PS_INCR_START              = 12,
   VGT_EVENT_TYPE_RST_PIX_CNT                      = 13,
   VGT_EVENT_TYPE_RST_VTX_CNT                      = 14,
   VGT_EVENT_TYPE_VS_PARTIAL_FLUSH                 = 15,
   VGT_EVENT_TYPE_PS_PARTIAL_FLUSH                 = 16,
   VGT_EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT     = 20,
   VGT_EVENT_TYPE_ZPASS_DONE                       = 21,
   VGT_EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT        = 22,
   VGT_EVENT_TYPE_PERFCOUNTER_START                = 23,
   VGT_EVENT_TYPE_PERFCOUNTER_STOP                 = 24,
   VGT_EVENT_TYPE_PIPELINESTAT_START               = 25,
   VGT_EVENT_TYPE_PIPELINESTAT_STOP                = 26,
   VGT_EVENT_TYPE_PERFCOUNTER_SAMPLE               = 27,
   VGT_EVENT_TYPE_FLUSH_ES_OUTPUT                  = 28,
   VGT_EVENT_TYPE_FLUSH_GS_OUTPUT                  = 29,
   VGT_EVENT_TYPE_SAMPLE_PIPELINESTAT              = 30,
   VGT_EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH            = 31,
   VGT_EVENT_TYPE_SAMPLE_STREAMOUTSTATS            = 32,
   VGT_EVENT_TYPE_RESET_VTX_CNT                    = 33,
   VGT_EVENT_TYPE_BLOCK_CONTEXT_DONE               = 34,
   VGT_EVENT_TYPE_VGT_FLUSH                        = 36,
   VGT_EVENT_TYPE_SQ_NON_EVENT                     = 38,
   VGT_EVENT_TYPE_SC_SEND_DB_VPZ                   = 39,
   VGT_EVENT_TYPE_BOTTOM_OF_PIPE_TS                = 40,
   VGT_EVENT_TYPE_FLUSH_SX_TS                      = 41,
   VGT_EVENT_TYPE_DB_CACHE_FLUSH_AND_INV           = 42,
   VGT_EVENT_TYPE_FLUSH_AND_INV_DB_DATA_TS         = 43,
   VGT_EVENT_TYPE_FLUSH_AND_INV_DB_META            = 44,
   VGT_EVENT_TYPE_FLUSH_AND_INV_CB_DATA_TS         = 45,
   VGT_EVENT_TYPE_FLUSH_AND_INV_CB_META            = 46,
};

enum VGT_EVENT_INDEX : uint32_t
{
   VGT_EVENT_INDEX_GENERIC                = 0,
   VGT_EVENT_INDEX_ZPASS_DONE             = 1,
   VGT_EVENT_INDEX_SAMPLE_PIPELINESTAT    = 2,
   VGT_EVENT_INDEX_SAMPLE_STREAMOUTSTAT   = 3,
   VGT_EVENT_INDEX_PARTIAL_FLUSH          = 4,
   VGT_EVENT_INDEX_CACHE_FLUSH            = 7,
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

enum VGT_OUTPUT_PATH_SELECT : uint32_t
{
   VGT_OUTPATH_VTX_REUSE            = 0,
   VGT_OUTPATH_TESS_EN              = 1,
   VGT_OUTPATH_PASSTHRU             = 2,
   VGT_OUTPATH_GS_BLOCK             = 3,
};

} // namespace latte
