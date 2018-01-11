#pragma once
#include <cstddef>

template<typename Type>
constexpr inline Type
align_up(Type value, size_t alignment)
{
   return static_cast<Type>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename Type>
constexpr inline Type
align_down(Type value, size_t alignment)
{
   return static_cast<Type>(static_cast<size_t>(value) & ~(alignment - 1));
}

template<typename Type>
constexpr inline Type *
align_up(Type *value, size_t alignment)
{
   return reinterpret_cast<Type*>((reinterpret_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename Type>
constexpr inline Type *
align_down(Type *value, size_t alignment)
{
   return reinterpret_cast<Type*>(reinterpret_cast<size_t>(value) & ~(alignment - 1));
}

template<typename Type>
constexpr bool
align_check(Type *value, size_t alignment)
{
   return (reinterpret_cast<size_t>(value) & (alignment - 1)) == 0;
}

template<typename Type>
constexpr bool
align_check(Type value, size_t alignment)
{
   return (static_cast<size_t>(value) & (alignment - 1)) == 0;
}
