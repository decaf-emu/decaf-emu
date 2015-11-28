#pragma once
#include "types.h"
#include "latte_enum_spi.h"

namespace latte
{

union SPI_INPUT_Z
{
   uint32_t value;

   struct
   {
      uint32_t PROVIDE_Z_TO_SPI : 1;
      uint32_t : 31;
   };
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
      SPI_BARYC_CNTL BARYC_SAMPLE_CNTL : 2;
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
union SPI_VS_OUT_ID_N
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

using SPI_VS_OUT_ID_0 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_1 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_2 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_3 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_4 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_5 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_6 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_7 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_8 = SPI_VS_OUT_ID_N;
using SPI_VS_OUT_ID_9 = SPI_VS_OUT_ID_N;

} // namespace latte
