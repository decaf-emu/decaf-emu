#include "system.h"

System gSystem;

void
System::registerModule(std::string name, SystemModule *module)
{
   mModules.insert(std::make_pair(name, module));
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

