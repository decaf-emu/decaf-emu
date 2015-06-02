#pragma once

struct SystemExport
{
   enum Type
   {
      Undefined,
      Function,
      Data
   };

   SystemExport() :
      type(Undefined)
   {
   }

   SystemExport(Type type) :
      type(type)
   {
   }

   virtual ~SystemExport()
   {
   }

   Type type;
};
