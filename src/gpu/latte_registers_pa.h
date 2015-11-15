#pragma once
#include "types.h"
#include "latte_enum_spi.h"

namespace latte
{

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

// Polygon Offset Depth Buffer Format Control
union PA_SU_POLY_OFFSET_DB_FMT_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t POLY_OFFSET_NEG_NUM_DB_BITS : 8;
      uint32_t POLY_OFFSET_DB_IS_FLOAT_FMT : 1;
      uint32_t : 15;
   };
};

} // namespace latte
