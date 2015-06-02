#pragma once
#include <cstdint>
#include <map>
#include "systemexport.h"
#include "systemfunction.h"

struct SystemData : public SystemExport
{
   SystemData() :
      SystemExport(SystemExport::Data)
   {
   }
};

#define RegisterSystemFunction(fn) \
   registerFunction(#fn, make_sysfunc(fn));

#define RegisterSystemFunctionName(name, fn) \
   registerFunction(name, make_sysfunc(fn));

#define RegisterSystemFunctionManual(fn) \
   registerFunction(#fn, make_manual_sysfunc(fn));

class SystemModule
{
public:
   void registerFunction(std::string name, SystemExport *exp);
   SystemExport *findExport(const std::string &name);

private:
   std::map<std::string, SystemExport*> mExport;
};
