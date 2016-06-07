#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class Memory;
class Thread;
class TeenyHeap;

namespace fs
{
class FileSystem;
}

namespace coreinit
{
namespace internal
{
struct LoadedModule;
}
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
   setFileSystem(fs::FileSystem *fs);

   fs::FileSystem *
   getFileSystem() const;

   TeenyHeap *getSystemHeap() const
   {
      return mSystemHeap;
   }

private:
   coreinit::internal::LoadedModule *mUserModule = nullptr;

   fs::FileSystem *mFileSystem = nullptr;
   TeenyHeap *mSystemHeap = nullptr;
};

extern System gSystem;
