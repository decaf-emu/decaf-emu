#include "gx2_enum_string.h"

#define GX2_ENUM(name, type) std::string GX2EnumAsString(name enumValue) { switch (enumValue) {
#define GX2_ENUM_WITH_RANGE(name, type, min, max) GX2_ENUM(name, type)
#define GX2_ENUM_END(name) default: return std::to_string(static_cast<int>(enumValue)); } }
#define GX2_ENUM_VALUE(key, value) case value: return #key;

#undef GX2_ENUM_H
#include "gx2_enum.h"
