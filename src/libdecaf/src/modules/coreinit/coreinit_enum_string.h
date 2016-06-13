#pragma once
#include <cstdint>
#include <string>
#include "coreinit_enum.h"

#define ENUM_BEG(name, type) std::string enumAsString(name enumValue);
#define ENUM_VALUE(key, value)
#define ENUM_END(name)

#undef COREINIT_ENUM_H
#include "coreinit_enum.h"
#undef COREINIT_ENUM_H
