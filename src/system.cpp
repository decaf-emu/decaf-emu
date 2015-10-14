#include <algorithm>
#include "instructiondata.h"
#include "kernelfunction.h"
#include "kernelmodule.h"
#include "memory.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "system.h"
#include "teenyheap.h"

System gSystem;

System::System()
{
}

void
System::registerSysCall(KernelFunction *func)
{
   func->syscallID = static_cast<uint32_t>(mSystemCalls.size());
   mSystemCalls.push_back(func);
}

uint32_t
System::registerUnimplementedFunction(const char* name)
{
   auto ppcFn = new kernel::functions::KernelFunctionImpl<void>();
   ppcFn->name = _strdup(name);
   ppcFn->wrapped_function = nullptr;
   registerSysCall(ppcFn);
   return ppcFn->syscallID;
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
         registerSysCall(reinterpret_cast<KernelFunction*>(exp));
      }
   }
}

void
System::setUserModule(LoadedModule *module)
{
   mUserModule = module;
}

LoadedModule *
System::getUserModule() const
{
   return mUserModule;
}

void
System::initialise()
{
   auto systemHeapStart = 0x01000000u;
   auto systemHeapSize = 0x01000000u;
   gMemory.alloc(systemHeapStart, systemHeapSize);
   void *systemMem = gMemory.translate(systemHeapStart);
   mSystemHeap = new TeenyHeap(systemMem, systemHeapSize);
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
   if (id >= mSystemCalls.size()) {
      return nullptr;
   }

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
System::setFileSystem(fs::FileSystem *fs)
{
   mFileSystem = fs;
}

fs::FileSystem *System::getFileSystem()
{
   return mFileSystem;
}
