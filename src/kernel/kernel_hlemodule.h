#pragma once
#include <cstdint>
#include <map>
#include "kernel_hleexport.h"
#include "kernel_hlefunction.h"
#include "kernel_hledata.h"
#include "common/virtual_ptr.h"

#define RegisterKernelFunction(fn) \
   registerExport(#fn, kernel::makeFunction(fn))

#define RegisterKernelFunctionName(name, fn) \
   registerExport(name, kernel::makeFunction(fn))

#define RegisterKernelFunctionConstructor(name, cls) \
   registerExport(name, kernel::makeConstructor<cls>())

#define RegisterKernelFunctionConstructorArgs(name, cls, ...) \
   registerExport(name, kernel::makeConstructor<cls, __VA_ARGS__>())

#define RegisterKernelFunctionDestructor(name, cls) \
   registerExport(name, kernel::makeDestructor<cls>())

#define RegisterKernelDataName(name, data) \
   registerExport(name, kernel::makeData(&data))

#define RegisterKernelData(data) \
   registerExport(#data, kernel::makeData(&data))

namespace kernel
{

using HleExportMap = std::map<std::string, HleExport*>;

class HleModule
{
public:
   virtual ~HleModule() = default;

   virtual void initialise() = 0;
   virtual const HleExportMap &getExportMap() const = 0;
   virtual HleExport *findExport(const char *name) const = 0;
   virtual virtual_ptr<void> findExportAddress(const char *name) const = 0;

};

template<typename ModuleType>
class HleModuleImpl : public HleModule
{
public:
   virtual ~HleModuleImpl() override = default;

   HleExport *
      findExport(const char *name) const override
   {
      auto &map = getExportMap();
      auto itr = map.find(name);

      if (itr == map.end()) {
         return nullptr;
      } else {
         return itr->second;
      }
   }

   virtual_ptr<void>
      findExportAddress(const char *name) const override
   {
      auto exp = findExport(name);

      if (!exp) {
         return nullptr;
      }

      return exp->ppcPtr;
   }

   virtual const HleExportMap &
      getExportMap() const override
   {
      return HleModuleImpl::getStaticExportMap();
   }

protected:
   static HleExportMap &
      getStaticExportMap()
   {
      static std::map<std::string, HleExport*> sExport;
      return sExport;
   }

   static void
      registerExport(const char *name, HleExport *exp)
   {
      exp->name = name;
      getStaticExportMap().insert(std::make_pair(std::string(name), exp));
   }
};

} // namespace kernel
