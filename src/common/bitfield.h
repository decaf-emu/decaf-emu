#pragma once
#include "bit_cast.h"
#include "bitutils.h"
#include "decaf_assert.h"

#include <common/type_traits.h>
#include <fmt/core.h>

template<typename BitfieldType, typename ValueType, unsigned Position, unsigned Bits>
struct BitfieldHelper
{
   using underlying_type = typename safe_underlying_type<ValueType>::type;
   using unsigned_underlying_type = typename std::make_unsigned<underlying_type>::type;
   static const auto RelativeMask = static_cast<unsigned_underlying_type>((1ull << (Bits)) - 1);
   static const auto AbsoluteMask = static_cast<typename BitfieldType::StorageType>(RelativeMask) << (Position);

   static inline ValueType get(BitfieldType bitfield)
   {
      auto value = static_cast<unsigned_underlying_type>((bitfield.value & AbsoluteMask) >> (Position));

      if (std::is_signed<underlying_type>::value) {
         value = sign_extend<Bits>(value);
      }

      return bit_cast<ValueType>(value);
   }

   static inline BitfieldType set(BitfieldType bitfield,
                                  ValueType value)
   {
      auto uValue = bit_cast<unsigned_underlying_type>(value);

      if (std::is_signed<underlying_type>::value) {
         uValue &= RelativeMask;
      }

      decaf_assert(uValue <= RelativeMask, fmt::format("{} <= {}", uValue, static_cast<unsigned>(RelativeMask)));
      bitfield.value &= ~AbsoluteMask;
      bitfield.value |= static_cast<typename BitfieldType::StorageType>(uValue) << (Position);
      return bitfield;
   }
};

// Specialise for float because of errors using make_unsigned on float type
template<typename BitfieldType, unsigned Position, unsigned Bits>
struct BitfieldHelper<BitfieldType, float, Position, Bits>
{
   using ValueBitfield = BitfieldHelper<BitfieldType, uint32_t, Position, Bits>;

   static float get(BitfieldType bitfield)
   {
      return bit_cast<float>(ValueBitfield::get(bitfield));
   }

   static inline BitfieldType set(BitfieldType bitfield, float floatValue)
   {
      return ValueBitfield::set(bitfield, bit_cast<uint32_t>(floatValue));
   }
};

// Specialise for bool because of compiler warnings for static_cast<bool>(int)
template<typename BitfieldType, unsigned Position, unsigned Bits>
struct BitfieldHelper<BitfieldType, bool, Position, Bits>
{
   static const auto AbsoluteMask = (static_cast<typename BitfieldType::StorageType>((1ull << (Bits)) - 1)) << (Position);

   static constexpr bool get(BitfieldType bitfield)
   {
      return !!(bitfield.value & AbsoluteMask);
   }

   static inline BitfieldType set(BitfieldType bitfield, bool value)
   {
      bitfield.value &= ~AbsoluteMask;
      bitfield.value |= (static_cast<typename BitfieldType::StorageType>(value ? 1 : 0)) << (Position);
      return bitfield;
   }
};

#ifndef DECAF_USE_STDLAYOUT_BITFIELD

#define BITFIELD_BEG(Name, Type)                                                  \
   union Name                                                                 \
   {                                                                          \
      using BitfieldType = Name;                                              \
      using StorageType = Type;                                               \
      Type value;                                                             \
      explicit operator StorageType() { return value; }                       \
      static inline Name get(Type v) {                                        \
         Name bitfield;                                                       \
         bitfield.value = v;                                                  \
         return bitfield;                                                     \
      }

#define BITFIELD_ENTRY(Pos, Size, ValueType, Name)                            \
   private: struct { StorageType : Pos; StorageType _##Name : Size; };        \
   public: inline ValueType Name() const {                                    \
      return BitfieldHelper<BitfieldType, ValueType, Pos, Size>::get(*this);  \
   }                                                                          \
   inline BitfieldType Name(ValueType fieldValue) const {                     \
      return BitfieldHelper<BitfieldType, ValueType, Pos, Size>               \
            ::set(*this, fieldValue);                                         \
   }

#else

#define BITFIELD_BEG(Name, Type)                                                  \
   union Name                                                                 \
   {                                                                          \
      using BitfieldType = Name;                                              \
      using StorageType = Type;                                               \
      Type value;                                                             \
      explicit operator StorageType() { return value; }                       \
      static inline Name get(Type v) {                                        \
         Name bitfield;                                                       \
         bitfield.value = v;                                                  \
         return bitfield;                                                     \
      }

#define BITFIELD_ENTRY(Pos, Size, ValueType, Name)                            \
   inline ValueType Name() const {                                            \
      return BitfieldHelper<BitfieldType, ValueType, Pos, Size>::get(*this);  \
   }                                                                          \
   inline BitfieldType Name(ValueType fieldValue) const {                     \
      return BitfieldHelper<BitfieldType, ValueType, Pos, Size>               \
         ::set(*this, fieldValue);                                            \
   }

#endif

#define BITFIELD_END                                                          \
   };
