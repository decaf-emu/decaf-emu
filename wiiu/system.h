#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>

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

   void loadThunks();
private:
   std::vector<SystemThread*> mThreads;
   std::map<std::string, SystemModule*> mModules;
   std::vector<SystemFunction*> mSystemCalls;
};

extern System gSystem;
