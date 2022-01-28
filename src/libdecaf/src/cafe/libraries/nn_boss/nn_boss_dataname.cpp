#include "nn_boss.h"
#include "nn_boss_dataname.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "common/strutils.h"

namespace cafe::nn_boss
{

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self)
{
   if (!self) {
      self = virt_cast<DataName *>(ghs::malloc(sizeof(DataName)));
      if (!self) {
         return nullptr;
      }
   }

   self->name.fill('\0');
   return self;
}

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self,
                     virt_ptr<const char> name)
{
   if (!self) {
      self = virt_cast<DataName *>(ghs::malloc(sizeof(DataName)));
      if (!self) {
         return nullptr;
      }
   }

   string_copy(virt_addrof(self->name).getRawPointer(),
               name.getRawPointer(),
               self->name.size());
   self->name[31] = '\0';
   return self;
}

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self,
                     virt_ptr<const DataName> other)
{
   if (!self) {
      self = virt_cast<DataName *>(ghs::malloc(sizeof(DataName)));
      if (!self) {
         return nullptr;
      }
   }

   memcpy(self.get(), other.get(), sizeof(DataName));
   return self;
}

virt_ptr<const char>
DataName_OperatorCastConstCharPtr(virt_ptr<const DataName> self)
{
   return virt_addrof(self->name);
}

virt_ptr<DataName>
DataName_OperatorAssign(virt_ptr<DataName> self,
                        virt_ptr<const char> name)
{
   string_copy(virt_addrof(self->name).getRawPointer(),
               name.getRawPointer(),
               self->name.size());
   self->name[31] = '\0';
   return self;
}

bool
DataName_OperatorEqual(virt_ptr<const DataName> self,
                       virt_ptr<const DataName> other)
{
   return std::strncmp(virt_addrof(self->name).getRawPointer(),
                       virt_addrof(other->name).getRawPointer(),
                       31) == 0;
}

bool
DataName_OperatorEqual(virt_ptr<const DataName> self,
                       virt_ptr<const char> name)
{
   return std::strncmp(virt_addrof(self->name).getRawPointer(),
                       name.getRawPointer(),
                       31) == 0;
}

bool
DataName_OperatorNotEqual(virt_ptr<const DataName> self,
                          virt_ptr<const DataName> other)
{
   return !DataName_OperatorEqual(self, other);
}

bool
DataName_OperatorNotEqual(virt_ptr<const DataName> self,
                          virt_ptr<const char> name)
{
   return !DataName_OperatorEqual(self, name);
}

void
Library::registerDataNameSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss8DataNameFv",
                              static_cast<virt_ptr<DataName> (*)(virt_ptr<DataName>)>(DataName_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss8DataNameFPCc",
                              static_cast<virt_ptr<DataName> (*)(virt_ptr<DataName>, virt_ptr<const char>)>(DataName_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss8DataNameFRCQ3_2nn4boss8DataName",
                              static_cast<virt_ptr<DataName> (*)(virt_ptr<DataName>, virt_ptr<const DataName>)>(DataName_Constructor));

   RegisterFunctionExportName("__opPCc__Q3_2nn4boss8DataNameCFv",
                              DataName_OperatorCastConstCharPtr);

   RegisterFunctionExportName("__as__Q3_2nn4boss8DataNameFPCc",
                              DataName_OperatorAssign);

   RegisterFunctionExportName("__eq__Q3_2nn4boss8DataNameCFRCQ3_2nn4boss8DataName",
                              static_cast<bool(*)(virt_ptr<const DataName>, virt_ptr<const DataName>)>(DataName_OperatorEqual));
   RegisterFunctionExportName("__eq__Q3_2nn4boss8DataNameCFPCc",
                              static_cast<bool(*)(virt_ptr<const DataName>, virt_ptr<const char>)>(DataName_OperatorEqual));

   RegisterFunctionExportName("__ne__Q3_2nn4boss8DataNameCFRCQ3_2nn4boss8DataName",
                              static_cast<bool(*)(virt_ptr<const DataName>, virt_ptr<const DataName>)>(DataName_OperatorNotEqual));
   RegisterFunctionExportName("__ne__Q3_2nn4boss8DataNameCFPCc",
                              static_cast<bool(*)(virt_ptr<const DataName>, virt_ptr<const char>)>(DataName_OperatorNotEqual));
}

}  // namespace cafe::nn_boss
