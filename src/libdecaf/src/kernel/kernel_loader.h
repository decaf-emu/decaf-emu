#pragma once
#include <vector>
#include <map>
#include "common/types.h"
#include "common/decaf_assert.h"
#include "ppcutils/wfunc_ptr.h"

namespace kernel
{

namespace loader
{

// int AppEntryPoint(uint32_t argc, const char *argv);
using AppEntryPoint = wfunc_ptr<int, uint32_t, void*>;

// int RplEntryPoint(void *moduleHandle, int reason);
using RplEntryPoint = wfunc_ptr<int, void*, int>;

// Reason codes for RPL entrypoint invocation
static const int RplEntryReasonLoad = 1;
static const int RplEntryReasonUnload = 2;

struct LoadedModule;

struct LoadedModuleHandleData
{
   LoadedModule *ptr;
};

enum class LoadedSectionType
{
   Unknown,
   Code,
   Data
};

struct LoadedSection
{
   std::string name;
   LoadedSectionType type;
   ppcaddr_t start;
   ppcaddr_t end;
};

struct LoadedModule
{
   ppcaddr_t
      findExport(const std::string& name) const
   {
      auto itr = exports.find(name);

      if (itr == exports.end()) {
         return 0u;
      }

      return itr->second;
   }

   template<typename ReturnType, typename... Args>
   wfunc_ptr<ReturnType, Args...>
      findFuncExport(const std::string& name) const
   {
      return wfunc_ptr<ReturnType, Args...>(findExport(name));
   }

   template<typename Type>
   Type *
      findDataExport(const std::string& name) const
   {
      return reinterpret_cast<Type *>(mem::translate(findExport(name)));
   }

   std::string name;
   LoadedModuleHandleData *handle = nullptr;
   ppcaddr_t entryPoint = 0;
   ppcsize_t defaultStackSize = 0;
   ppcaddr_t sdaBase = 0;
   ppcaddr_t sda2Base = 0;
   uint32_t tlsModuleIndex = 0;
   ppcaddr_t tlsBase = 0;
   ppcsize_t tlsAlignShift = 0;
   ppcsize_t tlsSize = 0;
   std::vector<LoadedSection> sections;
   std::map<std::string, ppcaddr_t> exports;
   std::map<std::string, ppcaddr_t> symbols;
};

void
lockLoader();

void
unlockLoader();

LoadedModule *
loadRPL(const std::string& name);

LoadedModule *
findModule(const std::string& name);

LoadedSection *
findSectionForAddress(ppcaddr_t address);

std::string *
findSymbolNameForAddress(ppcaddr_t address);

std::string
findNearestSymbolNameForAddress(ppcaddr_t address);

std::map<std::string, LoadedModule*>
getLoadedModules();

} // namespace loader

} // namespace kernel
