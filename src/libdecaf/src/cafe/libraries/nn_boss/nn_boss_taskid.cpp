#include "nn_boss.h"
#include "nn_boss_taskid.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include <common/strutils.h>

namespace cafe::nn_boss
{

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self)
{
   if (!self) {
      self = virt_cast<TaskID *>(ghs::malloc(sizeof(TaskID)));
      if (!self) {
         return nullptr;
      }
   }

   self->value[0] = char { 0 };
   return self;
}

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self,
                   virt_ptr<const char> id)
{
   if (!self) {
      self = virt_cast<TaskID *>(ghs::malloc(sizeof(TaskID)));
      if (!self) {
         return nullptr;
      }
   }

   string_copy(virt_addrof(self->value).get(), self->value.size(), id.get(), 8);
   self->value[7] = char { 0 };
   return self;
}

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self,
                   virt_ptr<TaskID> other)
{
   if (!self) {
      self = virt_cast<TaskID *>(ghs::malloc(sizeof(TaskID)));
      if (!self) {
         return nullptr;
      }
   }

   std::memcpy(virt_addrof(self->value).get(),
               virt_addrof(other->value).get(),
               8);
   return self;
}

virt_ptr<TaskID>
TaskID_OperatorAssign(virt_ptr<TaskID> self,
                      virt_ptr<const char> id)
{
   string_copy(virt_addrof(self->value).get(), self->value.size(), id.get(), 8);
   self->value[7] = char { 0 };
   return self;
}

bool
TaskID_OperatorEqual(virt_ptr<TaskID> self,
                     virt_ptr<const char> id)
{
   return std::strncmp(virt_addrof(self->value).get(), id.get(), 8) == 0;
}

bool
TaskID_OperatorEqual(virt_ptr<TaskID> self,
                     virt_ptr<TaskID> other)
{
   return std::strncmp(virt_addrof(self->value).get(),
                       virt_addrof(other->value).get(),
                       8) == 0;
}

bool
TaskID_OperatorNotEqual(virt_ptr<TaskID> self,
                        virt_ptr<const char> id)
{
   return !TaskID_OperatorEqual(self, id);
}

bool
TaskID_OperatorNotEqual(virt_ptr<TaskID> self,
                        virt_ptr<TaskID> other)
{
   return !TaskID_OperatorEqual(self, other);
}

virt_ptr<const char>
TaskID_OperatorCastConstCharPtr(virt_ptr<TaskID> self)
{
   return virt_addrof(self->value);
}

void
Library::registerTaskIdSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss6TaskIDFv",
                              static_cast<virt_ptr<TaskID> (*)(virt_ptr<TaskID>)>(TaskID_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss6TaskIDFPCc",
                              static_cast<virt_ptr<TaskID>(*)(virt_ptr<TaskID>, virt_ptr<const char>)>(TaskID_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss6TaskIDFRCQ3_2nn4boss6TaskID",
                              static_cast<virt_ptr<TaskID>(*)(virt_ptr<TaskID>, virt_ptr<TaskID>)>(TaskID_Constructor));
   RegisterFunctionExportName("__as__Q3_2nn4boss6TaskIDFPCc",
                              static_cast<virt_ptr<TaskID> (*)(virt_ptr<TaskID>, virt_ptr<const char>)>(TaskID_OperatorAssign));
   RegisterFunctionExportName("__eq__Q3_2nn4boss6TaskIDCFPCc",
                              static_cast<bool (*)(virt_ptr<TaskID>, virt_ptr<const char>)>(TaskID_OperatorEqual));
   RegisterFunctionExportName("__eq__Q3_2nn4boss6TaskIDCFRCQ3_2nn4boss6TaskID",
                              static_cast<bool (*)(virt_ptr<TaskID>, virt_ptr<TaskID>)>(TaskID_OperatorEqual));
   RegisterFunctionExportName("__ne__Q3_2nn4boss6TaskIDCFPCc",
                              static_cast<bool (*)(virt_ptr<TaskID>, virt_ptr<const char>)>(TaskID_OperatorNotEqual));
   RegisterFunctionExportName("__ne__Q3_2nn4boss6TaskIDCFRCQ3_2nn4boss6TaskID",
                              static_cast<bool(*)(virt_ptr<TaskID>, virt_ptr<TaskID>)>(TaskID_OperatorNotEqual));
   RegisterFunctionExportName("__opPCc__Q3_2nn4boss6TaskIDCFv",
                              TaskID_OperatorCastConstCharPtr);
}

} // namespace cafe::nn_boss
