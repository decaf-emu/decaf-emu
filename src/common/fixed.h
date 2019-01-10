#pragma once
#include "bitfield.h"
#include <sg14/fixed_point>

typedef sg14::make_fixed<3, 4, int8_t> fixed44_t;
typedef sg14::make_ufixed<4, 4, uint8_t> ufixed44_t;

typedef sg14::make_fixed<7, 8, int16_t> fixed88_t;
typedef sg14::make_ufixed<8, 8, uint16_t> ufixed88_t;

typedef sg14::make_fixed<15, 16, int32_t> fixed1616_t;
typedef sg14::make_ufixed<16, 16, uint32_t> ufixed1616_t;
typedef sg14::make_fixed<0, 15, int16_t> fixed016_t;
typedef sg14::make_ufixed<0, 16, uint16_t> ufixed016_t;
typedef sg14::make_fixed<15, 0, int16_t> fixed160_t;
typedef sg14::make_ufixed<16, 0, uint16_t> ufixed160_t;

typedef sg14::make_ufixed<1, 5, uint32_t> ufixed_1_5_t;
typedef sg14::make_ufixed<4, 6, uint32_t> ufixed_4_6_t;
typedef sg14::make_ufixed<1, 15, uint16_t> ufixed_1_15_t;
typedef sg14::make_fixed<5, 6, int32_t> sfixed_1_5_6_t;

using sfixed_1_3_1_t = sg14::make_fixed<3, 1, int8_t>;
using sfixed_1_3_3_t = sg14::make_fixed<3, 3, int8_t>;
using sfixed_1_5_6_t = sg14::make_fixed<5, 6, int16_t>;

// Specialise of BitfieldHelper for fixed_point
template<typename BitfieldType, unsigned Position, unsigned Bits, class Rep, int Exponent>
struct BitfieldHelper<BitfieldType, sg14::fixed_point<Rep, Exponent>, Position, Bits>
{
   using FixedType = sg14::fixed_point<Rep, Exponent>;
   using ValueBitfield = BitfieldHelper<BitfieldType, Rep, Position, Bits>;

   static FixedType get(BitfieldType bitfield)
   {
      return FixedType::from_data(ValueBitfield::get(bitfield));
   }

   static inline BitfieldType set(BitfieldType bitfield, FixedType fixedValue)
   {
      return ValueBitfield::set(bitfield, fixedValue.data());
   }
};
