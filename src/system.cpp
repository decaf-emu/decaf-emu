#include <algorithm>
#include <functional>
#include "cpu/cpu.h"
#include "cpu/mem.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "system.h"
#include "common/teenyheap.h"
#include "common/strutils.h"

System
gSystem;

void
System::initialise()
{
   mSystemHeap = new TeenyHeap(mem::translate(mem::SystemBase), mem::SystemSize);
}

void
System::setFileSystem(fs::FileSystem *fs)
{
   mFileSystem = fs;
}

void
System::setUserModule(LoadedModule *module)
{
   mUserModule = module;
}

const LoadedModule *
System::getUserModule() const
{
   return mUserModule;
}

fs::FileSystem *
System::getFileSystem() const
{
   return mFileSystem;
}
