#pragma once
#include <cassert>
#include <type_traits>

template<typename BitfieldType, typename ValueType, unsigned Position, unsigned Bits>
struct BitfieldField
{
   static const auto MinValue = static_cast<ValueType>(0);
   static const auto MaxValue = static_cast<ValueType>((1ull << (Bits)) - 1);
   static const auto Mask = (static_cast<typename BitfieldType::StorageType>((1ull << (Bits)) - 1)) << (Position);

   constexpr ValueType get() const
   {
      return static_cast<ValueType>((parent.value & Mask) >> (Position));
   }

   inline BitfieldType set(ValueType value)
   {
      assert(value >= MinValue);
      assert(value <= MaxValue);
      parent.value &= ~Mask;
      parent.value |= static_cast<typename BitfieldType::StorageType>(value) << (Position);
      return parent;
   }

   operator ValueType() const
   {
      return get();
   }

   BitfieldType parent;
};

// Specialise for bool because of compiler warnings for static_cast<bool>(int)
template<typename BitfieldType, unsigned Position, unsigned Bits>
struct BitfieldField<BitfieldType, bool, Position, Bits>
{
   static const auto Mask = (static_cast<typename BitfieldType::StorageType>((1ull << (Bits)) - 1)) << (Position);

   constexpr bool get() const
   {
      return !!((parent.value & Mask) >> (Position));
   }

   inline BitfieldType set(bool value)
   {
      parent.value &= ~Mask;
      parent.value |= (static_cast<typename BitfieldType::StorageType>(value ? 1 : 0)) << (Position);
      return parent;
   }

   operator bool() const
   {
      return get();
   }

   BitfieldType parent;
};


#define BITFIELD(Name, Type) \
   union Name { \
      using MyType = Name; \
      using StorageType = Type; \
      Type value; \
      static inline Name get(Type v) { \
         Name bitfield; \
         bitfield.value = v; \
         return bitfield; \
      }

#ifndef DECAF_USE_STDLAYOUT_BITFIELD

#define BITFIELD_ENTRY(Pos, Size, Type, Name) \
   private: struct { StorageType : Pos; StorageType _##Name : Size; }; \
   public: inline BitfieldField<MyType, Type, Pos, Size> Name() const { return { *this }; }

#else

#define BITFIELD_ENTRY(Pos, Size, Type, Name) \
   inline BitfieldField<MyType, Type, Pos, Size> Name() const { return { *this }; }

#endif

#define BITFIELD_END \
   };
