#pragma once
#include "systemmodule.h"

class System
{
public:
   void registerModule(std::string name, SystemModule *module);
   SystemModule *findModule(std::string name);

   void addThread(SystemThread *thread);
   void removeThread(SystemThread *thread);

private:
   std::vector<SystemThread*> mThreads;
   std::map<std::string, SystemModule*> mModules;
};

extern System gSystem;
