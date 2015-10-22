#pragma once
#include <cstdint>
#include <map>
#include "kernelexport.h"
#include "kernelfunction.h"
#include "kerneldata.h"
#include "utils/virtual_ptr.h"

#define RegisterKernelFunction(fn) \
   registerExport(#fn, kernel::makeFunction(fn))

#define RegisterKernelFunctionName(name, fn) \
   registerExport(name, kernel::makeFunction(fn))

#define RegisterKernelDataName(name, data) \
   registerExport(name, kernel::makeData(&data))

#define RegisterKernelData(data) \
   registerExport(#data, kernel::makeData(&data))

using KernelExportMap = std::map<std::string, KernelExport*>;

struct ModuleHandleData;

class KernelModule
{
public:
   virtual ~KernelModule() {}

   virtual void initialise() = 0;
   virtual const KernelExportMap &getExportMap() const = 0;
   virtual KernelExport *findExport(const char *name) const = 0;
   virtual virtual_ptr<void> findExportAddress(const char *name) const = 0;

   virtual_ptr<ModuleHandleData>
   getHandle() const
   {
      return mHandle;
   }

   void setHandle(virtual_ptr<ModuleHandleData> handle)
   {
      mHandle = handle;
   }

protected:
   virtual_ptr<ModuleHandleData> mHandle;
};

template<typename ModuleType>
class KernelModuleImpl : public KernelModule
{
public:
   virtual ~KernelModuleImpl() { }

   KernelExport *
   findExport(const char *name) const override
   {
      auto itr = getExportMap().find(name);

      if (itr == getExportMap().end()) {
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

   virtual const KernelExportMap &
   getExportMap() const override
   {
      return KernelModuleImpl::getStaticExportMap();
   }

protected:
   static KernelExportMap &
   getStaticExportMap()
   {
      static std::map<std::string, KernelExport*> sExport;
      return sExport;
   }

   static void
   registerExport(const char *name, KernelExport *exp)
   {
      exp->name = name;
      getStaticExportMap().insert(std::make_pair(std::string(name), exp));
   }
};
