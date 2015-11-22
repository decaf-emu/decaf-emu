#pragma once
#include "types.h"
#include "latte_enum_common.h"
#include "latte_enum_db.h"

namespace latte
{

struct DB_DEPTH_BASE
{
   uint32_t BASE_256B;
};

struct DB_DEPTH_HTILE_DATA_BASE
{
   uint32_t BASE_256B;
};

union DB_DEPTH_INFO
{
   uint32_t value;

   struct
   {
      DB_DEPTH_FORMAT FORMAT : 3;
      READ_SIZE READ_SIZE : 1;
      uint32_t : 11;
      ARRAY_MODE ARRAY_MODE : 4;
      uint32_t : 6;
      uint32_t TILE_SURFACE_ENABLE : 1;
      uint32_t TILE_COMPACT : 1;
      uint32_t : 4;
      uint32_t ZRANGE_PRECISION : 1;
   };
};

union DB_DEPTH_SIZE
{
   uint32_t value;

   struct
   {
      uint32_t PITCH_TILE_MAX : 10;
      uint32_t SLICE_TILE_MAX : 20;
      uint32_t : 2;
   };
};

union DB_DEPTH_VIEW
{
   uint32_t value;

   struct
   {
      uint32_t SLICE_START : 11;
      uint32_t : 2;
      uint32_t SLICE_MAX : 11;
      uint32_t : 8;
   };
};

union DB_HTILE_SURFACE
{
   uint32_t value;

   struct {
      uint32_t HTILE_WIDTH : 1;
      uint32_t HTILE_HEIGHT : 1;
      uint32_t LINEAR : 1;
      uint32_t FULL_CACHE : 1;
      uint32_t HTILE_USES_PRELOAD_WIN : 1;
      uint32_t PRELOAD : 1;
      uint32_t PREFETCH_WIDTH : 6;
      uint32_t PREFETCH_HEIGHT : 6;
      uint32_t : 14;
   };
};

union DB_DEPTH_CLEAR
{
   uint32_t value;
   float DEPTH_CLEAR;
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
      REF_FUNC ZFUNC : 3;
      uint32_t BACKFACE_ENABLE : 1;
      REF_FUNC STENCILFUNC : 3;
      DB_STENCIL_FUNC STENCILFAIL : 3;
      DB_STENCIL_FUNC STENCILZPASS : 3;
      DB_STENCIL_FUNC STENCILZFAIL : 3;
      REF_FUNC STENCILFUNC_BF : 3;
      DB_STENCIL_FUNC STENCILFAIL_BF : 3;
      DB_STENCIL_FUNC STENCILZPASS_BF : 3;
      DB_STENCIL_FUNC STENCILZFAIL_BF : 3;
   };
};

union DB_STENCILREFMASK
{
   uint32_t value;

   struct
   {
      uint32_t STENCILREF : 8;
      uint32_t STENCILMASK : 8;
      uint32_t STENCILWRITEMASK: 8;
   };
};

union DB_STENCILREFMASK_BF
{
   uint32_t value;

   struct
   {
      uint32_t STENCILREF_BF : 8;
      uint32_t STENCILMASK_BF : 8;
      uint32_t STENCILWRITEMASK_BF : 8;
   };
};

union DB_PREFETCH_LIMIT
{
   uint32_t value;

   struct
   {
      uint32_t DEPTH_HEIGHT_TILE_MAX : 10;
      uint32_t : 22;
   };
};

union DB_PRELOAD_CONTROL
{
   uint32_t value;

   struct
   {
      uint32_t START_X : 8;
      uint32_t START_Y : 8;
      uint32_t MAX_X : 8;
      uint32_t MAX_Y : 8;
   };
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

} // namespace latte
