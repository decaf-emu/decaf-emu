#include <algorithm>
#include <functional>
#include "cpu/cpu.h"
#include "kernelfunction.h"
#include "kernelmodule.h"
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


/**
 * Register a kernel module by name
 */
void
System::registerModule(const std::string &name, KernelModule *module)
{
   mSystemModules.emplace(name, module);

   // Map syscall IDs
   auto exports = module->getExportMap();

   if (exports.empty()) {
      gLog->warn("Module {} has no exports", name);
   }

   for (auto &itr : exports) {
      auto exp = itr.second;

      if (exp->type == KernelExport::Function) {
         registerSysCall(reinterpret_cast<KernelFunction*>(exp));
      }
   }
}


/**
 * Register an alternative name for a kernel module
 */
void
System::registerModuleAlias(const std::string &module, const std::string &alias)
{
   auto itr = mSystemModules.find(module);

   if (itr != mSystemModules.end()) {
      mSystemModules.emplace(alias, itr->second);
   }
}


/**
 * Find a kernel module by name
 */
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


/**
 * Forwarder function which PPC will branch to for a kernel library function call
 */
static void
kcstub(cpu::Core *state, void *data)
{
   auto func = static_cast<KernelFunction *>(data);

   if (!func->valid) {
      gLog->info("Unimplemented kernel function {}::{} called from 0x{:08X}", func->module, func->name, state->lr);
      return;
   }

   func->call(state);
}


/**
 * Register a kernel call
 */
void
System::registerSysCall(KernelFunction *func)
{
   func->syscallID = cpu::register_kernel_call({ kcstub, func });
   mSystemCalls[func->syscallID] = func;
}


/**
 * Register an unimplemented function
 */
uint32_t
System::registerUnimplementedFunction(const std::string &module, const std::string &name)
{
   auto ppcFn = new kernel::functions::KernelFunctionImpl<void>();
   ppcFn->valid = false;
   ppcFn->module = module;
   ppcFn->name = name;
   ppcFn->wrapped_function = nullptr;
   registerSysCall(ppcFn);
   return ppcFn->syscallID;
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

const KernelFunction *
System::getSyscallData(const uint32_t id) const
{
   return mSystemCalls.at(id);
}

fs::FileSystem *
System::getFileSystem() const
{
   return mFileSystem;
}
