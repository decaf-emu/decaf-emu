#pragma once
#include "types.h"
#include "latte_enum_common.h"
#include "latte_enum_sx.h"

namespace latte
{

union SX_ALPHA_TEST_CONTROL
{
   uint32_t value;

   struct
   {
      REF_FUNC ALPHA_FUNC: 3;
      uint32_t ALPHA_TEST_ENABLE : 1;
      uint32_t : 4;
      uint32_t ALPHA_TEST_BYPASS : 1;
      uint32_t : 23;
   };
};

union SX_ALPHA_REF
{
   uint32_t value;

   struct
   {
      float ALPHA_REF;
   };
};

} // namespace latte
