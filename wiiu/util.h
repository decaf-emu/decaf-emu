#pragma once

template<typename Type>
static inline bool
testFlag(Type flags, Type flag)
{
   return !!(static_cast<unsigned>(flags) & static_cast<unsigned>(flag));
}

template<typename Type>
static inline Type
alignUp(Type value, size_t alignment)
{
   return static_cast<Type>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename Type>
static inline Type
alignDown(Type value, size_t alignment)
{
   return static_cast<Type>(static_cast<size_t>(value) & ~(alignment - 1));
}

template<typename Type>
static inline Type *
alignUp(Type *value, size_t alignment)
{
   return reinterpret_cast<Type>((reinterpret_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename Type>
static inline Type *
alignDown(Type *value, size_t alignment)
{
   return reinterpret_cast<Type>(reinterpret_cast<size_t>(value) & ~(alignment - 1));
}
