#pragma once
#include "be_val.h"

template<typename Type, size_t Size>
class be_array
{
public:
   be_val<Type> &operator[](int index)
   {
      return mValues[index];
   }

   const be_val<Type> &operator[](int index) const
   {
      return mValues[index];
   }

   std::array<Type, Size> value() const
   {
      std::array<Type, Size> result;

      for (auto i = 0u; i < Size; ++i) {
         result[i] = mValues[i];
      }

      return result;
   }

private:
   be_val<Type> mValues[Size];
};
