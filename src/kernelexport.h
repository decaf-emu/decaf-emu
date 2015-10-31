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

   virtual ~KernelExport()
   {
   }

   Type type;
   std::string name;
   void *ppcPtr;
};
