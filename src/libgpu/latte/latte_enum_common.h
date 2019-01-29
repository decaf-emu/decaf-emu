#ifndef LATTE_ENUM_COMMON_H
#define LATTE_ENUM_COMMON_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(latte)

ENUM_BEG(BUFFER_ARRAY_MODE, uint32_t)
   ENUM_VALUE(LINEAR_GENERAL,             0)
   ENUM_VALUE(LINEAR_ALIGNED,             1)
   ENUM_VALUE(TILED_2D_THIN1,             4)
ENUM_END(BUFFER_ARRAY_MODE)

ENUM_BEG(BUFFER_READ_SIZE, uint32_t)
   ENUM_VALUE(READ_256_BITS,              0)
   ENUM_VALUE(READ_512_BITS,              1)
ENUM_END(BUFFER_READ_SIZE)

ENUM_BEG(REF_FUNC, uint32_t)
   ENUM_VALUE(NEVER,                      0)
   ENUM_VALUE(LESS,                       1)
   ENUM_VALUE(EQUAL,                      2)
   ENUM_VALUE(LESS_EQUAL,                 3)
   ENUM_VALUE(GREATER,                    4)
   ENUM_VALUE(NOT_EQUAL,                  5)
   ENUM_VALUE(GREATER_EQUAL,              6)
   ENUM_VALUE(ALWAYS,                     7)
ENUM_END(REF_FUNC)

ENUM_NAMESPACE_EXIT(latte)

#include <common/enum_end.inl>

#endif // ifdef LATTE_ENUM_COMMON_H
