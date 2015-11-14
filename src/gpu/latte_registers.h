#pragma once
#include "types.h"

namespace latte
{

namespace Register
{

enum Value : uint32_t
{
   // Config Registers
   ConfigRegisterBase               = 0x08000,
   VGT_PRIMITIVE_TYPE               = 0x08958,
   VGT_NUM_INDICES                  = 0x08970,
   ConfigRegisterEnd                = 0x08b00,

   // Context Registers
   ContextRegisterBase              = 0x28000,
   DB_STENCIL_CLEAR                 = 0x28028,
   DB_DEPTH_CLEAR                   = 0x2802c,
   CB_COLOR0_BASE                   = 0x28040,
   CB_COLOR1_BASE                   = 0x28044,
   CB_COLOR2_BASE                   = 0x28048,
   CB_COLOR3_BASE                   = 0x2804C,
   CB_COLOR4_BASE                   = 0x28050,
   CB_COLOR5_BASE                   = 0x28054,
   CB_COLOR6_BASE                   = 0x28058,
   CB_COLOR7_BASE                   = 0x2805C,
   CB_COLOR0_SIZE                   = 0x28060,
   CB_COLOR1_SIZE                   = 0x28064,
   CB_COLOR2_SIZE                   = 0x28068,
   CB_COLOR3_SIZE                   = 0x2806C,
   CB_COLOR4_SIZE                   = 0x28070,
   CB_COLOR5_SIZE                   = 0x28074,
   CB_COLOR6_SIZE                   = 0x28078,
   CB_COLOR7_SIZE                   = 0x2807C,
   CB_COLOR0_VIEW                   = 0x28080,
   CB_COLOR1_VIEW                   = 0x28084,
   CB_COLOR2_VIEW                   = 0x28088,
   CB_COLOR3_VIEW                   = 0x2808C,
   CB_COLOR4_VIEW                   = 0x28090,
   CB_COLOR5_VIEW                   = 0x28094,
   CB_COLOR6_VIEW                   = 0x28098,
   CB_COLOR7_VIEW                   = 0x2809C,
   CB_COLOR0_INFO                   = 0x280A0,
   CB_COLOR1_INFO                   = 0x280A4,
   CB_COLOR2_INFO                   = 0x280A8,
   CB_COLOR3_INFO                   = 0x280AC,
   CB_COLOR4_INFO                   = 0x280B0,
   CB_COLOR5_INFO                   = 0x280B4,
   CB_COLOR6_INFO                   = 0x280B8,
   CB_COLOR7_INFO                   = 0x280BC,
   CB_COLOR0_TILE                   = 0x280C0,
   CB_COLOR1_TILE                   = 0x280C4,
   CB_COLOR2_TILE                   = 0x280C8,
   CB_COLOR3_TILE                   = 0x280CC,
   CB_COLOR4_TILE                   = 0x280D0,
   CB_COLOR5_TILE                   = 0x280D4,
   CB_COLOR6_TILE                   = 0x280D8,
   CB_COLOR7_TILE                   = 0x280DC,
   CB_COLOR0_FRAG                   = 0x280E0,
   CB_COLOR1_FRAG                   = 0x280E4,
   CB_COLOR2_FRAG                   = 0x280E8,
   CB_COLOR3_FRAG                   = 0x280EC,
   CB_COLOR4_FRAG                   = 0x280F0,
   CB_COLOR5_FRAG                   = 0x280F4,
   CB_COLOR6_FRAG                   = 0x280F8,
   CB_COLOR7_FRAG                   = 0x280FC,
   CB_COLOR0_MASK                   = 0x28100,
   CB_COLOR1_MASK                   = 0x28104,
   CB_COLOR2_MASK                   = 0x28108,
   CB_COLOR3_MASK                   = 0x2810C,
   CB_COLOR4_MASK                   = 0x28110,
   CB_COLOR5_MASK                   = 0x28114,
   CB_COLOR6_MASK                   = 0x28118,
   CB_COLOR7_MASK                   = 0x2811C,
   CB_SHADER_MASK                   = 0x2823C,
   SQ_VTX_SEMANTIC_0                = 0x28380,
   SQ_VTX_SEMANTIC_1                = 0x28384,
   SQ_VTX_SEMANTIC_2                = 0x28388,
   SQ_VTX_SEMANTIC_3                = 0x2838C,
   SQ_VTX_SEMANTIC_4                = 0x28390,
   SQ_VTX_SEMANTIC_5                = 0x28394,
   SQ_VTX_SEMANTIC_6                = 0x28398,
   SQ_VTX_SEMANTIC_7                = 0x2839C,
   SQ_VTX_SEMANTIC_8                = 0x283A0,
   SQ_VTX_SEMANTIC_9                = 0x283A4,
   SQ_VTX_SEMANTIC_10               = 0x283A8,
   SQ_VTX_SEMANTIC_11               = 0x283AC,
   SQ_VTX_SEMANTIC_12               = 0x283B0,
   SQ_VTX_SEMANTIC_13               = 0x283B4,
   SQ_VTX_SEMANTIC_14               = 0x283B8,
   SQ_VTX_SEMANTIC_15               = 0x283BC,
   SQ_VTX_SEMANTIC_16               = 0x283C0,
   SQ_VTX_SEMANTIC_17               = 0x283C4,
   SQ_VTX_SEMANTIC_18               = 0x283C8,
   SQ_VTX_SEMANTIC_19               = 0x283CC,
   SQ_VTX_SEMANTIC_20               = 0x283D0,
   SQ_VTX_SEMANTIC_21               = 0x283D4,
   SQ_VTX_SEMANTIC_22               = 0x283D8,
   SQ_VTX_SEMANTIC_23               = 0x283DC,
   SQ_VTX_SEMANTIC_24               = 0x283E0,
   SQ_VTX_SEMANTIC_25               = 0x283E4,
   SQ_VTX_SEMANTIC_26               = 0x283E8,
   SQ_VTX_SEMANTIC_27               = 0x283EC,
   SQ_VTX_SEMANTIC_28               = 0x283F0,
   SQ_VTX_SEMANTIC_29               = 0x283F4,
   SQ_VTX_SEMANTIC_30               = 0x283F8,
   SQ_VTX_SEMANTIC_31               = 0x283FC,
   VGT_MULTI_PRIM_IB_RESET_INDX     = 0x2840c,
   CB_BLEND_RED                     = 0x28414,
   CB_BLEND_GREEN                   = 0x28418,
   CB_BLEND_BLUE                    = 0x2841C,
   CB_BLEND_ALPHA                   = 0x28420,
   SPI_VS_OUT_ID_0                  = 0x28614,
   SPI_VS_OUT_ID_1                  = 0x28618,
   SPI_VS_OUT_ID_2                  = 0x2861C,
   SPI_VS_OUT_ID_3                  = 0x28620,
   SPI_VS_OUT_ID_4                  = 0x28624,
   SPI_VS_OUT_ID_5                  = 0x28628,
   SPI_VS_OUT_ID_6                  = 0x2862C,
   SPI_VS_OUT_ID_7                  = 0x28630,
   SPI_VS_OUT_ID_8                  = 0x28634,
   SPI_VS_OUT_ID_9                  = 0x28638,
   SPI_PS_INPUT_CNTL_0              = 0x28640,
   SPI_PS_INPUT_CNTL_1              = 0x28644,
   SPI_PS_INPUT_CNTL_2              = 0x28648,
   SPI_PS_INPUT_CNTL_3              = 0x2864C,
   SPI_PS_INPUT_CNTL_4              = 0x28650,
   SPI_PS_INPUT_CNTL_5              = 0x28654,
   SPI_PS_INPUT_CNTL_6              = 0x28658,
   SPI_PS_INPUT_CNTL_7              = 0x2865C,
   SPI_PS_INPUT_CNTL_8              = 0x28660,
   SPI_PS_INPUT_CNTL_9              = 0x28664,
   SPI_PS_INPUT_CNTL_10             = 0x28668,
   SPI_PS_INPUT_CNTL_11             = 0x2866C,
   SPI_PS_INPUT_CNTL_12             = 0x28670,
   SPI_PS_INPUT_CNTL_13             = 0x28674,
   SPI_PS_INPUT_CNTL_14             = 0x28678,
   SPI_PS_INPUT_CNTL_15             = 0x2867C,
   SPI_PS_INPUT_CNTL_16             = 0x28680,
   SPI_PS_INPUT_CNTL_17             = 0x28684,
   SPI_PS_INPUT_CNTL_18             = 0x28688,
   SPI_PS_INPUT_CNTL_19             = 0x2868C,
   SPI_PS_INPUT_CNTL_20             = 0x28690,
   SPI_PS_INPUT_CNTL_21             = 0x28694,
   SPI_PS_INPUT_CNTL_22             = 0x28698,
   SPI_PS_INPUT_CNTL_23             = 0x2869C,
   SPI_PS_INPUT_CNTL_24             = 0x286A0,
   SPI_PS_INPUT_CNTL_25             = 0x286A4,
   SPI_PS_INPUT_CNTL_26             = 0x286A8,
   SPI_PS_INPUT_CNTL_27             = 0x286AC,
   SPI_PS_INPUT_CNTL_28             = 0x286B0,
   SPI_PS_INPUT_CNTL_29             = 0x286B4,
   SPI_PS_INPUT_CNTL_30             = 0x286B8,
   SPI_PS_INPUT_CNTL_31             = 0x286BC,
   SPI_VS_OUT_CONFIG                = 0x286C4,
   SPI_PS_IN_CONTROL_0              = 0x286CC,
   SPI_PS_IN_CONTROL_1              = 0x286D0,
   SPI_INPUT_Z                      = 0x286D8,
   CB_BLEND0_CONTROL                = 0x28780,
   CB_BLEND1_CONTROL                = 0x28784,
   CB_BLEND2_CONTROL                = 0x28788,
   CB_BLEND3_CONTROL                = 0x2878C,
   CB_BLEND4_CONTROL                = 0x28790,
   CB_BLEND5_CONTROL                = 0x28794,
   CB_BLEND6_CONTROL                = 0x28798,
   CB_BLEND7_CONTROL                = 0x2879C,
   CB_SHADER_CONTROL                = 0x287A0,
   VGT_DRAW_INITIATOR               = 0x287F0,
   VGT_DMA_BASE_HI                  = 0x287E4,
   VGT_DMA_BASE                     = 0x287E8,
   DB_DEPTH_CONTROL                 = 0x28800,
   CB_BLEND_CONTROL                 = 0x28804,
   CB_COLOR_CONTROL                 = 0x28808,
   DB_SHADER_CONTROL                = 0x2880C,
   PA_CL_VS_OUT_CNTL                = 0x2881C,
   SQ_PGM_RESOURCES_PS              = 0x28850,
   SQ_PGM_EXPORTS_PS                = 0x28854,
   SQ_PGM_START_VS                  = 0x28858,
   SQ_PGM_RESOURCES_VS              = 0x28868,
   SQ_PGM_RESOURCES_FS              = 0x288A4,
   SQ_ESGS_RING_ITEMSIZE            = 0x288A8,
   SQ_PGM_CF_OFFSET_VS              = 0x288D0,
   SQ_VTX_SEMANTIC_CLEAR            = 0x288E0,
   VGT_HOS_REUSE_DEPTH              = 0x28A20,
   VGT_DMA_SIZE                     = 0x28A74,
   VGT_DMA_MAX_SIZE                 = 0x28A78,
   VGT_DMA_INDEX_TYPE               = 0x28A7c,
   VGT_PRIMITIVEID_EN               = 0x28A84,
   VGT_DMA_NUM_INSTANCES            = 0x28A88,
   VGT_MULTI_PRIM_IB_RESET_EN       = 0x28A94,
   VGT_STRMOUT_BUFFER_EN            = 0x28B20,
   VGT_VERTEX_REUSE_BLOCK_CNTL      = 0x28C58,
   ContextRegisterEnd               = 0x29000,

