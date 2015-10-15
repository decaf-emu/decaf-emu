#pragma once

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
   const char *name;
   void *ppcPtr;
};
