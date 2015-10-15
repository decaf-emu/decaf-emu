#pragma once
#pragma pack(push, 1)

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
   return reinterpret_cast<Type*>((reinterpret_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename Type>
static inline Type *
alignDown(Type *value, size_t alignment)
{
   return reinterpret_cast<Type*>(reinterpret_cast<size_t>(value) & ~(alignment - 1));
}

template<typename Type, unsigned Size = sizeof(Type)>
struct storage_type_t;

template<typename Type>
struct storage_type_t<Type, 1>
{
   using value = uint8_t;
};

template<typename Type>
struct storage_type_t<Type, 2>
{
   using value = uint16_t;
};

template<typename Type>
struct storage_type_t<Type, 4>
{
   using value = uint32_t;
};

template<typename Type>
struct storage_type_t<Type, 8>
{
   using value = uint64_t;
};

template<typename...>
struct type_list
{
};

#pragma pack(pop)