   // Alu Const Registers
   AluConstRegisterBase             = 0x30000,

   // Resource Registers
   ResourceRegisterBase             = 0x38000,
   SQ_TEX_RESOURCE_WORD0_0          = 0x38000,
   SQ_TEX_RESOURCE_WORD1_0          = 0x38004,
   SQ_TEX_RESOURCE_WORD2_0          = 0x38008,
   SQ_TEX_RESOURCE_WORD3_0          = 0x3800C,
   SQ_TEX_RESOURCE_WORD4_0          = 0x38010,
   SQ_TEX_RESOURCE_WORD5_0          = 0x38014,
   SQ_TEX_RESOURCE_WORD6_0          = 0x38018,

   // Sampler Registers
   SamplerRegisterBase              = 0x3C000,

   // Control Registers
   ControlRegisterBase              = 0x3CFF0,
   SQ_VTX_BASE_VTX_LOC              = 0x3CFF0,

   // Loop Const Registers
   LoopConstRegisterBase            = 0x3E200,

   // Bool Const Registers
   BoolConstRegisterBase            = 0x3E380,
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

// Blend function used for all render targets
union CB_BLENDN_CONTROL
{
   uint32_t value;

   struct
   {
      BLEND_FUNC COLOR_SRCBLEND : 5;
      COMB_FUNC COLOR_COMB_FCN : 3;
      BLEND_FUNC COLOR_DESTBLEND : 5;
      uint32_t OPACITY_WEIGHT : 1;
      uint32_t : 2;
      BLEND_FUNC ALPHA_SRCBLEND : 5;
      COMB_FUNC ALPHA_COMB_FCN : 3;
      BLEND_FUNC ALPHA_DESTBLEND : 5;
      uint32_t SEPARATE_ALPHA_BLEND : 1;
      uint32_t : 2;
   };
};

// Blend function used for individual render targets
using CB_BLEND0_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND1_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND2_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND3_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND4_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND5_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND6_CONTROL = CB_BLENDN_CONTROL;
using CB_BLEND7_CONTROL = CB_BLENDN_CONTROL;

struct CB_BLEND_RED
{
   float BLEND_RED;
};

struct CB_BLEND_GREEN
{
   float BLEND_GREEN;
};

struct CB_BLEND_BLUE
{
   float BLEND_BLUE;
};

struct CB_BLEND_ALPHA
{
   float BLEND_ALPHA;
};

struct CB_COLORN_BASE
{
   uint32_t BASE_256B;
};

using CB_COLOR0_BASE = CB_COLORN_BASE;
using CB_COLOR1_BASE = CB_COLORN_BASE;
using CB_COLOR2_BASE = CB_COLORN_BASE;
using CB_COLOR3_BASE = CB_COLORN_BASE;
using CB_COLOR4_BASE = CB_COLORN_BASE;
using CB_COLOR5_BASE = CB_COLORN_BASE;
using CB_COLOR6_BASE = CB_COLORN_BASE;
using CB_COLOR7_BASE = CB_COLORN_BASE;

struct CB_COLORN_TILE
{
   uint32_t BASE_256B;
};

using CB_COLOR0_TILE = CB_COLORN_TILE;
using CB_COLOR1_TILE = CB_COLORN_TILE;
using CB_COLOR2_TILE = CB_COLORN_TILE;
using CB_COLOR3_TILE = CB_COLORN_TILE;
using CB_COLOR4_TILE = CB_COLORN_TILE;
using CB_COLOR5_TILE = CB_COLORN_TILE;
using CB_COLOR6_TILE = CB_COLORN_TILE;
using CB_COLOR7_TILE = CB_COLORN_TILE;

struct CB_COLORN_FRAG
{
   uint32_t addr256B;
};

using CB_COLOR0_FRAG = CB_COLORN_FRAG;
using CB_COLOR1_FRAG = CB_COLORN_FRAG;
using CB_COLOR2_FRAG = CB_COLORN_FRAG;
using CB_COLOR3_FRAG = CB_COLORN_FRAG;
using CB_COLOR4_FRAG = CB_COLORN_FRAG;
using CB_COLOR5_FRAG = CB_COLORN_FRAG;
using CB_COLOR6_FRAG = CB_COLORN_FRAG;
using CB_COLOR7_FRAG = CB_COLORN_FRAG;

union CB_COLORN_SIZE
{
   uint32_t value;

