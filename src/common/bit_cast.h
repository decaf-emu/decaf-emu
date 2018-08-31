#pragma once
#include <cstring>
#include <memory>
#include <type_traits>

// reinterpret_cast for value types
template<typename DstType, typename SrcType>
inline DstType
bit_cast(const SrcType& src)
{
   static_assert(sizeof(SrcType) == sizeof(DstType), "bit_cast must be between same sized types");
   static_assert(std::is_trivially_copyable<SrcType>::value, "SrcType is not trivially copyable.");
   static_assert(std::is_trivially_copyable<DstType>::value, "DstType is not trivially copyable.");

   DstType dst;
   std::memcpy(std::addressof(dst), std::addressof(src), sizeof(SrcType));
   return dst;
}
