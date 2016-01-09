#pragma once
#include "types.h"
#include "latte_enum_spi.h"
#include "latte_enum_pa.h"

namespace latte
{

// Clipper Control Bits
union PA_CL_CLIP_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t UCP_ENA_0 : 1;
      uint32_t UCP_ENA_1 : 1;
      uint32_t UCP_ENA_2 : 1;
      uint32_t UCP_ENA_3 : 1;
      uint32_t UCP_ENA_4 : 1;
      uint32_t UCP_ENA_5 : 1;
      uint32_t : 7;
      uint32_t PS_UCP_Y_SCALE_NEG : 1;
      PA_PS_UCP_MODE PS_UCP_MODE : 2;
      uint32_t CLIP_DISABLE : 1;
      uint32_t UCP_CULL_ONLY_ENA : 1;
      uint32_t BOUNDARY_EDGE_FLAG_ENA : 1;
      uint32_t DX_CLIP_SPACE_DEF : 1;
      uint32_t DIS_CLIP_ERR_DETECT : 1;
      uint32_t VTX_KILL_OR : 1;
      uint32_t RASTERISER_DISABLE : 1;
      uint32_t : 1;
      uint32_t DX_LINEAR_ATTR_CLIP_ENA : 1;
      uint32_t VTE_VPORT_PROVOKE_DISABLE : 1;
      uint32_t ZCLIP_NEAR_DISABLE : 1;
      uint32_t ZCLIP_FAR_DISABLE : 1;
      uint32_t : 4;
   };
};

// Horizontal Guard Band Clip Adjust Register
union PA_CL_GB_HORZ_CLIP_ADJ
{
   uint32_t value;
   float DATA_REGISTER;
};

// Horizontal Guard Band Discard Adjust Register
union PA_CL_GB_HORZ_DISC_ADJ
{
   uint32_t value;
   float DATA_REGISTER;
};

// Vertical Guard Band Clip Adjust Register
union PA_CL_GB_VERT_CLIP_ADJ
{
   uint32_t value;
   float DATA_REGISTER;
};

// Vertical Guard Band Discard Adjust Register
union PA_CL_GB_VERT_DISC_ADJ
{
   uint32_t value;
   float DATA_REGISTER;
};

// Viewport Transform X Scale Factor
union PA_CL_VPORT_XSCALE_N
{
   uint32_t value;
   float VPORT_XSCALE;
};

using PA_CL_VPORT_XSCALE_0 = PA_CL_VPORT_XSCALE_N;
using PA_CL_VPORT_XSCALE_15 = PA_CL_VPORT_XSCALE_N;

// Viewport Transform X Offset
union PA_CL_VPORT_XOFFSET_N
{
   uint32_t value;
   float VPORT_XOFFSET;
};

using PA_CL_VPORT_XOFFSET_0 = PA_CL_VPORT_XOFFSET_N;
using PA_CL_VPORT_XOFFSET_15 = PA_CL_VPORT_XOFFSET_N;

// Viewport Transform Y Scale Factor
union PA_CL_VPORT_YSCALE_N
{
   uint32_t value;
   float VPORT_YSCALE;
};

using PA_CL_VPORT_YSCALE_0 = PA_CL_VPORT_YSCALE_N;
using PA_CL_VPORT_YSCALE_15 = PA_CL_VPORT_YSCALE_N;

// Viewport Transform Y Offset
union PA_CL_VPORT_YOFFSET_N
{
   uint32_t value;
   float VPORT_YOFFSET;
};

using PA_CL_VPORT_YOFFSET_0 = PA_CL_VPORT_YOFFSET_N;
using PA_CL_VPORT_YOFFSET_15 = PA_CL_VPORT_YOFFSET_N;

// Viewport Transform Z Scale Factor
union PA_CL_VPORT_ZSCALE_N
{
   uint32_t value;
   float VPORT_ZSCALE;
};

using PA_CL_VPORT_ZSCALE_0 = PA_CL_VPORT_ZSCALE_N;
using PA_CL_VPORT_ZSCALE_15 = PA_CL_VPORT_ZSCALE_N;

// Viewport Transform Z Offset
union PA_CL_VPORT_ZOFFSET_N
{
   uint32_t value;
   float VPORT_ZOFFSET;
};

using PA_CL_VPORT_ZOFFSET_0 = PA_CL_VPORT_ZOFFSET_N;
using PA_CL_VPORT_ZOFFSET_15 = PA_CL_VPORT_ZOFFSET_N;

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

