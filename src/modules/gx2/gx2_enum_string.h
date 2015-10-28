#pragma once
#include <string>
#include "gx2_enum.h"

#define GX2_ENUM(name, type) std::string GX2EnumAsString(name::Value enumValue);
#define GX2_ENUM_VALUE(key, value)
#define GX2_ENUM_VALID_RANGE(min, max)
#define GX2_ENUM_END

#undef GX2_ENUM_H
#include "gx2_enum.h"
