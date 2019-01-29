#pragma once
#include <cstdint>
#include <common/align.h>
#include <type_traits>

namespace cpu
{

template<class Type>
class Address
{
public:
   using StorageType = uint32_t;

   constexpr Address() = default;

   constexpr explicit Address(StorageType address) :
      mAddress(address)
   {
   }

   explicit operator bool() const
   {
      return !!mAddress;
   }

   template<typename OtherType,
            typename = typename std::enable_if<std::is_integral<OtherType>::value>::type>
   explicit operator OtherType() const
   {
      return static_cast<OtherType>(mAddress);
   }

   constexpr bool operator == (const Address &other) const
   {
      return mAddress == other.mAddress;
   }

   constexpr bool operator != (const Address &other) const
   {
      return mAddress != other.mAddress;
   }

   constexpr bool operator >= (const Address &other) const
   {
      return mAddress >= other.mAddress;
   }

   constexpr bool operator <= (const Address &other) const
   {
      return mAddress <= other.mAddress;
   }

   constexpr bool operator > (const Address &other) const
   {
      return mAddress > other.mAddress;
   }

   constexpr bool operator < (const Address &other) const
   {
      return mAddress < other.mAddress;
   }

   constexpr Address &operator += (ptrdiff_t value)
   {
      mAddress = static_cast<StorageType>(mAddress + value);
      return *this;
   }

   constexpr Address &operator -= (ptrdiff_t value)
   {
      mAddress = static_cast<StorageType>(mAddress - value);
      return *this;
   }

   constexpr Address &operator &= (StorageType value)
   {
      mAddress = mAddress & value;
      return *this;
   }

   constexpr Address &operator ^= (StorageType value)
   {
      mAddress = mAddress ^ value;
      return *this;
   }

   constexpr Address operator + (ptrdiff_t value) const
   {
      return Address { static_cast<StorageType>(mAddress + value) };
   }

   constexpr Address operator - (ptrdiff_t value) const
   {
      return Address { static_cast<StorageType>(mAddress - value) };
   }

   constexpr ptrdiff_t operator - (const Address &other) const
   {
      return mAddress - other.mAddress;
   }

   constexpr Address operator & (StorageType value) const
   {
      return Address { static_cast<StorageType>(mAddress & value) };
   }

   constexpr Address operator ^ (StorageType value) const
   {
      return Address { static_cast<StorageType>(mAddress ^ value) };
   }

   constexpr StorageType operator % (StorageType value) const
   {
      return mAddress % value;
   }

   constexpr StorageType operator / (StorageType value) const
   {
      return mAddress / value;
   }

   constexpr StorageType operator << (StorageType value) const
   {
      return mAddress << value;
   }

   constexpr StorageType operator >> (StorageType value) const
   {
      return mAddress >> value;
   }

   constexpr StorageType getAddress() const
   {
      return mAddress;
   }

private:
   StorageType mAddress = 0;
};

template<typename Type>
struct AddressRange
{
   using address_type = Address<Type>;

   AddressRange(address_type start,
                uint32_t size) :
      start(start),
      size(size)
   {
   }

   AddressRange(address_type start,
                address_type end) :
      start(start),
      size(static_cast<uint32_t>(end - start + 1))
   {
   }

   constexpr bool
   contains(AddressRange &other) const
   {
      return (start <= other.start && start + size >= other.start + other.size);
   }

   address_type start;
   uint32_t size;
};

class Physical;
class Virtual;

using PhysicalAddress = Address<Physical>;
using VirtualAddress = Address<Virtual>;

using PhysicalAddressRange = AddressRange<Physical>;
using VirtualAddressRange = AddressRange<Virtual>;

} // namespace cpu

template<typename Type>
constexpr inline cpu::Address<Type>
align_up(cpu::Address<Type> value, size_t alignment)
{
   return cpu::Address<Type> { align_up(value.getAddress(), alignment) };
}

template<typename Type>
constexpr inline cpu::Address<Type>
align_down(cpu::Address<Type> value, size_t alignment)
{
   return cpu::Address<Type> { align_down(value.getAddress(), alignment) };
}

// Custom formatters for fmtlib
namespace fmt
{

inline namespace v5
{
template<typename Type, typename Char, typename Enabled>
struct formatter;
}

template<typename AddressType, typename Char>
struct formatter<cpu::Address<AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Address<AddressType> &addr, FormatContext &ctx)
   {
      return format_to(ctx.begin(), "0x{:08X}", addr.getAddress());
   }
};

} // namespace fmt
