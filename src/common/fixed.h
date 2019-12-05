#pragma once
#include "bitfield.h"
#include <cnl/fixed_point.h>

using ufixed_16_16_t = cnl::fixed_point<uint32_t, -16>;
using sfixed_1_0_15_t = cnl::fixed_point<int16_t, -15>;
using ufixed_0_16_t = cnl::fixed_point<uint16_t, -16>;
using ufixed_1_15_t = cnl::fixed_point<uint16_t, -15>;

using ufixed_1_5_t = cnl::fixed_point<uint32_t, -5>;
using ufixed_4_6_t = cnl::fixed_point<uint32_t, -6>;

using sfixed_1_3_1_t = cnl::fixed_point<int8_t, -1>;
using sfixed_1_3_3_t = cnl::fixed_point<int8_t, -3>;
using sfixed_1_5_6_t = cnl::fixed_point<int16_t, -6>;

template<typename FixedType, typename RepType>
static constexpr
FixedType fixed_from_data(RepType data)
{
   return cnl::from_rep<FixedType, typename FixedType::rep>{}(data);
}

template<typename Rep, int Exponent, int Radix>
static constexpr
Rep fixed_to_data(cnl::fixed_point<Rep, Exponent, Radix> data)
{
   return cnl::to_rep<cnl::fixed_point<Rep, Exponent, Radix>>{}(data);
}

// Specialise of BitfieldHelper for fixed_point
template<typename BitfieldType, unsigned Position, unsigned Bits, class Rep, int Exponent, int Radix>
struct BitfieldHelper<BitfieldType, cnl::fixed_point<Rep, Exponent, Radix>, Position, Bits>
{
   using FixedType = cnl::fixed_point<Rep, Exponent, Radix>;
   using ValueBitfield = BitfieldHelper<BitfieldType, Rep, Position, Bits>;

   static FixedType get(BitfieldType bitfield)
   {
      return cnl::from_rep<FixedType, Rep>{}(ValueBitfield::get(bitfield));
   }

   static inline BitfieldType set(BitfieldType bitfield, FixedType fixedValue)
   {
      return ValueBitfield::set(bitfield, cnl::to_rep<FixedType>{}(fixedValue));
   }
};
