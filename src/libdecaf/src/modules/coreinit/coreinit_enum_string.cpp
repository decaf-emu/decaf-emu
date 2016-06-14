#include "coreinit_enum_string.h"

#define ENUM_BEG(name, type) std::string enumAsString(name enumValue) { using namespace name##_; switch (enumValue) {
#define ENUM_END(name) default: return std::to_string(static_cast<int>(enumValue)); } }
#define ENUM_VALUE(key, value) case key: return #key;

#include "coreinit_enum.h"
