#pragma once
#include "types.h"
#include "latte_enum_cb.h"
#include "latte_enum_common.h"

namespace latte
{

// Blend function used for all render targets
union CB_BLENDN_CONTROL
{
   uint32_t value;

   struct
   {
      CB_BLEND_FUNC COLOR_SRCBLEND : 5;
      CB_COMB_FUNC COLOR_COMB_FCN : 3;
      CB_BLEND_FUNC COLOR_DESTBLEND : 5;
      uint32_t OPACITY_WEIGHT : 1;
      uint32_t : 2;
      CB_BLEND_FUNC ALPHA_SRCBLEND : 5;
      CB_COMB_FUNC ALPHA_COMB_FCN : 3;
      CB_BLEND_FUNC ALPHA_DESTBLEND : 5;
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

union CB_BLEND_RED
{
   uint32_t value;
   float BLEND_RED;
};

union CB_BLEND_GREEN
{
   uint32_t value;
   float BLEND_GREEN;
};

union CB_BLEND_BLUE
{
   uint32_t value;
   float BLEND_BLUE;
};

union CB_BLEND_ALPHA
{
   uint32_t value;
   float BLEND_ALPHA;
};

union CB_COLORN_BASE
{
   uint32_t value;
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

union CB_COLORN_INFO
{
   uint32_t value;

   struct
   {
      CB_ENDIAN ENDIAN : 2;
      CB_FORMAT FORMAT : 6;
      ARRAY_MODE ARRAY_MODE : 4;
      CB_NUMBER_TYPE NUMBER_TYPE : 3;
      READ_SIZE READ_SIZE : 1;
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

union CB_COLOR_CONTROL
{
   uint32_t value;

   struct
   {
      uint32_t FOG_ENABLE : 1;
      uint32_t MULTIWRITE_ENABLE : 1;
      uint32_t DITHER_ENABLE : 1;
      uint32_t DEGAMMA_ENABLE : 1;
      CB_SPECIAL_OP SPECIAL_OP : 3;
      uint32_t PER_MRT_BLEND : 1;
      uint32_t TARGET_BLEND_ENABLE : 8;
      uint32_t ROP3 : 8;
      uint32_t : 8;
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

// Contains color component mask fields for writing the render targets. Red, green, blue, and alpha
// are components 0, 1, 2, and 3 in the pixel shader and are enabled by bits 0, 1, 2, and 3 in each field
union CB_TARGET_MASK
{
   uint32_t value;

   struct
   {
      uint32_t TARGET0_ENABLE : 4;
      uint32_t TARGET1_ENABLE : 4;
      uint32_t TARGET2_ENABLE : 4;
      uint32_t TARGET3_ENABLE : 4;
      uint32_t TARGET4_ENABLE : 4;
      uint32_t TARGET5_ENABLE : 4;
      uint32_t TARGET6_ENABLE : 4;
      uint32_t TARGET7_ENABLE : 4;
   };
};

} // namespace latte
