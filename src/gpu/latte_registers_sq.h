#pragma once
#include "types.h"
#include "latte_enum_sq.h"

namespace latte
{

// ALU Constant store data for use in DX9 mode (DX10 mode uses the constant-cache
// instead and this constant-file is not available).
// Constants 0-225 are reserved for pixel shader
// Constants 256-511 are reserved for vertex shader
struct SQ_ALU_CONSTANT0_0
{
   float X;
};

struct SQ_ALU_CONSTANT1_0
{
   float Y;
};

struct SQ_ALU_CONSTANT2_0
{
   float Z;
};

struct SQ_ALU_CONSTANT3_0
{
   float W;
};

union SQ_CONFIG
{
   uint32_t value;

   struct {
      uint32_t VC_ENABLE : 1;
      uint32_t EXPORT_SRC_C : 1;
      uint32_t DX9_CONSTS : 1;
      uint32_t ALU_INST_PREFER_VECTOR : 1;
      uint32_t DX10_CLAMP : 1;
      uint32_t ALU_PREFER_ONE_WATERFALL : 1;
      uint32_t ALU_MAX_ONE_WATERFALL : 1;
      uint32_t : 1;
      uint32_t CLAUSE_SEQ_PRIO : 2;
      uint32_t NO_GPR_CLAMP : 1;
      uint32_t EN_TEX_SKEW : 1;
      uint32_t : 12;
      uint32_t PS_PRIO : 2;
      uint32_t VS_PRIO : 2;
      uint32_t GS_PRIO : 2;
      uint32_t ES_PRIO : 2;
   };
};

// Used for SQ_CF_INST_LOOP and SQ_CF_INST_LOOP_NO_AL
struct SQ_LOOP_CONST_DX9_0
{
   uint32_t COUNT : 12;
   uint32_t INIT : 12;
   uint32_t INC : 8;
};

// Used for SQ_CF_INST_LOOP_DX10
struct SQ_LOOP_CONST_DX10_0
{
   uint32_t COUNT;
};

union SQ_VTX_CONSTANT_WORD2_0
{
   uint32_t value;

   struct
   {
      uint32_t BASE_ADDRESS_HI : 8;
      uint32_t STRIDE : 11;
      SQ_VTX_CLAMP CLAMP_X : 1;
      SQ_DATA_FORMAT DATA_FORMAT : 6;
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
      SQ_DATA_FORMAT DATA_FORMAT : 6;
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

union SQ_TEX_SAMPLER_WORD0_0
{
   uint32_t value;

   struct
   {
      SQ_TEX_CLAMP CLAMP_X : 3;
      SQ_TEX_CLAMP CLAMP_Y : 3;
      SQ_TEX_CLAMP CLAMP_Z : 3;
      SQ_TEX_XY_FILTER XY_MAG_FILTER : 3;
      SQ_TEX_XY_FILTER XY_MIN_FILTER : 3;
      SQ_TEX_Z_FILTER Z_FILTER : 2;
      SQ_TEX_Z_FILTER MIP_FILTER : 2;
      SQ_TEX_ANISO MAX_ANISO_RATIO : 3;
      SQ_TEX_BORDER_COLOR BORDER_COLOR_TYPE : 2;
      uint32_t POINT_SAMPLING_CLAMP : 1;
      uint32_t TEX_ARRAY_OVERRIDE : 1;
      SQ_TEX_DEPTH_COMPARE DEPTH_COMPARE_FUNCTION : 3;
      SQ_TEX_CHROMA_KEY CHROMA_KEY : 2;
      uint32_t LOD_USES_MINOR_AXIS : 1;
   };
};

union SQ_TEX_SAMPLER_WORD1_0
{
   uint32_t value;

   struct
   {
      uint32_t MIN_LOD : 10;
      uint32_t MAX_LOD : 10;
      uint32_t LOD_BIAS : 12;
   };
};

union SQ_TEX_SAMPLER_WORD2_0
{
   uint32_t value;

   struct
   {
      uint32_t LOD_BIAS_SEC : 12;
      uint32_t MC_COORD_TRUNCATE : 1;
      uint32_t FORCE_DEGAMMA : 1;
      uint32_t HIGH_PRECISION_FILTER : 1;
      uint32_t PERF_MIP : 3;
      uint32_t PERF_Z : 2;
      uint32_t ANISO_BIAS : 6;
      uint32_t FETCH_4 : 1;
      uint32_t SAMPLE_IS_PCF : 1;
      uint32_t TRUNCATE_COORD : 1;
      uint32_t DISABLE_CUBE_WRAP : 1;
      uint32_t: 1;
      uint32_t TYPE : 1;
   };
};

} // namespace latte
