#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include "heapmanager.h"

class Memory;
class SystemThread;
class SystemModule;
struct SystemFunction;

class System
{
public:
   void registerModule(std::string name, SystemModule *module);
   SystemModule *findModule(std::string name);

   void addThread(SystemThread *thread);
   void removeThread(SystemThread *thread);

   SystemFunction *getSyscall(uint32_t id);

   void initialiseModules();
   void loadThunks();

   WHeapHandle addHeap(HeapManager *heap);
   HeapManager *getHeap(WHeapHandle handle);
   HeapManager *getHeapByAddress(uint32_t vaddr);
   void removeHeap(WHeapHandle handle);

private:
   std::vector<SystemThread*> mThreads;
   std::map<std::string, SystemModule*> mModules;
   std::vector<SystemFunction*> mSystemCalls;

   Memory *mMemory;
   std::vector<HeapManager *> mHeaps;
};

extern System gSystem;
