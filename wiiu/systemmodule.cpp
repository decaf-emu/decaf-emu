#include "systemmodule.h"

void SystemModule::registerExport(const char *name, SystemExport *exp)
{
   exp->name = name;
   mExport.insert(std::make_pair(std::string(name), exp));
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

uint32_t
SystemModule::findExportAddress(const std::string &name)
{
   auto exp = findExport(name);

   if (exp) {
      if (exp->type == SystemExport::Function) {
         return reinterpret_cast<SystemFunction*>(exp)->vaddr;
      } else if (exp->type == SystemExport::Data) {
         return (*reinterpret_cast<SystemData*>(exp)->vptr).value;
      }
   }

   return 0;
}
