#pragma once
#include <string>

namespace kernel
{

struct HleExport
{
   enum Type
   {
      Undefined,
      Function,
      Data
   };

   HleExport() :
      type(Undefined)
   {
   }

   HleExport(Type type) :
      type(type)
   {
   }

   virtual ~HleExport() = default;

   Type type = Undefined;
   std::string name;
   std::string module;
   void *ppcPtr = nullptr;
};

} // namespace kernel
