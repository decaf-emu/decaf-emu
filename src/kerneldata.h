#pragma once
#include "systemtypes.h"
#include "kernelexport.h"

struct KernelData : public KernelExport
{
   KernelData() :
      KernelExport(KernelExport::Data)
   {
   }

   p32<void> *vptr;
   uint32_t size;
};

namespace kernel
{

// Create a KernelData export from a data pointer
template<typename Type>
inline KernelData *
makeData(p32<Type> *value)
{
   auto data = new KernelData();
   data->vptr = reinterpret_cast<p32<void>*>(value);
   data->size = sizeof(Type);
   return data;
}

};
