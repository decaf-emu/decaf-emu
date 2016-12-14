#ifndef AOC_ENUM_H
#define AOC_ENUM_H

#include "common/enum_start.h"

ENUM_NAMESPACE_BEG(nn)

ENUM_NAMESPACE_BEG(aoc)

ENUM_BEG(AOCResult, int32_t)
   ENUM_VALUE(Success,        0)
   ENUM_VALUE(GenericError,   -1)
ENUM_END(AOCResult)

ENUM_NAMESPACE_END(aoc)

ENUM_NAMESPACE_END(nn)

#include "common/enum_end.h"

#endif // ifdef AOC_ENUM_H