   struct
   {
      uint32_t PITCH_TILE_MAX : 10;
      uint32_t SLICE_TILE_MAX : 20;
      uint32_t : 2;
   };
};

using CB_COLOR0_SIZE = CB_COLORN_SIZE;
using CB_COLOR1_SIZE = CB_COLORN_SIZE;
using CB_COLOR2_SIZE = CB_COLORN_SIZE;
using CB_COLOR3_SIZE = CB_COLORN_SIZE;
using CB_COLOR4_SIZE = CB_COLORN_SIZE;
using CB_COLOR5_SIZE = CB_COLORN_SIZE;
using CB_COLOR6_SIZE = CB_COLORN_SIZE;
using CB_COLOR7_SIZE = CB_COLORN_SIZE;

enum CB_ENDIAN : uint32_t
{
   ENDIAN_NONE                = 0,
   ENDIAN_8IN16               = 1,
   ENDIAN_8IN32               = 2,
   ENDIAN_8IN64               = 3,
};

enum CB_FORMAT : uint32_t
{
   COLOR_INVALID              = 0,
   COLOR_8                    = 1,
   COLOR_4_4                  = 2,
   COLOR_3_3_2                = 3,
   COLOR_16                   = 5,
   COLOR_16_FLOAT             = 6,
   COLOR_8_8                  = 7,
   COLOR_5_6_5                = 8,
   COLOR_6_5_5                = 9,
   COLOR_1_5_5_5              = 10,
   COLOR_4_4_4_4              = 11,
   COLOR_5_5_5_1              = 12,
   COLOR_32                   = 13,
   COLOR_32_FLOAT             = 14,
   COLOR_16_16                = 15,
   COLOR_16_16_FLOAT          = 16,
   COLOR_8_24                 = 17,
   COLOR_8_24_FLOAT           = 18,
   COLOR_24_8                 = 19,
   COLOR_24_8_FLOAT           = 20,
   COLOR_10_11_11             = 21,
   COLOR_10_11_11_FLOAT       = 22,
   COLOR_11_11_10             = 23,
   COLOR_11_11_10_FLOAT       = 24,
   COLOR_2_10_10_10           = 25,
   COLOR_8_8_8_8              = 26,
   COLOR_10_10_10_2           = 27,
   COLOR_X24_8_32_FLOAT       = 28,
   COLOR_32_32                = 29,
   COLOR_32_32_FLOAT          = 30,
   COLOR_16_16_16_16          = 31,
   COLOR_16_16_16_16_FLOAT    = 32,
   COLOR_32_32_32_32          = 34,
   COLOR_32_32_32_32_FLOAT    = 35,
};

enum CB_ARRAY_MODE : uint32_t
{
   ARRAY_LINEAR_GENERAL       = 0,
   ARRAY_LINEAR_ALIGNED       = 1,
   ARRAY_2D_TILED_THIN1       = 4,
};

enum CB_NUMBER_TYPE : uint32_t
{
   NUMBER_UNORM               = 0,
   NUMBER_SNORM               = 1,
   NUMBER_USCALED             = 2,
   NUMBER_SSCALED             = 3,
   NUMBER_UINT                = 4,
   NUMBER_SINT                = 5,
   NUMBER_SRGB                = 6,
   NUMBER_FLOAT               = 7,
};

enum CB_READ_SIZE : uint32_t
{
   READ_256_BITS              = 0,
   READ_512_BITS              = 0,
};

enum CB_COMP_SWAP : uint32_t
{
   SWAP_STD                   = 0,
   SWAP_ALT                   = 1,
   SWAP_STD_REV               = 2,
   SWAP_ALT_REV               = 3,
};

enum CB_TILE_MODE : uint32_t
{
   TILE_DISABLE               = 0,
   TILE_CLEAR_ENABLE          = 1,
   TILE_FRAG_ENABLE           = 2,
};

enum CB_ROUND_MODE : uint32_t
{
   ROUND_BY_HALF              = 0,
   ROUND_TRUNCATE             = 1,
};

enum CB_SOURCE_FORMAT
{
   EXPORT_FULL                = 0,
   EXPORT_NORM                = 1,
};

union CB_COLORN_INFO
{
   uint32_t value;

