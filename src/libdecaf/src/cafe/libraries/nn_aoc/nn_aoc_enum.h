#ifndef CAFE_NN_AOC_ENUM_H
#define CAFE_NN_AOC_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn)
ENUM_NAMESPACE_ENTER(aoc)

ENUM_BEG(AOCError, int32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(GenericError,      -1)
ENUM_END(AOCError)

ENUM_NAMESPACE_EXIT(aoc)
ENUM_NAMESPACE_EXIT(nn)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_NN_AOC_ENUM_H
