#ifndef CAFE_NN_AOC_ENUM_H
#define CAFE_NN_AOC_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(nn)
ENUM_NAMESPACE_BEG(aoc)

ENUM_BEG(AOCError, int32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(GenericError,      -1)
ENUM_END(AOCError)

ENUM_NAMESPACE_END(aoc)
ENUM_NAMESPACE_END(nn)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_NN_AOC_ENUM_H
