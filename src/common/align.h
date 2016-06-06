#pragma once

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
