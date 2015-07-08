#include <algorithm>
#include "instructiondata.h"
#include "system.h"
#include "kernelmodule.h"
#include "modules/coreinit/coreinit_memheap.h"

System gSystem;

System::System()
{
}

void
System::registerModule(const char *name, KernelModule *module)
{
   mSystemModules.insert(std::make_pair(std::move(name), module));

   // Map syscall IDs
   auto exports = module->getExportMap();

   for (auto &itr : exports) {
      auto exp = itr.second;

      if (exp->type == KernelExport::Function) {
         auto func = reinterpret_cast<KernelFunction*>(exp);
         func->syscallID = static_cast<uint32_t>(mSystemCalls.size());
         mSystemCalls.push_back(func);
      }
   }
}

void
System::registerModule(const char *name, UserModule *module)
{
   mUserModule = module;
}

UserModule *
System::getUserModule() const
{
   return mUserModule;
}

void
System::initialiseModules()
{
   CoreInitSystemHeap();
   loadThunks();

   for (auto &pair : mSystemModules) {
      pair.second->initialise();
   }
}

KernelModule *
System::findModule(const char *name) const
{
   auto itr = mSystemModules.find(name);

   if (itr == mSystemModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

KernelFunction *
System::getSyscall(uint32_t id)
{
   return mSystemCalls.at(id);
}

void
System::loadThunks()
{
   uint32_t addr;

   // Allocate space for all thunks!
   mSystemThunks = OSAllocFromSystem(static_cast<uint32_t>(mSystemCalls.size() * 8), 4);
   addr = gMemory.untranslate(mSystemThunks);

   for (auto &func : mSystemCalls) {
      // Set the function virtual address
      func->vaddr = addr;

      // Write syscall with symbol id
      auto kc = gInstructionTable.encode(InstructionID::kc);
      kc.li = func->syscallID;
      kc.aa = 1;
      gMemory.write(addr, kc.value);

      // Return by Branch to LR
      auto bclr = gInstructionTable.encode(InstructionID::bclr);
      bclr.bo = 0x1f;
      gMemory.write(addr + 4, bclr.value);

      addr += 8;
   }
}

void
System::setFileSystem(FileSystem *fs)
{
   mFileSystem = fs;
}

FileSystem *System::getFileSystem()
{
   return mFileSystem;
}
