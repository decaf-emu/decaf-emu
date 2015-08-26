#pragma once
#include <string>
#include <vector>
#include <map>
#include "systemtypes.h"
#include "elf.h"

class KernelModule;
class TeenyHeap;

struct LoadedModuleHandleData
{
   class LoadedModule *ptr;
};

class LoadedModule
{
   friend class Loader;
public:
   LoadedModule()
   {
   }

   template<typename ReturnType, typename... Args>
   wfunc_ptr<ReturnType, Args...>
   findFuncExport(const std::string& name) {
      return wfunc_ptr<ReturnType, Args...>(findExport(name));
   }

   template<typename T>
   T* findDataExport(const std::string& name) {
      return static_cast<T*>(findExport(name));
   }

   LoadedModuleHandleData * getHandle() {
      return mHandle;
   }

   virtual std::string getName() {
      return mName;
   }

   virtual void* findExport(const std::string& name) {
      auto exportIter = mExports.find(name);
      if (exportIter == mExports.end()) {
         return nullptr;
      }
      return exportIter->second;
   }

   uint32_t entryPoint() const {
      return mEntryPoint;
   }

   uint32_t defaultStackSize() const {
      return mDefaultStackSize;
   }

   uint32_t sdaBase() const {
      return mSdaBase;
   }

   uint32_t sda2Base() const {
      return mSda2Base;
   }

protected:
   LoadedModuleHandleData *mHandle;
   std::string mName;
   std::map<std::string, void*> mExports;
   uint32_t mEntryPoint;
   uint32_t mDefaultStackSize;
   uint32_t mSdaBase;
   uint32_t mSda2Base;

};

class Loader
{
public:
   void initialise(ppcsize_t maxCodeSize);

   LoadedModule * loadRPL(const std::string& name);

   const char * getUnimplementedData(uint32_t addr);

protected:
   uint32_t registerUnimplementedData(const std::string& name);
   void * registerUnimplementedFunction(const std::string& name);
   LoadedModule * loadKernelModule(const std::string &name, KernelModule *module);
   LoadedModule * loadRPL(const std::string& name, const std::vector<uint8_t> data);

   std::map<std::string, void*> mUnimplementedFunctions;
   std::map<std::string, int> mUnimplementedData;
   std::map<std::string, LoadedModule*> mModules;
   TeenyHeap *mCodeHeap;

};

extern Loader gLoader;
