#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "loader.h"

class Memory;
class Thread;
class KernelModule;
struct KernelFunction;
class TeenyHeap;

namespace fs
{
class FileSystem;
}

struct ModuleHandleData
{
   enum Type
   {
      Unknown,
      User,
      Kernel
   };

   Type type;
   void *ptr;
};


/**
 * Contains the state of the emulator, including the filesystem, modules, exports etc.
 */

class System
{
public:
   void
   initialise();

   uint32_t
   registerUnimplementedFunction(const std::string &module, const std::string &name);

   void
   registerModule(const std::string &name, KernelModule *module);

   void
   registerModuleAlias(const std::string &module, const std::string &alias);

   KernelModule *
   findModule(const std::string &name) const;

   void
   setUserModule(LoadedModule *module);

   const LoadedModule *
   getUserModule() const;

   const KernelFunction *
   getSyscallData(const uint32_t id) const;

   void
   setFileSystem(fs::FileSystem *fs);

   fs::FileSystem *
   getFileSystem() const;

   TeenyHeap *getSystemHeap() const
   {
      return mSystemHeap;
   }

protected:
   void registerSysCall(KernelFunction *func);

private:
   LoadedModule *mUserModule = nullptr;
   std::map<std::string, KernelModule*> mSystemModules;

   std::map<uint32_t, KernelFunction*> mSystemCalls;

   fs::FileSystem *mFileSystem = nullptr;
   TeenyHeap *mSystemHeap = nullptr;
};

extern System gSystem;
