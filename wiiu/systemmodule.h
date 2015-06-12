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

using SystemExportMap = std::map<std::string, SystemExport*>;

class SystemModule
{
public:
   virtual ~SystemModule() {}

   virtual void initialise() = 0;
   virtual const SystemExportMap &getExportMap() const = 0;
   virtual SystemExport *findExport(const char *name) const = 0;
   virtual uint32_t findExportAddress(const char *name) const = 0;
};

template<typename ModuleType>
class SystemModuleImpl : public SystemModule
{
public:
   virtual ~SystemModuleImpl() { }

   SystemExport *findExport(const char *name) const
   {
      auto itr = getExportMap().find(name);

      if (itr == getExportMap().end()) {
         return nullptr;
      } else {
         return itr->second;
      }
   }

   uint32_t findExportAddress(const char *name) const
   {
      auto exp = findExport(name);

      if (exp) {
         if (exp->type == SystemExport::Function) {
            return reinterpret_cast<SystemFunction*>(exp)->vaddr;
         } else if (exp->type == SystemExport::Data) {
            return static_cast<uint32_t>(*reinterpret_cast<SystemData*>(exp)->vptr);
         }
      }

      return 0;
   }

   virtual const SystemExportMap &getExportMap() const
   {
      return SystemModuleImpl::getStaticExportMap();
   }

protected:
   static SystemExportMap &getStaticExportMap()
   {
      static std::map<std::string, SystemExport*> sExport;
      return sExport;
   }

   static void registerExport(const char *name, SystemExport *exp)
   {
      exp->name = name;
      getStaticExportMap().insert(std::make_pair(std::string(name), exp));
   }
};
