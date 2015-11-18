#pragma once
#include "types.h"

template<size_t IntegerBits, size_t FractionalBits>
class Fixed
{
public:
   // TODO: Stuff.

private:
   union
   {
      uint32_t mValue;

      struct
      {
         uint32_t mInteger : IntegerBits;
         uint32_t mFraction : FractionalBits;
      };
   };
};
