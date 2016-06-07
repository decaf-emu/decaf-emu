#pragma once
#include <cstdint>
#include "kernel_hlesymbol.h"

namespace kernel
{

struct HleData : public HleSymbol
{
   HleData() :
      HleSymbol(HleSymbol::Data)
   {
   }

   uint32_t size = 0;
};

// Create a HleData export from a data pointer
template<typename Type>
inline HleData *
makeData(Type **value)
{
   auto data = new HleData();
   data->hostPtr = reinterpret_cast<void**>(value);
   data->size = sizeof(Type);
   return data;
}

};
