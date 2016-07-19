#pragma once
#include "bit_cast.h"
#include "bitutils.h"
#include <numeric>

union FloatBitsSingle
{
   static const unsigned exponent_min = 0;
   static const unsigned exponent_max = 0xff;

   float v;
   uint32_t uv;

   struct
   {
      uint32_t mantissa : 23;
      uint32_t exponent : 8;
      uint32_t sign : 1;
   };

   struct
   {
      uint32_t : 22;
      uint32_t quiet : 1;
      uint32_t : 9;
   };
};

union FloatBitsDouble
{
   static const uint64_t exponent_min = 0;
   static const uint64_t exponent_max = 0x7ff;

   double v;
   uint64_t uv;

   struct
   {
      uint64_t mantissa : 52;
      uint64_t exponent : 11;
      uint64_t sign : 1;
   };

   struct
   {
      uint64_t : 51;
      uint64_t quiet : 1;
      uint64_t : 12;
   };
};

inline FloatBitsSingle
get_float_bits(float v)
{
   return { v };
}

inline FloatBitsDouble
get_float_bits(double v)
{
   return { v };
}

template<typename Type>
inline bool
is_negative(Type v)
{
   return get_float_bits(v).sign;
}

template<typename Type>
inline bool
is_positive(Type v)
{
   return !is_negative(v);
}

template<typename Type>
inline bool
is_zero(Type v)
{
   auto b = get_float_bits(v);
   return b.exponent == 0 && b.mantissa == 0;
}

template<typename Type>
inline bool
is_positive_zero(Type v)
{
   return is_positive(v) && is_zero(v);
}

template<typename Type>
inline bool
is_negative_zero(Type v)
{
   return is_negative(v) && is_zero(v);
}

template<typename Type>
inline bool
is_normal(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent > d.exponent_min
       && d.exponent < d.exponent_max;
}

template<typename Type>
inline bool
is_denormal(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent == d.exponent_min
       && d.mantissa != 0;
}

template<typename Type>
inline bool
is_infinity(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent == d.exponent_max
       && d.mantissa == 0;
}

template<typename Type>
inline bool
is_positive_infinity(Type v)
{
   return is_positive(v) && is_infinity(v);
}

template<typename Type>
inline bool
is_negative_infinity(Type v)
{
   return is_negative(v) && is_infinity(v);
}

template<typename Type>
inline bool
is_nan(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent == d.exponent_max
       && d.mantissa != 0;
}

template<typename Type>
inline bool
is_quiet(Type v)
{
   return !!get_float_bits(v).quiet;
}

template<typename Type>
inline bool
is_quiet_nan(Type v)
{
   return is_nan(v) && is_quiet(v);
}

template<typename Type>
inline bool
is_signalling_nan(Type v)
{
   return is_nan(v) && !is_quiet(v);
}

template<typename Type>
Type
make_quiet(Type v)
{
   auto bits = get_float_bits(v);
   bits.quiet = 1;
   return bits.v;
}

template<typename Type>
Type
make_nan()
{
   auto bits = get_float_bits(static_cast<Type>(0));
   bits.exponent = bits.exponent_max;
   bits.quiet = 1;
   return bits.v;
}

inline uint64_t
extend_float_nan_bits(uint32_t v)
{
   return ((uint64_t)(v & 0xC0000000) << 32
           | (v & 0x40000000 ? UINT64_C(7) : UINT64_C(0)) << 59
           | (uint64_t)(v & 0x3FFFFFFF) << 29);
}

inline double
extend_float(float v)
{
   if (is_nan(v)) {
       return bit_cast<double>(extend_float_nan_bits(bit_cast<uint32_t>(v)));
   } else {
       return static_cast<double>(v);
   }
}

inline uint32_t
truncate_double_bits(uint64_t v)
{
   return (v>>32 & 0xC0000000) | (v>>29 & 0x3FFFFFFF);
}

inline float
truncate_double(double v)
{
   const FloatBitsDouble bits = get_float_bits(v);
   if (bits.exponent <= 873) {
      return bit_cast<float>(static_cast<uint32_t>(bits.sign)<<31);
   } else if (bits.exponent <= 896) {
      uint32_t mantissa = static_cast<uint32_t>(1<<23 | bits.mantissa>>29);
      return bit_cast<float>(static_cast<uint32_t>(bits.sign)<<31 | (mantissa >> (897 - bits.exponent)));
   } else if (bits.exponent >= 1151 && bits.exponent != 2047) {
      return bit_cast<float>(static_cast<uint32_t>(bits.sign)<<31 | 0x7F800000);
   } else {
       return bit_cast<float>(truncate_double_bits(bits.uv));
   }
}
