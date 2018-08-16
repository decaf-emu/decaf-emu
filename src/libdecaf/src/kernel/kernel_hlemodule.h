#pragma once
#include "kernel_hlesymbol.h"
#include "kernel_hlefunction.h"
#include "kernel_hledata.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/log.h>
#include <cstdint>
#include <unordered_map>

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

namespace kernel
{

using HleSymbolMap = std::unordered_map<std::string, HleSymbol *>;

class HleModule
{
public:
   virtual ~HleModule() = default;

   virtual void initialise() = 0;

   virtual const HleSymbolMap &getSymbolMap() const = 0;
   virtual void *findExportAddress(const std::string &name) const = 0;

   virtual void setRpl(std::vector<uint8_t> &&data) = 0;
   virtual const std::vector<uint8_t> &getRpl() const = 0;
};

template<typename ModuleType>
class HleModuleImpl : public HleModule
{
public:
   virtual ~HleModuleImpl() override = default;

   virtual const HleSymbolMap &
   getSymbolMap() const override
   {
      return getStaticSymbolMap();
   }

   virtual void *
   findExportAddress(const std::string &name) const override
   {
      auto &symbols = getSymbolMap();
      auto itr = symbols.find(name);

      if (itr == symbols.end()) {
         return nullptr;
      }

      return itr->second->ppcPtr;
   }

   std::vector<uint8_t> rplData;
   virtual void setRpl(std::vector<uint8_t> &&data)
   {
      rplData = std::move(data);
   }

   virtual const std::vector<uint8_t> &getRpl() const
   {
      return rplData;
   }

protected:
   template<typename ReturnType, typename... Args>
   static void RegisterKernelFunctionName(const char *name, ReturnType(*fn)(Args...))
   {
      registerExportedSymbol(name, kernel::makeFunction(fn));
   }

   template<typename ReturnType, typename Class, typename... Args>
   static void RegisterKernelFunctionName(const char *name, ReturnType(Class::*fn)(Args...))
   {
      registerExportedSymbol(name, kernel::makeFunction(fn));
   }

   template <typename ...Args>
   static void _RegisterKernelFunctionConstructor(const char *name)
   {
      registerExportedSymbol(name, kernel::makeConstructor<Args...>());
   }

   template <typename Type>
   static void _RegisterKernelFunctionDestructor(const char *name)
   {
      registerExportedSymbol(name, kernel::makeDestructor<Type>());
   }

   template <typename Type>
   static void RegisterKernelDataName(const char *name, Type &data)
   {
      registerExportedSymbol(name, kernel::makeData(&data));
   }

   template <typename Type, size_t N>
   static void RegisterKernelDataName(const char *name, std::array<Type*, N> &data)
   {
      registerExportedSymbol(name, kernel::makeData(&data[0], N));
   }

   template<typename HostType, typename ReturnType, typename... Args>
   static void RegisterInternalFunctionName(const char *name, ReturnType(*fn)(Args...))
   {
      static_assert(std::is_same<wfunc_ptr<ReturnType, Args...>, HostType>::value, "Internal function declaration and variable types do not match.");
      registerSymbol(name, kernel::makeFunction(fn));
   }

   template<typename HostType, typename ReturnType, typename... Args>
   static void RegisterInternalFunctionName(const char *name, ReturnType(*fn)(Args...), HostType &hostPtr)
   {
      static_assert(std::is_same<wfunc_ptr<ReturnType, Args...>, HostType>::value, "Internal function declaration and variable types do not match.");
      registerSymbol(name, kernel::makeFunction(fn, &hostPtr));
   }

   template <typename Type>
   static void RegisterInternalDataName(const char *name, Type &data)
   {
      registerSymbol(name, kernel::makeData(&data));
   }

   template <typename Type, size_t N>
   static void RegisterInternalDataName(const char *name, std::array<Type *, N> &data)
   {
      registerSymbol(name, kernel::makeData(&data[0], N));
   }

private:
   static HleSymbolMap &
   getStaticSymbolMap()
   {
      static HleSymbolMap sSymbol;
      return sSymbol;
   }

   static void
   registerExportedSymbol(const char *name, HleSymbol *exp)
   {
      auto &symbols = getStaticSymbolMap();
      decaf_check(symbols.find(name) == symbols.end());

      exp->name = name;
      exp->exported = true;
      symbols.emplace(exp->name, exp);
   }

   static void
   registerSymbol(const char *name, HleSymbol *exp)
   {
      auto &symbols = getStaticSymbolMap();
      decaf_check(symbols.find(name) == symbols.end());

      exp->name = name;
      exp->exported = false;
      symbols.emplace(exp->name, exp);
   }
};

} // namespace kernel

#ifdef _MSC_VER
#define PRETTY_FUNCTION_NAME __FUNCSIG__
#else
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#define decaf_warn_stub() \
   { \
      static bool warned = false; \
      if (!warned) { \
         gLog->warn("Application invoked stubbed function `{}`", PRETTY_FUNCTION_NAME); \
         warned = true; \
      } \
   }
