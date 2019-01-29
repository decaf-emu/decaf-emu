#ifndef LATTE_ENUM_DB_H
#define LATTE_ENUM_DB_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(latte)

ENUM_BEG(DB_FORMAT, uint32_t)
   ENUM_VALUE(DEPTH_INVALID,              0)
   ENUM_VALUE(DEPTH_16,                   1)
   ENUM_VALUE(DEPTH_X8_24,                2)
   ENUM_VALUE(DEPTH_8_24,                 3)
   ENUM_VALUE(DEPTH_X8_24_FLOAT,          4)
   ENUM_VALUE(DEPTH_8_24_FLOAT,           5)
   ENUM_VALUE(DEPTH_32_FLOAT,             6)
   ENUM_VALUE(DEPTH_X24_8_32_FLOAT,       7)
ENUM_END(DB_FORMAT)

ENUM_BEG(DB_FORCE, uint32_t)
   ENUM_VALUE(OFF,                        0)
   ENUM_VALUE(ENABLE,                     1)
   ENUM_VALUE(DISABLE,                    2)
ENUM_END(DB_FORCE)

ENUM_BEG(DB_STENCIL_FUNC, uint32_t)
   ENUM_VALUE(KEEP,                       0)
   ENUM_VALUE(ZERO,                       1)
   ENUM_VALUE(REPLACE,                    2)
   ENUM_VALUE(INCR_CLAMP,                 3)
   ENUM_VALUE(DECR_CLAMP,                 4)
   ENUM_VALUE(INVERT,                     5)
   ENUM_VALUE(INCR_WRAP,                  6)
   ENUM_VALUE(DECR_WRAP,                  7)
ENUM_END(DB_STENCIL_FUNC)

ENUM_BEG(DB_Z_EXPORT, uint32_t)
   ENUM_VALUE(ANY_Z,                      0)
   ENUM_VALUE(LESS_THAN_Z,                1)
   ENUM_VALUE(GREATER_THAN_Z,             2)
ENUM_END(DB_Z_EXPORT)

ENUM_BEG(DB_Z_ORDER, uint32_t)
   ENUM_VALUE(LATE_Z,                     0)
   ENUM_VALUE(EARLY_Z_THEN_LATE_Z,        1)
   ENUM_VALUE(RE_Z,                       2)
   ENUM_VALUE(EARLY_Z_THEN_RE_Z,          3)
ENUM_END(DB_Z_ORDER)

ENUM_NAMESPACE_EXIT(latte)

#include <common/enum_end.inl>

#endif // ifdef LATTE_ENUM_DB_H
