#include <algorithm>
#include "instructiondata.h"
#include "system.h"
#include "systemmodule.h"
#include "systemthread.h"

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
