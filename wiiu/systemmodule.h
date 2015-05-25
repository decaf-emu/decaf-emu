#pragma once
#include <cstdint>
#include <map>
#include "function_traits.h"

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

   Type type;
};

struct SystemFunction : public SystemExport
{
   SystemFunction(void *fptr, size_t retSize, size_t arity) :
      SystemExport(SystemExport::Function),
      fptr(fptr),
      returnBytes(retSize),
      arity(arity)
   {
   }

   void *fptr;          // Function pointer
   size_t returnBytes;  // Sizeof return value
   size_t arity;        // Number of arguments
};

struct SystemData : public SystemExport
{
   SystemData() :
      SystemExport(SystemExport::Data)
   {
   }
};

#define RegisterSystemFunction(fn) \
   registerFunction(#fn, new SystemFunction { &fn, sizeof(function_traits<decltype(fn)>::return_type), function_traits<decltype(fn)>::arity });

class SystemModule
{
public:
   void registerFunction(std::string name, SystemFunction *exp);
   SystemExport *findExport(const std::string &name);

private:
   std::map<std::string, SystemExport*> mExport;
};
