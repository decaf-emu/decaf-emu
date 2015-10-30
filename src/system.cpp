#include <algorithm>
#include <functional>
#include "cpu/cpu.h"
#include "cpu/instructiondata.h"
#include "kernelfunction.h"
#include "kernelmodule.h"
#include "mem/mem.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "system.h"
#include "utils/teenyheap.h"
#include "utils/strutils.h"

System gSystem;

System::System()
{
}

static void
kcstub(ThreadState *state, void *userData)
{
   KernelFunction *func = static_cast<KernelFunction*> (userData);
   if (!func->valid) {
      gLog->info("unimplemented kernel function {}", func->name);
      return;
   }

   func->call(state);
}

void
System::registerSysCall(KernelFunction *func)
{
   cpu::KernelCallEntry entry(kcstub, func);
   func->syscallID = cpu::registerKernelCall(entry);
   mSystemCalls[func->syscallID] = func;
}

uint32_t
System::registerUnimplementedFunction(const std::string &name)
{
   auto ppcFn = new kernel::functions::KernelFunctionImpl<void>();
   ppcFn->valid = false;
   ppcFn->name = name;
   ppcFn->wrapped_function = nullptr;
   registerSysCall(ppcFn);
   return ppcFn->syscallID;
}

void
System::registerModule(const std::string &name, KernelModule *module)
{
   mSystemModules.emplace(name, module);

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
   void *systemMem = mem::translate(0x01000000);
   mSystemHeap = new TeenyHeap(systemMem, 0x01000000);
}

KernelModule *
System::findModule(const std::string &name) const
{
   auto itr = mSystemModules.find(name);

   if (itr == mSystemModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

KernelFunction *
System::getSyscallData(uint32_t id)
{
   return mSystemCalls[id];
}

void
System::loadThunks()
{
   uint32_t addr;

   // Allocate space for all thunks!
   mSystemThunks = OSAllocFromSystem(static_cast<uint32_t>(mSystemCalls.size() * 8), 4);
   addr = mem::untranslate(mSystemThunks);

   for (auto &i : mSystemCalls) {
      auto &func = i.second;

      // Set the function virtual address
      func->vaddr = addr;

      // Write syscall with symbol id
      auto kc = gInstructionTable.encode(InstructionID::kc);
      kc.li = func->syscallID;
      kc.aa = 1;
      mem::write(addr, kc.value);

      // Return by Branch to LR
      auto bclr = gInstructionTable.encode(InstructionID::bclr);
      bclr.bo = 0x1f;
      mem::write(addr + 4, bclr.value);

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