   struct
   {
      CB_ENDIAN ENDIAN : 2;
      CB_FORMAT FORMAT : 6;
      CB_ARRAY_MODE ARRAY_MODE : 4;
      CB_NUMBER_TYPE NUMBER_TYPE : 3;
      CB_READ_SIZE READ_SIZE : 1;
      CB_COMP_SWAP COMP_SWAP : 2;
      CB_TILE_MODE TILE_MODE : 2;
      uint32_t BLEND_CLAMP : 1;
      uint32_t CLEAR_COLOR : 1;
      uint32_t BLEND_BYPASS : 1;
      uint32_t BLEND_FLOAT32 : 1;
      uint32_t SIMPLE_FLOAT : 1;
      CB_ROUND_MODE ROUND_MODE : 1;
      uint32_t TILE_COMPACT : 1;
      CB_SOURCE_FORMAT SOURCE_FORMAT : 1;
      uint32_t : 4;
   };
};

using CB_COLOR0_INFO = CB_COLORN_INFO;
using CB_COLOR1_INFO = CB_COLORN_INFO;
using CB_COLOR2_INFO = CB_COLORN_INFO;
using CB_COLOR3_INFO = CB_COLORN_INFO;
using CB_COLOR4_INFO = CB_COLORN_INFO;
using CB_COLOR5_INFO = CB_COLORN_INFO;
using CB_COLOR6_INFO = CB_COLORN_INFO;
using CB_COLOR7_INFO = CB_COLORN_INFO;

// Selects slice index range for render target
union CB_COLORN_VIEW
{
   uint32_t value;

