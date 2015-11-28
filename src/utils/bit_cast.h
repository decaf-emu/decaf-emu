#pragma once
#include <cstring>
#include <type_traits>

// reinterpret_cast for value types
template<typename DstType, typename SrcType>
static inline DstType
bit_cast(const SrcType& src)
{
   static_assert(sizeof(SrcType) == sizeof(DstType), "bit_cast must be between same sized types");
   static_assert(std::is_trivially_copyable<SrcType>(), "SrcType is not trivially copyable.");
   static_assert(std::is_trivially_copyable<DstType>(), "DstType is not trivially copyable.");

   DstType dst;
   std::memcpy(&dst, &src, sizeof(SrcType));
   return dst;
}
