#ifndef CAFE_DMAE_ENUM_H
#define CAFE_DMAE_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(dmae)

ENUM_BEG(DMAEEndianSwapMode, uint32_t)
   ENUM_VALUE(None,                    0)
   ENUM_VALUE(Swap8In16,               1)
   ENUM_VALUE(Swap8In32,               2)
ENUM_END(DMAEEndianSwapMode)

ENUM_NAMESPACE_END(dmae)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_DMAE_ENUM_H