   struct
   {
      uint32_t SLICE_START : 11;
      uint32_t SLICE_MAX : 11;
      uint32_t : 10;
   };
};

using CB_COLOR0_VIEW = CB_COLORN_VIEW;
using CB_COLOR1_VIEW = CB_COLORN_VIEW;
using CB_COLOR2_VIEW = CB_COLORN_VIEW;
using CB_COLOR3_VIEW = CB_COLORN_VIEW;
using CB_COLOR4_VIEW = CB_COLORN_VIEW;
using CB_COLOR5_VIEW = CB_COLORN_VIEW;
using CB_COLOR6_VIEW = CB_COLORN_VIEW;
using CB_COLOR7_VIEW = CB_COLORN_VIEW;

union CB_COLORN_MASK
{
   uint32_t value;

   struct
   {
      uint32_t CMASK_BLOCK_MASK : 12;
      uint32_t FMASK_TILE_MASK : 20;
   };
};

using CB_COLOR0_MASK = CB_COLORN_MASK;
using CB_COLOR1_MASK = CB_COLORN_MASK;
using CB_COLOR2_MASK = CB_COLORN_MASK;
using CB_COLOR3_MASK = CB_COLORN_MASK;
using CB_COLOR4_MASK = CB_COLORN_MASK;
using CB_COLOR5_MASK = CB_COLORN_MASK;
using CB_COLOR6_MASK = CB_COLORN_MASK;
using CB_COLOR7_MASK = CB_COLORN_MASK;

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
   uint32_t value;

   struct
   {
      uint32_t FOG_ENABLE : 1;
      uint32_t MULTIWRITE_ENABLE : 1;
      uint32_t DITHER_ENABLE : 1;
      uint32_t DEGAMMA_ENABLE : 1;
      uint32_t SPECIAL_OP : 3;
      uint32_t PER_MRT_BLEND : 1;
      uint32_t TARGET_BLEND_ENABLE : 8;
      uint32_t ROP3 : 8;
      uint32_t : 8;
   };
};

// Contains color component mask fields for the colors output by the shader
union CB_SHADER_MASK
{
   uint32_t value;

   struct
   {
      uint32_t OUTPUT0_ENABLE : 4;
      uint32_t OUTPUT1_ENABLE : 4;
      uint32_t OUTPUT2_ENABLE : 4;
      uint32_t OUTPUT3_ENABLE : 4;
      uint32_t OUTPUT4_ENABLE : 4;
      uint32_t OUTPUT5_ENABLE : 4;
      uint32_t OUTPUT6_ENABLE : 4;
      uint32_t OUTPUT7_ENABLE : 4;
   };
};

union CB_SHADER_CONTROL
{
   uint32_t value;

   struct
   {
      uint32_t RT0_ENABLE : 1;
      uint32_t RT1_ENABLE : 1;
      uint32_t RT2_ENABLE : 1;
      uint32_t RT3_ENABLE : 1;
      uint32_t RT4_ENABLE : 1;
      uint32_t RT5_ENABLE : 1;
      uint32_t RT6_ENABLE : 1;
      uint32_t RT7_ENABLE : 1;
      uint32_t : 24;
   };
};

union DB_STENCIL_CLEAR
{
   uint32_t value;

   struct
   {
      uint32_t CLEAR : 8;
      uint32_t MIN : 16;
      uint32_t : 8;
   };
};

struct DB_DEPTH_CLEAR
{
   float DEPTH_CLEAR;
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

// This register controls depth and stencil tests.
union DB_DEPTH_CONTROL
{
   uint32_t value;

   struct
   {
      uint32_t STENCIL_ENABLE : 1;
      uint32_t Z_ENABLE : 1;
      uint32_t Z_WRITE_ENABLE : 1;
      uint32_t : 1;
      FRAG_FUNC ZFUNC : 3;
      uint32_t BACKFACE_ENABLE : 1;
      REF_FUNC STENCILFUNC : 3;
      STENCIL_FUNC STENCILFAIL : 3;
      STENCIL_FUNC STENCILZPASS : 3;
      STENCIL_FUNC STENCILZFAIL : 3;
      REF_FUNC STENCILFUNC_BF : 3;
      STENCIL_FUNC STENCILFAIL_BF : 3;
      STENCIL_FUNC STENCILZPASS_BF : 3;
      STENCIL_FUNC STENCILZFAIL_BF : 3;
   };
};

enum DB_Z_ORDER
{
   LATE_Z               = 0,
   EARLY_Z_THEN_LATE_Z  = 1,
   RE_Z                 = 2,
   EARLY_Z_THEN_RE_Z    = 3,
};

union DB_SHADER_CONTROL
{
   uint32_t value;

   struct
   {
      uint32_t Z_EXPORT_ENABLE: 1;
      uint32_t STENCIL_REF_EXPORT_ENABLE: 1;
      uint32_t : 2;
      DB_Z_ORDER Z_ORDER : 2;
      uint32_t KILL_ENABLE : 1;
      uint32_t COVERAGE_TO_MASK_ENABLE : 1;
      uint32_t MASK_EXPORT_ENABLE : 1;
      uint32_t DUAL_EXPORT_ENABLE : 1;
      uint32_t EXEC_ON_HIER_FAIL : 1;
      uint32_t EXEC_ON_NOOP : 1;
      uint32_t ALPHA_TO_MASK_DISABLE : 1;
      uint32_t : 19;
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

// VGT DMA Size
struct VGT_DMA_SIZE
{
   uint32_t NUM_INDICES;
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

// Draw Inititiator
union VGT_DRAW_INITIATOR
{
   uint32_t value;

