#include "systemmodule.h"

void SystemModule::registerFunction(std::string name, SystemExport *exp)
{
   mExport.insert(std::make_pair(name, exp));
}

SystemExport *
SystemModule::findExport(const std::string &name)
{
   auto itr = mExport.find(name);

   if (itr == mExport.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}
