#pragma once
#include <string>

namespace kernel
{

struct HleSymbol
{
   enum Type
   {
      Undefined,
      Function,
      Data
   };

   HleSymbol() :
      type(Undefined), ppcPtr(nullptr), hostPtr(nullptr)
   {
   }

   HleSymbol(Type type_) :
      type(type_), ppcPtr(nullptr), hostPtr(nullptr)
   {
   }

   virtual ~HleSymbol() = default;

   Type type = Undefined;
   std::string name;
   std::string module;
   void *hostPtr = nullptr;
   void *ppcPtr = nullptr;
};

} // namespace kernel
