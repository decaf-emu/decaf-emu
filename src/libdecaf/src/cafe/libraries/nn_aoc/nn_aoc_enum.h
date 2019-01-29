#ifndef CAFE_NN_AOC_ENUM_H
#define CAFE_NN_AOC_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn_aoc)

ENUM_BEG(AOCError, int32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(GenericError,      -1)
ENUM_END(AOCError)

ENUM_NAMESPACE_EXIT(nn_aoc)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_NN_AOC_ENUM_H
