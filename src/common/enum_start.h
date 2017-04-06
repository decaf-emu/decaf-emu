
#ifndef ENUM_BEG
#include <cstdint>
#include <type_traits>
#define ENUM_BEG(name, type) namespace name##_ { enum Value : type {
#endif

#ifndef ENUM_END
#define ENUM_END(name) }; }; using name = name##_::Value;
#endif

#ifndef ENUM_VALUE
#define ENUM_VALUE(key, value) key = value,
#endif

#ifndef FLAGS_BEG
#define FLAGS_BEG(name, type) namespace name##_ { enum Value : type {
#endif

#ifndef FLAGS_END
#define FLAGS_END(E) }; }; using E = E##_::Value; \
   inline E operator | (E lhs, E rhs) { return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) | static_cast<std::underlying_type_t<E>>(rhs)); } \
   inline E operator & (E lhs, E rhs) { return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) & static_cast<std::underlying_type_t<E>>(rhs)); } \
   inline E operator ^ (E lhs, E rhs) { return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) ^ static_cast<std::underlying_type_t<E>>(rhs)); } \
   inline E operator ~ (E lhs) { return static_cast<E>(~static_cast<std::underlying_type_t<E>>(lhs)); }
#endif

#ifndef FLAGS_VALUE
#define FLAGS_VALUE(key, value) key = value,
#endif

#ifndef ENUM_NAMESPACE_BEG
#define ENUM_NAMESPACE_BEG(name) namespace name {
#endif

#ifndef ENUM_NAMESPACE_END
#define ENUM_NAMESPACE_END(name) }
#endif
