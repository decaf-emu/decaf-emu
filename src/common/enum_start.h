
#ifndef ENUM_BEG
#include <cstdint>
#define ENUM_BEG(name, type) namespace name##_ { enum Value : type {
#endif

#ifndef ENUM_END
#define ENUM_END(name) }; } using name = name##_::Value;
#endif

#ifndef ENUM_VALUE
#define ENUM_VALUE(key, value) key = value,
#endif

#ifndef ENUM_NAMESPACE_BEG
#define ENUM_NAMESPACE_BEG(name) namespace name {
#endif

#ifndef ENUM_NAMESPACE_END
#define ENUM_NAMESPACE_END(name) }
#endif
