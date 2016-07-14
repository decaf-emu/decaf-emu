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
   uint32_t itemSize = 0;
};

// Create a HleData export from a data pointer
template<typename Type>
inline HleData *
makeData(Type **value, uint32_t arraySize = 1)
{
   auto data = new HleData();
   data->hostPtr = reinterpret_cast<void**>(value);
   data->size = sizeof(Type) * arraySize;
   data->itemSize = sizeof(Type);
   return data;
}

} // namespace kernel
