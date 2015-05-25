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
   auto pos = name.find(".rpl");

   if (pos == std::string::npos) {
      name += ".rpl";
   }

   auto itr = mModules.find(name);

   if (itr == mModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

