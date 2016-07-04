#pragma once
#include "common/types.h"
#include "common/bitfield.h"
#include "latte_enum_common.h"
#include "latte_enum_sx.h"

namespace latte
{

BITFIELD(SX_ALPHA_TEST_CONTROL, uint32_t)
   BITFIELD_ENTRY(0, 3, REF_FUNC, ALPHA_FUNC);
   BITFIELD_ENTRY(3, 1, bool, ALPHA_TEST_ENABLE);
   BITFIELD_ENTRY(8, 1, bool, ALPHA_TEST_BYPASS);
BITFIELD_END

union SX_ALPHA_REF
{
   uint32_t value;
   float ALPHA_REF;
};

} // namespace latte
