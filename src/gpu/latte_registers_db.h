#pragma once
#include "types.h"
#include "latte_enum_db.h"

namespace latte
{

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
      DB_FRAG_FUNC ZFUNC : 3;
      uint32_t BACKFACE_ENABLE : 1;
      DB_REF_FUNC STENCILFUNC : 3;
      DB_STENCIL_FUNC STENCILFAIL : 3;
      DB_STENCIL_FUNC STENCILZPASS : 3;
      DB_STENCIL_FUNC STENCILZFAIL : 3;
      DB_REF_FUNC STENCILFUNC_BF : 3;
      DB_STENCIL_FUNC STENCILFAIL_BF : 3;
      DB_STENCIL_FUNC STENCILZPASS_BF : 3;
      DB_STENCIL_FUNC STENCILZFAIL_BF : 3;
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

} // namespace latte