   struct
   {
      DI_SRC_SEL SOURCE_SELECT : 2;
      DI_MAJOR_MODE MAJOR_MODE : 2;
      uint32_t SPRITE_EN_R6XX : 1;
      uint32_t NOT_EOP : 1;
      uint32_t USE_OPAQUE : 1;
      uint32_t : 25;
   };
};

// VGT Number of Indices
struct VGT_NUM_INDICES
{
   uint32_t NUM_INDICES;
};

enum DI_PRIMITIVE_TYPE
{
   DI_PT_NONE = 0,
   DI_PT_POINTLIST = 1,
   DI_PT_LINELIST = 2,
   DI_PT_LINESTRIP = 3,
   DI_PT_TRILIST = 4,
   DI_PT_TRIFAN = 5,
   DI_PT_TRISTRIP = 6,
   DI_PT_UNUSED_0 = 7,
   DI_PT_UNUSED_1 = 8,
   DI_PT_UNUSED_2 = 9,
   DI_PT_LINELIST_ADJ = 10,
   DI_PT_LINESTRIP_ADJ = 11,
   DI_PT_TRILIST_ADJ = 12,
   DI_PT_TRISTRIP_ADJ = 13,
   DI_PT_UNUSED_3 = 14,
   DI_PT_UNUSED_4 = 15,
   DI_PT_TRI_WITH_WFLAGS = 16,
   DI_PT_RECTLIST = 17,
   DI_PT_LINELOOP = 18,
   DI_PT_QUADLIST = 19,
   DI_PT_QUADSTRIP = 20,
   DI_PT_POLYGON = 21,
   DI_PT_2D_COPY_RECT_LIST_V0 = 22,
   DI_PT_2D_COPY_RECT_LIST_V1 = 23,
   DI_PT_2D_COPY_RECT_LIST_V2 = 24,
   DI_PT_2D_COPY_RECT_LIST_V3 = 25,
   DI_PT_2D_FILL_RECT_LIST = 26,
   DI_PT_2D_LINE_STRIP = 27,
   DI_PT_2D_TRI_STRIP = 28,
};

// VGT Primitive Type
union VGT_PRIMITIVE_TYPE
{
   uint32_t value;

   struct
   {
      DI_PRIMITIVE_TYPE PRIM_TYPE : 6;
      uint32_t : 26;
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
      uint32_t BASE_ADDRESS_HI : 8;
      uint32_t STRIDE : 11;
      SQ_VTX_CLAMP CLAMP_X : 1;
      uint32_t DATA_FORMAT : 6;
      SQ_NUM_FORMAT NUM_FORMAT_ALL : 2;
      SQ_FORMAT_COMP FORMAT_COMP_ALL : 1;
      SQ_SRF_MODE SRF_MODE_ALL : 1;
      SQ_ENDIAN ENDIAN_SWAP : 2;
   };
};

union SQ_VTX_CONSTANT_WORD3_0
{
   uint32_t value;

   struct
   {
      uint32_t MEM_REQUEST_SIZE : 2;
      uint32_t UNCACHED : 1;
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
      SQ_TEX_VTX_TYPE TYPE : 2;
   };
};

// Vertex fetch base location
struct SQ_VTX_BASE_VTX_LOC
{
   uint32_t OFFSET;
};

// Resource requirements to run the Vertex Shader program
union SQ_PGM_RESOURCES_VS
{
   uint32_t value;

   struct
   {
      uint32_t NUM_GPRS : 8;
      uint32_t STACK_SIZE : 8;
      uint32_t DX10_CLAMP : 1;
      uint32_t PRIME_CACHE_PGM_EN : 1;
      uint32_t PRIME_CACHE_ON_DRAW : 1;
      uint32_t FETCH_CACHE_LINES : 3;
      uint32_t UNCACHED_FIRST_INST : 1;
      uint32_t PRIME_CACHE_ENABLE : 1;
      uint32_t PRIME_CACHE_ON_CONST : 1;
      uint32_t : 1;
   };
};

// Resource requirements to run the Pixel Shader program
union SQ_PGM_RESOURCES_PS
{
   uint32_t value;

   struct
   {
      uint32_t NUM_GPRS : 8;
      uint32_t STACK_SIZE : 8;
      uint32_t DX10_CLAMP : 1;
      uint32_t PRIME_CACHE_PGM_EN : 1;
      uint32_t PRIME_CACHE_ON_DRAW : 1;
      uint32_t FETCH_CACHE_LINES : 3;
      uint32_t UNCACHED_FIRST_INST : 1;
      uint32_t PRIME_CACHE_ENABLE : 1;
      uint32_t PRIME_CACHE_ON_CONST : 1;
      uint32_t CLAMP_CONSTS : 1;
   };
};

// Resource requirements to run the Fetch Shader program
union SQ_PGM_RESOURCES_FS
{
   uint32_t value;

   struct
   {
      uint32_t NUM_GPRS : 8;
      uint32_t STACK_SIZE : 8;
      uint32_t : 5;
      uint32_t DX10_CLAMP : 1;
      uint32_t : 10;
   };
};

// Defines the exports from the Pixel Shader Program.
union SQ_PGM_EXPORTS_PS
{
   uint32_t value;

