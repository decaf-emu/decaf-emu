#pragma once
#include "address.h"

namespace cpu
{

template<typename AddressType, typename FunctionType>
struct func_pointer_cast_impl;

template<typename AddressType, typename FunctionType>
class FunctionPointer;

template<typename AddressType, typename FunctionType>
class FunctionPointer
{
public:
   using function_type = FunctionType;

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

   FunctionPointer &
   operator =(std::nullptr_t)
   {
      mAddress = AddressType { 0 };
      return *this;
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
   template<typename, typename>
   friend struct func_pointer_cast_impl;

   AddressType mAddress;
};

template<typename FunctionType>
using VirtualFunctionPointer = FunctionPointer<VirtualAddress, FunctionType>;

template<typename FunctionType>
using PhysicalFunctionPointer = FunctionPointer<PhysicalAddress, FunctionType>;

template<typename AddressType, typename FunctionType>
struct func_pointer_cast_impl
{
   using FunctionPointerType = FunctionPointer<AddressType, FunctionType>;

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

template<typename AddressType, typename FunctionType>
struct func_pointer_cast_impl<AddressType, FunctionPointer<AddressType, FunctionType>>
{
   using FunctionPointerType = FunctionPointer<AddressType, FunctionType>;

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
