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
class TeenyHeap;

namespace fs
{
class FileSystem;
}

/**
 * Contains the state of the emulator, including the filesystem, modules, exports etc.
 */

class System
{
public:
   void
   initialise();

   void
   setUserModule(LoadedModule *module);

   const LoadedModule *
   getUserModule() const;

   void
   setFileSystem(fs::FileSystem *fs);

   fs::FileSystem *
   getFileSystem() const;

   TeenyHeap *getSystemHeap() const
   {
      return mSystemHeap;
   }

private:
   LoadedModule *mUserModule = nullptr;

   fs::FileSystem *mFileSystem = nullptr;
   TeenyHeap *mSystemHeap = nullptr;
};

extern System gSystem;
