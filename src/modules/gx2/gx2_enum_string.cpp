#include "gx2_enum_string.h"

#define GX2_ENUM(name, type) std::string GX2EnumAsString(name ::Value enumValue) { switch (enumValue) {
#define GX2_ENUM_END default: return std::to_string(static_cast<int>(enumValue)); } }
#define GX2_ENUM_VALUE(key, value) case value: return #key;
#define GX2_ENUM_VALID_RANGE(key, value)

#undef GX2_ENUM_H
#include "gx2_enum.h"