   struct
   {
      uint32_t EXPORT_MODE : 5;
      uint32_t : 27;
   };
};

// This register is used to clear the contents of the vertex semantic table.
// Entries can be cleared independently -- each has one bit in this register to clear or leave alone.
union SQ_VTX_SEMANTIC_CLEAR
{
   uint32_t value;

   uint32_t CLEAR;
};

union SQ_VTX_SEMANTIC_N
{
   uint32_t value;

   uint32_t SEMANTIC_ID;
};

using SQ_VTX_SEMANTIC_0 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_1 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_2 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_3 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_4 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_5 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_6 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_7 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_8 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_9 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_10 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_11 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_12 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_13 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_14 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_15 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_16 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_17 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_18 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_19 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_20 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_21 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_22 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_23 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_24 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_25 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_26 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_27 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_28 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_29 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_30 = SQ_VTX_SEMANTIC_N;
using SQ_VTX_SEMANTIC_31 = SQ_VTX_SEMANTIC_N;

enum SQ_TEX_DIM : uint32_t
{
   SQ_TEX_DIM_1D              = 0x0,
   SQ_TEX_DIM_2D              = 0x1,
   SQ_TEX_DIM_3D              = 0x2,
   SQ_TEX_DIM_CUBEMAP         = 0x3,
   SQ_TEX_DIM_1D_ARRAY        = 0x4,
   SQ_TEX_DIM_2D_ARRAY        = 0x5,
   SQ_TEX_DIM_2D_MSAA         = 0x6,
   SQ_TEX_DIM_2D_ARRAY_MSAA   = 0x7,
};

union SQ_TEX_RESOURCE_WORD0_0
{
   uint32_t value;

   struct
   {
      SQ_TEX_DIM DIM : 3;
      uint32_t TILE_MODE : 4;
      uint32_t TILE_TYPE : 1;
      uint32_t PITCH : 11;
      uint32_t TEX_WIDTH : 13;
   };
};

union SQ_TEX_RESOURCE_WORD1_0
{
   uint32_t value;

   struct
   {
      uint32_t TEX_HEIGHT : 13;
      uint32_t TEX_DEPTH : 13;
      uint32_t DATA_FORMAT : 6;
   };
};

struct SQ_TEX_RESOURCE_WORD2_0
{
   uint32_t BASE_ADDRESS;
};

struct SQ_TEX_RESOURCE_WORD3_0
{
   uint32_t MIP_ADDRESS;
};

enum SQ_SEL : uint32_t
{
   SQ_SEL_X                         = 0,
   SQ_SEL_Y                         = 1,
   SQ_SEL_Z                         = 2,
   SQ_SEL_W                         = 3,
   SQ_SEL_0                         = 4,
   SQ_SEL_1                         = 5,
};

union SQ_TEX_RESOURCE_WORD4_0
{
   uint32_t value;

   struct
   {
      SQ_FORMAT_COMP FORMAT_COMP_X : 2;
      SQ_FORMAT_COMP FORMAT_COMP_Y : 2;
      SQ_FORMAT_COMP FORMAT_COMP_Z : 2;
      SQ_FORMAT_COMP FORMAT_COMP_W : 2;
      SQ_NUM_FORMAT NUM_FORMAT_ALL : 2;
      SQ_SRF_MODE SRF_MODE_ALL : 1;
      uint32_t FORCE_DEGAMMA : 1;
      SQ_ENDIAN ENDIAN_SWAP : 2;
      uint32_t REQUEST_SIZE : 2;
      SQ_SEL DST_SEL_X : 3;
      SQ_SEL DST_SEL_Y : 3;
      SQ_SEL DST_SEL_Z : 3;
      SQ_SEL DST_SEL_W : 3;
      uint32_t BASE_LEVEL : 4;
   };
};

union SQ_TEX_RESOURCE_WORD5_0
{
   uint32_t value;

   struct
   {
      uint32_t LAST_LEVEL : 4;
      uint32_t BASE_ARRAY : 13;
      uint32_t LAST_ARRAY : 13;
      uint32_t YUV_CONV : 2;
   };
};

enum SQ_TEX_MPEG_CLAMP : uint32_t
{
   SQ_TEX_MPEG_CLAMP_OFF   = 0,
   SQ_TEX_MPEG_9           = 1,
   SQ_TEX_MPEG_10          = 2,
};

union SQ_TEX_RESOURCE_WORD6_0
{
   uint32_t value;

   struct
   {
      SQ_TEX_MPEG_CLAMP MPEG_CLAMP : 2;
      uint32_t MAX_ANISO_RATIO : 3;
      uint32_t PERF_MODULATION : 3;
      uint32_t INTERLACED : 1;
      uint32_t ADVIS_FAULT_LOD : 4;
      uint32_t ADVIS_CLAMP_LOD : 6;
      uint32_t : 11;
      SQ_TEX_VTX_TYPE TYPE : 2;
   };
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

union VGT_HOS_REUSE_DEPTH
{
   uint32_t value;

   struct
   {
      uint32_t REUSE_DEPTH : 8;
      uint32_t : 24;
   };
};

// Vertex Shader output configuration
union SPI_VS_OUT_CONFIG
{
   uint32_t value;

   struct
   {
      uint32_t VS_PER_COMPONENT : 1;
      uint32_t VS_EXPORT_COUNT : 5;
      uint32_t : 2;
      uint32_t VS_EXPORTS_FOG : 1;
      uint32_t VS_OUT_FOG_VEC_ADDR : 5;
      uint32_t : 18;
   };
};

// Vertex Shader output semantic mapping
union SPI_VS_OUT_ID
{
   uint32_t value;

