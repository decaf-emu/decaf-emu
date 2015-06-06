#pragma once
#include <cstdint>
#include <map>
#include "systemexport.h"
#include "systemfunction.h"
#include "systemdata.h"

#define RegisterSystemFunction(fn) \
   registerExport(#fn, make_sysfunc(fn));

#define RegisterSystemFunctionName(name, fn) \
   registerExport(name, make_sysfunc(fn));

#define RegisterSystemFunctionManual(fn) \
   registerExport(#fn, make_manual_sysfunc(fn));

#define RegisterSystemDataName(name, data) \
   registerExport(name, make_sysdata(&data));

#define RegisterSystemData(data) \
   registerExport(#data, make_sysdata(&data));

class SystemModule
{
public:
   virtual void initialise() = 0;

   void registerExport(const char *name, SystemExport *exp);
   SystemExport *findExport(const std::string &name);
   uint32_t findExportAddress(const std::string &name);

protected:
   friend class System;
   std::map<std::string, SystemExport*> mExport;
};
