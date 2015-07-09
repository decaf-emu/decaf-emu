#pragma once
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include "systemtypes.h"

class Memory;
class Thread;
class KernelModule;
struct UserModule;
struct KernelFunction;
class FileSystem;

struct LoadedModule
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

   void initialiseModules();

   void registerModule(const char *name, KernelModule *module);
   KernelModule *findModule(const char *name) const;

   void registerModule(const char *name, UserModule *module);
   UserModule *getUserModule() const;

   KernelFunction *getSyscall(uint32_t id);

   FileSystem *getFileSystem();
   void setFileSystem(FileSystem *fs);

protected:
   void loadThunks();

private:
   UserModule *mUserModule;
   std::map<std::string, KernelModule*> mSystemModules;

   void *mSystemThunks;
   std::vector<KernelFunction*> mSystemCalls;

   FileSystem *mFileSystem;
};

extern System gSystem;
