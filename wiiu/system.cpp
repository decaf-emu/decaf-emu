#include <algorithm>
#include "instructiondata.h"
#include "system.h"
#include "systemmodule.h"
#include "systemthread.h"
#include "modules/coreinit/coreinit_memory.h"

System gSystem;

void
System::registerModule(std::string name, SystemModule *module)
{
   mModules.insert(std::make_pair(name, module));

   // Map syscall id
   for (auto &itr : module->mExport) {
      auto exp = itr.second;

      if (exp->type == SystemExport::Function) {
         auto func = reinterpret_cast<SystemFunction*>(exp);
         func->syscallID = static_cast<uint32_t>(mSystemCalls.size());
         mSystemCalls.push_back(func);
      }
   }
}

void
System::initialiseModules()
{
   for (auto &mpair : mModules) {
      mpair.second->initialise();
   }
}

SystemModule *
System::findModule(std::string name)
{
   auto itr = mModules.find(name);

   if (itr == mModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

void
System::addThread(SystemThread *thread)
{
   mThreads.push_back(thread);
}

void
System::removeThread(SystemThread *thread)
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
   // Allocate space for all thunks!
   auto addr = OSAllocFromSystem(static_cast<uint32_t>(mSystemCalls.size() * 8), 4).value;

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
