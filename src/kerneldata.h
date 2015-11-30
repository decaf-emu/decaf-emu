#pragma once
#include <cstdint>
#include "kernelexport.h"

struct KernelData : public KernelExport
{
   KernelData() :
      KernelExport(KernelExport::Data)
   {
   }

   void **hostPtr = nullptr;
   uint32_t size = 0;
};

namespace kernel
{

// Create a KernelData export from a data pointer
template<typename Type>
inline KernelData *
makeData(Type **value)
{
   auto data = new KernelData();
   data->hostPtr = reinterpret_cast<void**>(value);
   data->size = sizeof(Type);
   return data;
}

};
