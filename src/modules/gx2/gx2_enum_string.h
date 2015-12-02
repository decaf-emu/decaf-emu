#pragma once
#include <string>
#include "types.h"
#include "gx2_enum.h"

#define GX2_ENUM(name, type) std::string GX2EnumAsString(name enumValue);
#define GX2_ENUM_WITH_RANGE(name, type, min, max) GX2_ENUM(name, type)
#define GX2_ENUM_VALUE(key, value)
#define GX2_ENUM_END(name)

#undef GX2_ENUM_H
#include "gx2_enum.h"
