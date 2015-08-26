#pragma once
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include "systemtypes.h"
#include "loader.h"

class Memory;
class Thread;
class KernelModule;
struct KernelFunction;
class FileSystem;
class TeenyHeap;

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

   KernelFunction *getSyscall(uint32_t id);

   FileSystem *getFileSystem();
   void setFileSystem(FileSystem *fs);

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
   std::vector<KernelFunction*> mSystemCalls;

   FileSystem *mFileSystem;
   TeenyHeap *mSystemHeap;
};

extern System gSystem;