// Multisample AA Mask
union PA_SC_AA_MASK
{
   uint32_t value;

   struct
   {
      uint32_t AA_MASK_ULC : 8;
      uint32_t AA_MASK_URC : 8;
      uint32_t AA_MASK_LLC : 8;
      uint32_t AA_MASK_LRC : 8;
   };
};

// Generic Scissor rectangle specification
union PA_SC_GENERIC_SCISSOR_BR
{
   uint32_t value;

   struct
   {
      uint32_t BR_X : 14;
      uint32_t : 2;
      uint32_t BR_Y : 14;
      uint32_t : 2;
   };
};

// Generic Scissor rectangle specification
union PA_SC_GENERIC_SCISSOR_TL
{
   uint32_t value;

   struct
   {
      uint32_t TL_X : 14;
      uint32_t : 2;
      uint32_t TL_Y : 14;
      uint32_t : 1;
      uint32_t WINDOW_OFFSET_DISABLE : 1;
   };
};

// Viewport Transform Z Min Clamp
union PA_SC_VPORT_ZMIN_N
{
   uint32_t value;
   float VPORT_ZMIN;
};

using PA_SC_VPORT_ZMIN_0 = PA_SC_VPORT_ZMIN_N;
using PA_SC_VPORT_ZMIN_15 = PA_SC_VPORT_ZMIN_N;

// Viewport Transform Z Max Clamp
union PA_SC_VPORT_ZMAX_N
{
   uint32_t value;
   float VPORT_ZMAX;
};

using PA_SC_VPORT_ZMAX_0 = PA_SC_VPORT_ZMAX_N;
using PA_SC_VPORT_ZMAX_15 = PA_SC_VPORT_ZMAX_N;

// Line control
union PA_SU_LINE_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t WIDTH : 16; // 16.0 fixed
      uint32_t : 16;
   };
};

// Specifies maximum and minimum point & sprite sizes for per vertex size specification
union PA_SU_POINT_MINMAX
{
   uint32_t value;

   struct
   {
      uint32_t MIN_SIZE : 16; // 12.4 fixed
      uint32_t MAX_SIZE : 16; // 12.4 fixed
   };
};

// Dimensions for Points
union PA_SU_POINT_SIZE
{
   uint32_t value;

   struct
   {
      uint32_t HEIGHT : 16; // 12.4 fixed
      uint32_t WIDTH : 16; // 12.4 fixed
   };
};

// Clamp Value for Polygon Offset
union PA_SU_POLY_OFFSET_CLAMP
{
   uint32_t value;
   float CLAMP;
};

// Back-Facing Polygon Offset Scale
union PA_SU_POLY_OFFSET_BACK_SCALE
{
   uint32_t value;
   float SCALE;
};

// Back-Facing Polygon Offset Scale
union PA_SU_POLY_OFFSET_BACK_OFFSET
{
   uint32_t value;
   float OFFSET;
};

// Front-Facing Polygon Offset Scale
union PA_SU_POLY_OFFSET_FRONT_SCALE
{
   uint32_t value;
   float SCALE;
};

// Front-Facing Polygon Offset Scale
union PA_SU_POLY_OFFSET_FRONT_OFFSET
{
   uint32_t value;
   float OFFSET;
};

// SU/SC Controls for Facedness Culling, Polymode, Polygon Offset, and various Enables
union PA_SU_SC_MODE_CNTL
{
   uint32_t value;

   struct
   {
      uint32_t CULL_FRONT : 1;
      uint32_t CULL_BACK : 1;
      PA_FACE FACE : 1;
      uint32_t POLY_MODE : 2;
      uint32_t POLYMODE_FRONT_PTYPE : 3;
      uint32_t POLYMODE_BACK_PTYPE : 3;
      uint32_t POLY_OFFSET_FRONT_ENABLE : 1;
      uint32_t POLY_OFFSET_BACK_ENABLE : 1;
      uint32_t POLY_OFFSET_PARA_ENABLE : 1;
      uint32_t : 2;
      uint32_t VTX_WINDOW_OFFSET_ENABLE : 1;
      uint32_t : 2;
      uint32_t PROVOKING_VTX_LAST : 1;
      uint32_t PERSP_CORR_DIS : 1;
      uint32_t MULTI_PRIM_IB_ENA : 1;
      uint32_t : 10;
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
