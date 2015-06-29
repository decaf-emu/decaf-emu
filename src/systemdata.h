#pragma once
#include "systemtypes.h"
#include "systemexport.h"

struct SystemData : public SystemExport
{
   SystemData() :
      SystemExport(SystemExport::Data)
   {
   }

   p32<void> *vptr;
   uint32_t size;
};

// Create a SystemFunction export from a function pointer
template<typename Type>
inline SystemData *
make_sysdata(p32<Type> *value)
{
   auto data = new SystemData();
   data->vptr = reinterpret_cast<p32<void>*>(value);
   data->size = sizeof(Type);
   return data;
}
