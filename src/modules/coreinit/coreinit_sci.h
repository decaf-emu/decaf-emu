#pragma once
#include "types.h"

namespace SCICountryCode
{
enum Country : uint32_t
{
   USA = 0x31,
   UnitedKingdom = 0x63,
};
}

namespace SCILanguage
{
enum Language : uint32_t
{
   English = 0x01,
};
}

namespace SCIRegion
{
enum Region : uint8_t
{
   US = 2,
   EUR = 4,
};
}
