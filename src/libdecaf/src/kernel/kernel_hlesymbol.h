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

   //! Symbol type
   Type type = Undefined;

   //! Symbol name
   std::string name;

   //! Module which this symbol comes from, this is only set for unimplemented symbols
   std::string module;

   //! A pointer to the PPC allocated memory where this symbol lives
   void *ppcPtr = nullptr;

   //! A C++ pointer that should be updated to point to the PPC allocated memory,
   //! this is used for statically allocating PPC data in kernel modules
   void *hostPtr = nullptr;

   //! Whether this symbol is exported from the module or not
   bool exported = false;
};

} // namespace kernel
