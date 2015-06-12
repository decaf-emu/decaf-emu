#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include "heapmanager.h"
#include "systemtypes.h"

class Memory;
class Thread;
class SystemModule;
struct UserModule;
struct SystemFunction;

class System
{
public:
   System();

   void initialiseModules();

   void registerModule(const char *name, SystemModule *module);
   SystemModule *findModule(const char *name) const;

   void registerModule(const char *name, UserModule *module);
   UserModule *getUserModule() const;

   void addThread(Thread *thread);
   void removeThread(Thread *thread);

   SystemFunction *getSyscall(uint32_t id);

   WHeapHandle addHeap(HeapManager *heap);
   HeapManager *getHeap(WHeapHandle handle);
   HeapManager *getHeapByAddress(uint32_t vaddr);
   void removeHeap(WHeapHandle handle);

protected:
   void loadThunks();

private:
   UserModule *mUserModule;
   std::map<std::string, SystemModule*> mSystemModules;
   p32<void> mSystemThunks;
   std::vector<SystemFunction*> mSystemCalls;

   std::vector<HeapManager *> mHeaps;
   std::vector<Thread*> mThreads;
};

extern System gSystem;
