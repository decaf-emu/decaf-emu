#pragma once
#include "address.h"

namespace cpu
{

template<typename AddressType, typename ReturnType, typename... ArgTypes>
struct func_pointer_cast_impl;

template<typename AddressType, typename ReturnType, typename... ArgTypes>
class FunctionPointer
{
public:
   FunctionPointer() = default;
   FunctionPointer(const FunctionPointer &other) = default;
   FunctionPointer(FunctionPointer &&other) = default;
   FunctionPointer &operator=(const FunctionPointer &) = default;
   FunctionPointer &operator=(FunctionPointer &&) = default;

   /**
   * Constructs a FunctionPointer from a nullptr
   */
   FunctionPointer(std::nullptr_t) :
      mAddress(0)
   {
   }

   AddressType getAddress() const
   {
      return mAddress;
   }

   explicit operator bool() const
   {
      return static_cast<bool>(mAddress);
   }

   constexpr bool operator ==(std::nullptr_t) const
   {
      return !static_cast<bool>(mAddress);
   }

   constexpr bool operator == (const FunctionPointer &other) const
   {
      return mAddress == other.mAddress;
   }

   constexpr bool operator != (const FunctionPointer &other) const
   {
      return mAddress != other.mAddress;
   }

protected:
   template<typename AddressType, typename ReturnType, typename... ArgTypes>
   friend struct func_pointer_cast_impl;

   AddressType mAddress;
};

template<typename ReturnType, typename... ArgTypes>
using VirtualFunctionPointer = FunctionPointer<VirtualAddress, ReturnType, ArgTypes...>;

template<typename ReturnType, typename... ArgTypes>
using PhysicalFunctionPointer = FunctionPointer<PhysicalAddress, ReturnType, ArgTypes...>;

template<typename AddressType, typename ReturnType, typename... ArgTypes>
struct func_pointer_cast_impl<AddressType, ReturnType, ArgTypes...>
{
   using FunctionPointerType = FunctionPointer<AddressType, ReturnType, ArgTypes...>;

   static constexpr AddressType cast(FunctionPointerType src)
   {
      return src.mAddress;
   }

   static constexpr FunctionPointerType cast(AddressType src)
   {
      FunctionPointerType dst;
      dst.mAddress = src;
      return dst;
   }
};

} // namespace cpu
