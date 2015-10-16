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

class System
{
public:
   System();

   void initialise();

   uint32_t registerUnimplementedFunction(const char* name);
   void registerModule(const char *name, KernelModule *module);
   KernelModule *findModule(const char *name) const;

   void setUserModule(LoadedModule *module);
   LoadedModule *getUserModule() const;

   KernelFunction *getSyscallData(uint32_t id);

   fs::FileSystem *getFileSystem();
   void setFileSystem(fs::FileSystem *fs);

   TeenyHeap *getSystemHeap() const {
      return mSystemHeap;
   }

protected:
   void registerSysCall(KernelFunction *func);
   void loadThunks();

private:
   LoadedModule *mUserModule;
   std::map<std::string, KernelModule*> mSystemModules;

   void *mSystemThunks;
   std::map<uint32_t, KernelFunction*> mSystemCalls;

   fs::FileSystem *mFileSystem;
   TeenyHeap *mSystemHeap;
};

extern System gSystem;
