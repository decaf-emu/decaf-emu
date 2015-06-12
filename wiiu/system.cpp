#include <algorithm>
#include "instructiondata.h"
#include "system.h"
#include "systemmodule.h"
#include "thread.h"
#include "modules/coreinit/coreinit_memory.h"

System gSystem;

System::System()
{
}

void
System::registerModule(const char *name, SystemModule *module)
{
   mSystemModules.insert(std::make_pair(std::move(name), module));

   // Map syscall IDs
   auto exports = module->getExportMap();

   for (auto &itr : exports) {
      auto exp = itr.second;

      if (exp->type == SystemExport::Function) {
         auto func = reinterpret_cast<SystemFunction*>(exp);
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
   loadThunks();

   for (auto &pair : mSystemModules) {
      pair.second->initialise();
   }
}

SystemModule *
System::findModule(const char *name) const
{
   auto itr = mSystemModules.find(name);

   if (itr == mSystemModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

void
System::addThread(Thread *thread)
{
   mThreads.push_back(thread);
}

void
System::removeThread(Thread *thread)
{
   mThreads.erase(std::remove(mThreads.begin(), mThreads.end(), thread), mThreads.end());
}

SystemFunction *
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
   addr = static_cast<uint32_t>(mSystemThunks);

   for (auto &func : mSystemCalls) {
      // Set the function virtual address
      func->vaddr = addr;

      // Write syscall with symbol id
      auto sc = gInstructionTable.encode(InstructionID::sc);
      sc.bd = func->syscallID;
      gMemory.write(addr, sc.value);

      // Return by Branch to LR
      auto bclr = gInstructionTable.encode(InstructionID::bclr);
      bclr.bo = 0x1f;
      gMemory.write(addr + 4, bclr.value);

      addr += 8;
   }
}

WHeapHandle
System::addHeap(HeapManager *heap)
{
   for (auto i = 0u; i < mHeaps.size(); ++i) {
      if (mHeaps[i] == nullptr) {
         mHeaps[i] = heap;
         return i + 1;
      }
   }

   mHeaps.push_back(heap);
   return static_cast<WHeapHandle>(mHeaps.size());
}

HeapManager *
System::getHeap(WHeapHandle handle)
{
   return mHeaps[handle - 1];
}

HeapManager *
System::getHeapByAddress(uint32_t vaddr)
{
   for (auto heap : mHeaps) {
      auto base = heap->getAddress();
      auto size = heap->getSize();

      if (vaddr >= base && vaddr < base + size) {
         return heap;
      }
   }

   return nullptr;
}

void
System::removeHeap(WHeapHandle handle)
{
   mHeaps[handle - 1] = nullptr;
}
