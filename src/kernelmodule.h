#pragma once
#include <cstdint>
#include <map>
#include "kernelexport.h"
#include "kernelfunction.h"
#include "kerneldata.h"

#define RegisterKernelFunction(fn) \
   registerExport(#fn, kernel::makeFunction(fn));

#define RegisterKernelFunctionName(name, fn) \
   registerExport(name, kernel::makeFunction(fn));

#define RegisterKernelFunctionManual(fn) \
   registerExport(#fn, kernel::makeManualFunction(fn));

#define RegisterKernelDataName(name, data) \
   registerExport(name, kernel::makeData(&data));

#define RegisterKernelData(data) \
   registerExport(#data, kernel::makeData(&data));

using KernelExportMap = std::map<std::string, KernelExport*>;

struct LoadedModule;

class KernelModule
{
public:
   virtual ~KernelModule() {}

   virtual void initialise() = 0;
   virtual const KernelExportMap &getExportMap() const = 0;
   virtual KernelExport *findExport(const char *name) const = 0;
   virtual uint32_t findExportAddress(const char *name) const = 0;

   p32<LoadedModule>
   getHandle() const
   {
      return mHandle;
   }

   void setHandle(p32<LoadedModule> handle)
   {
      mHandle = handle;
   }

protected:
   p32<LoadedModule> mHandle;
};

template<typename ModuleType>
class KernelModuleImpl : public KernelModule
{
public:
   virtual ~KernelModuleImpl() { }

   KernelExport *
   findExport(const char *name) const
   {
      auto itr = getExportMap().find(name);

      if (itr == getExportMap().end()) {
         return nullptr;
      } else {
         return itr->second;
      }
   }

   uint32_t
   findExportAddress(const char *name) const
   {
      auto exp = findExport(name);

      if (exp) {
         if (exp->type == KernelExport::Function) {
            return reinterpret_cast<KernelFunction*>(exp)->vaddr;
         } else if (exp->type == KernelExport::Data) {
            return static_cast<uint32_t>(*reinterpret_cast<KernelData*>(exp)->vptr);
         }
      }

      return 0;
   }

   virtual const KernelExportMap &
   getExportMap() const
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
