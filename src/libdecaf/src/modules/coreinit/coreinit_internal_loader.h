#pragma once
#include <vector>
#include <map>
#include "common/types.h"
#include "common/emuassert.h"
#include "ppcutils/wfunc_ptr.h"

namespace coreinit
{

namespace internal
{

struct LoadedModule;

struct LoadedModuleHandleData
{
   LoadedModule *ptr;
};

struct LoadedSection
{
   std::string name;
   ppcaddr_t start;
   ppcaddr_t end;
};

struct LoadedModule
{
   ppcaddr_t findExport(const std::string& name) const
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
   Type *findDataExport(const std::string& name) const
   {
      return reinterpret_cast<Type *>(mem::translate(findExport(name)));
   }

   std::string name;
   LoadedModuleHandleData *handle = nullptr;
   ppcaddr_t entryPoint = 0;
   ppcsize_t defaultStackSize = 0;
   ppcaddr_t sdaBase = 0;
   ppcaddr_t sda2Base = 0;
   std::vector<LoadedSection> sections;
   std::map<std::string, ppcaddr_t> exports;
   std::map<std::string, ppcaddr_t> symbols;
};

void lockLoader();
void unlockLoader();

LoadedModule * loadRPX(ppcsize_t maxCodeSize, const std::string& name);
LoadedModule * loadRPL(const std::string& name);

LoadedModule * findModule(const std::string& name);
LoadedModule * getUserModule();

std::map<std::string, LoadedModule*> getLoadedModules();

} // namespace internal

} // namespace coreinit
