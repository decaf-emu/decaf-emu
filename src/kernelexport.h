#pragma once

#include <string>

struct KernelExport
{
   enum Type
   {
      Undefined,
      Function,
      Data
   };

   KernelExport() :
      type(Undefined)
   {
   }

   KernelExport(Type type) :
      type(type)
   {
   }

   virtual ~KernelExport() = default;

   Type type = Undefined;
   std::string name;
   std::string module;
   void *ppcPtr = nullptr;
};
