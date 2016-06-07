#pragma once
#include <cstdint>
#include "kernel_hleexport.h"

namespace kernel
{

struct HleData : public HleExport
{
   HleData() :
      HleExport(HleExport::Data)
   {
   }

   void **hostPtr = nullptr;
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
