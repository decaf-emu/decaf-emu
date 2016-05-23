#pragma once
#include <cassert>
#include <type_traits>

template<typename ParentType, typename Type>
struct Bitfield
{
   using StorageType = Type;
   using BitfieldType = ParentType;

   template<typename ValueType, unsigned Position, unsigned Bits>
   struct Field
   {
      static const auto MinValue = static_cast<ValueType>(0);
      static const auto MaxValue = static_cast<ValueType>((static_cast<StorageType>(1) << Bits) - 1);
      static const auto Mask = ((static_cast<StorageType>(1) << Bits) - 1) << Position;

      ValueType get() const
      {
         return static_cast<ValueType>((parent.value & Mask) >> Position);
      }

      BitfieldType set(ValueType value)
      {
         assert(value >= MinValue);
         assert(value <= MaxValue);
         parent.value &= ~Mask;
         parent.value |= static_cast<StorageType>(value) << Position;
         return parent;
      }

      operator ValueType() const
      {
         return get();
      }

      BitfieldType parent;
   };

   static BitfieldType get(StorageType value)
   {
      BitfieldType bitfield;
      bitfield.value = value;
      return bitfield;
   }

   StorageType value;
   static_assert(std::is_integral<StorageType>::value, "Bitfield storage type must be an integer type");
};

#define BITFIELD_ENTRY(Pos, Size, Type, Name) Field<Type, Pos, Size> Name() const { return { *this }; }
