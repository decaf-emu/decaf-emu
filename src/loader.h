#pragma once
#include <array_view.h>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "elf.h"
#include "memory_translate.h"
#include "types.h"
#include "utils/wfunc_ptr.h"

class KernelModule;
class TeenyHeap;
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
   ppcaddr_t findExport(const std::string& name)
   {
      auto itr = exports.find(name);

      if (itr == exports.end()) {
         return 0u;
      }

      return itr->second;
   }

   template<typename ReturnType, typename... Args>
   wfunc_ptr<ReturnType, Args...>
   findFuncExport(const std::string& name)
   {
      return wfunc_ptr<ReturnType, Args...>(findExport(name));
   }

   template<typename Type>
   Type *findDataExport(const std::string& name)
   {
      return reinterpret_cast<Type *>(memory_translate(findExport(name)));
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

class SequentialMemoryTracker;
using ModuleList = std::map<std::string, std::unique_ptr<LoadedModule>>;
using SectionList = std::vector<elf::XSection>;
using AddressRange = std::pair<ppcaddr_t, ppcaddr_t>;

class Loader
{
public:
   void initialise(ppcsize_t maxCodeSize);
   LoadedModule *loadRPL(std::string name);

   const char *getUnimplementedData(ppcaddr_t addr);

   const ModuleList &getLoadedModules() const
   {
      return mModules;
   }

private:
   ppcaddr_t registerUnimplementedData(const std::string& name);
   ppcaddr_t registerUnimplementedFunction(const std::string& name);

   std::unique_ptr<LoadedModule> loadKernelModule(const std::string &name, KernelModule *module);
   std::unique_ptr<LoadedModule> loadRPL(const std::string& name, const gsl::array_view<uint8_t> &data);

   bool processImports(LoadedModule *loadedMod, const SectionList &sections);
   bool processExports(LoadedModule *loadedMod, const SectionList &sections);
   bool processRelocations(LoadedModule *loadedMod, const SectionList &sections, BigEndianView &in, const char *shStrTab, SequentialMemoryTracker &codeSeg, AddressRange &trampSeg);

private:
   ModuleList mModules;
   std::map<std::string, ppcaddr_t> mUnimplementedFunctions;
   std::map<std::string, int> mUnimplementedData;
   std::unique_ptr<TeenyHeap> mCodeHeap;
};

extern Loader gLoader;
