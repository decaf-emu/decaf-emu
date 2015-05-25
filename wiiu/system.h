#pragma once
#include "systemmodule.h"

class System
{
public:
   void registerModule(std::string name, SystemModule *module);
   SystemModule *findModule(std::string name);

private:
   std::map<std::string, SystemModule*> mModules;
};

extern System gSystem;
