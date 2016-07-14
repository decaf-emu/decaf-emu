#pragma once
#include <cstdint>
#include <map>
#include "kernel_hlesymbol.h"
#include "kernel_hlefunction.h"
#include "kernel_hledata.h"
#include "virtual_ptr.h"
#include "ppcutils/wfunc_ptr.h"

#define RegisterKernelFunction(fn) \
   RegisterKernelFunctionName(#fn, fn)

#define RegisterKernelData(data) \
   RegisterKernelDataName(#data, data)

#define RegisterKernelFunctionConstructor(name, cls) \
   _RegisterKernelFunctionConstructor<cls>(name)

#define RegisterKernelFunctionConstructorArgs(name, cls, ...) \
   _RegisterKernelFunctionConstructor<cls, __VA_ARGS__>(name)

#define RegisterKernelFunctionDestructor(name, cls) \
   _RegisterKernelFunctionDestructor<cls>(name);

#define RegisterInternalFunction(fn, ...) \
   RegisterInternalFunctionName(#fn, fn, __VA_ARGS__)

#define RegisterInternalData(data) \
   RegisterInternalDataName(#data, data)

#define FindSymbol(sym) \
   findSymbol<decltype(sym)>(#sym)

namespace kernel
{

typedef std::vector<HleSymbol*> HleSymbolList;

class HleModule
{
public:
   virtual ~HleModule() = default;

   virtual void initialise() = 0;
   virtual const HleSymbolList &getSymbols() const = 0;
   virtual const HleSymbolList &getExports() const = 0;
   virtual virtual_ptr<void> findExportAddress(const char *name) const = 0;
};

template<typename ModuleType>
class HleModuleImpl : public HleModule
{
public:
   virtual ~HleModuleImpl() override = default;

   virtual const HleSymbolList &
   getSymbols() const override
   {
      return getStaticSymbolList();
   }

   virtual const HleSymbolList &
   getExports() const override
   {
      return HleModuleImpl::getStaticExportList();
   }

   // TODO: This should be removed in place of hostPtr stuff
   virtual virtual_ptr<void>
   findExportAddress(const char *name) const override
   {
      for (auto &i : getExports()) {
         if (i->name.compare(name) == 0) {
            return i->ppcPtr;
         }
      }
      return nullptr;
   }

protected:
   template<typename ReturnType, typename... Args>
   static void RegisterKernelFunctionName(const std::string &name, ReturnType(*fn)(Args...))
   {
      registerExportedSymbol(name, kernel::makeFunction(fn));
   }

   template<typename ReturnType, typename Class, typename... Args>
   static void RegisterKernelFunctionName(const std::string &name, ReturnType(Class::*fn)(Args...))
   {
      registerExportedSymbol(name, kernel::makeFunction(fn));
   }

   template <typename ...Args>
   static void _RegisterKernelFunctionConstructor(const std::string &name)
   {
      registerExportedSymbol(name, kernel::makeConstructor<Args...>());
   }

   template <typename Type>
   static void _RegisterKernelFunctionDestructor(const std::string &name)
   {
      registerExportedSymbol(name, kernel::makeDestructor<Type>());
   }

   template <typename Type>
   static void RegisterKernelDataName(const std::string &name, Type &data)
   {
      registerExportedSymbol(name, kernel::makeData(&data));
   }

   template<typename HostType, typename ReturnType, typename... Args>
   static void RegisterInternalFunctionName(const std::string &name, ReturnType(*fn)(Args...))
   {
      static_assert(std::is_same<wfunc_ptr<ReturnType, Args...>, HostType>::value, "Internal function declaration and variable types do not match.");
      registerSymbol(name, kernel::makeFunction(fn));
   }

   template<typename HostType, typename ReturnType, typename... Args>
   static void RegisterInternalFunctionName(const std::string &name, ReturnType(*fn)(Args...), HostType &hostPtr)
   {
      static_assert(std::is_same<wfunc_ptr<ReturnType, Args...>, HostType>::value, "Internal function declaration and variable types do not match.");
      registerSymbol(name, kernel::makeFunction(fn, &hostPtr));
   }

   template <typename Type>
   static void RegisterInternalDataName(const std::string &name, Type &data)
   {
      registerSymbol(name, kernel::makeData(&data));
   }

   template <typename Type, size_t N>
   static void RegisterInternalDataName(const std::string &name, std::array<Type*, N> &data)
   {
      registerSymbol(name, kernel::makeData(&data[0], N));
   }

private:
   static HleSymbolList &
   getStaticExportList()
   {
      static HleSymbolList sExport;
      return sExport;
   }

   static HleSymbolList &
   getStaticSymbolList()
   {
      static HleSymbolList sSymbol;
      return sSymbol;
   }

   static void
   registerExportedSymbol(const std::string& name, HleSymbol *exp)
   {
      exp->name = name;
      getStaticExportList().push_back(exp);
      getStaticSymbolList().push_back(exp);
   }

   static void
   registerSymbol(const std::string& name, HleSymbol *exp)
   {
      exp->name = name;
      getStaticSymbolList().push_back(exp);
   }
};

} // namespace kernel