   struct
   {
      uint32_t SEMANTIC_0 : 8;
      uint32_t SEMANTIC_1 : 8;
      uint32_t SEMANTIC_2 : 8;
      uint32_t SEMANTIC_3 : 8;
   };
};

using SPI_VS_OUT_ID_0 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_1 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_2 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_3 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_4 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_5 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_6 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_7 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_8 = SPI_VS_OUT_ID;
using SPI_VS_OUT_ID_9 = SPI_VS_OUT_ID;

enum BARYC_CNTL
{
   CENTROIDS_ONLY          = 0,
   CENTERS_ONLY            = 1,
   CENTROIDS_AND_CENTERS   = 2,
};

// Interpolator control settings
union SPI_PS_IN_CONTROL_0
{
   uint32_t value;

   struct
   {
      uint32_t NUM_INTERP : 6;
      uint32_t POSITION_ENA : 1;
      uint32_t POSITION_CENTROID : 1;
      uint32_t POSITION_ADDR : 5;
      uint32_t PARAM_GEN : 4;
      uint32_t PARAM_GEN_ADDR : 7;
      uint32_t BARYC_SAMPLE_CNTL : 2;
      uint32_t PERSP_GRADIENT_ENA : 1;
      uint32_t LINEAR_GRADIENT_ENA : 1;
      uint32_t POSITION_SAMPLE : 1;
      uint32_t BARYC_AT_SAMPLE_ENA : 1;
   };
};

// Interpolator control settings
union SPI_PS_IN_CONTROL_1
{
   uint32_t value;

   struct
   {
      uint32_t GEN_INDEX_PIX : 1;
      uint32_t GEN_INDEX_PIX_ADDR : 7;
      uint32_t FRONT_FACE_ENA : 1;
      uint32_t FRONT_FACE_CHAN : 2;
      uint32_t FRONT_FACE_ALL_BITS : 1;
      uint32_t FRONT_FACE_ADDR : 5;
      uint32_t FOG_ADDR : 7;
      uint32_t FIXED_PT_POSITION_ENA : 1;
      uint32_t FIXED_PT_POSITION_ADDR : 5;
      uint32_t POSITION_ULC : 1;
   };
};

// PS interpolator setttings for parameter N
union SPI_PS_INPUT_CNTL_N
{
   uint32_t value;

   struct
   {
      uint32_t SEMANTIC : 8;
      uint32_t DEFAULT_VAL : 2;
      uint32_t FLAT_SHADE : 1;
      uint32_t SEL_CENTROID : 1;
      uint32_t SEL_LINEAR : 1;
      uint32_t CYL_WRAP : 4;
      uint32_t PT_SPRITE_TEX : 1;
      uint32_t SEL_SAMPLE : 1;
      uint32_t : 13;
   };
};

using SPI_PS_INPUT_CNTL_0 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_1 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_2 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_3 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_4 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_5 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_6 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_7 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_8 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_9 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_10 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_11 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_12 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_13 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_14 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_15 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_16 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_17 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_18 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_19 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_20 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_21 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_22 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_23 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_24 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_25 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_26 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_27 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_28 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_29 = SPI_PS_INPUT_CNTL_N;
using SPI_PS_INPUT_CNTL_30 = SPI_PS_INPUT_CNTL_N;

union SPI_INPUT_Z
{
   uint32_t value;

   struct
   {
      uint32_t PROVIDE_Z_TO_SPI : 1;
      uint32_t : 31;
   };
};

// Vertex Shader Output Control
union PA_CL_VS_OUT_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t CLIP_DIST_ENA_0 : 1;
      uint32_t CLIP_DIST_ENA_1 : 1;
      uint32_t CLIP_DIST_ENA_2 : 1;
      uint32_t CLIP_DIST_ENA_3 : 1;
      uint32_t CLIP_DIST_ENA_4 : 1;
      uint32_t CLIP_DIST_ENA_5 : 1;
      uint32_t CLIP_DIST_ENA_6 : 1;
      uint32_t CLIP_DIST_ENA_7 : 1;
      uint32_t CULL_DIST_ENA_0 : 1;
      uint32_t CULL_DIST_ENA_1 : 1;
      uint32_t CULL_DIST_ENA_2 : 1;
      uint32_t CULL_DIST_ENA_3 : 1;
      uint32_t CULL_DIST_ENA_4 : 1;
      uint32_t CULL_DIST_ENA_5 : 1;
      uint32_t CULL_DIST_ENA_6 : 1;
      uint32_t CULL_DIST_ENA_7 : 1;
      uint32_t USE_VTX_POINT_SIZE : 1;
      uint32_t USE_VTX_EDGE_FLAG : 1;
      uint32_t USE_VTX_RENDER_TARGET_INDX : 1;
      uint32_t USE_VTX_VIEWPORT_INDX : 1;
      uint32_t USE_VTX_KILL_FLAG : 1;
      uint32_t VS_OUT_MISC_VEC_ENA : 1;
      uint32_t VS_OUT_CCDIST0_VEC_ENA : 1;
      uint32_t VS_OUT_CCDIST1_VEC_ENA : 1;
      uint32_t VS_OUT_MISC_SIDE_BUS_ENA : 1;
      uint32_t USE_VTX_GS_CUT_FLAG : 1;
   };
};

} // namespace latte
