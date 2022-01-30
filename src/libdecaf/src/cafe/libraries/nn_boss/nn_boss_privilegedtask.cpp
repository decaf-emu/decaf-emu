#include "nn_boss.h"
#include "nn_boss_privilegedtask.h"
#include "nn_boss_task.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> PrivilegedTask::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> PrivilegedTask::TypeDescriptor = nullptr;

virt_ptr<PrivilegedTask>
PrivilegedTask_Constructor(virt_ptr<PrivilegedTask> self)
{
   if (!self) {
      self = virt_cast<PrivilegedTask *>(ghs::malloc(sizeof(PrivilegedTask)));
      if (!self) {
         return nullptr;
      }
   }

   Task_Constructor(virt_cast<Task *>(self));
   self->virtualTable = PrivilegedTask::VirtualTable;
   return self;
}

virt_ptr<PrivilegedTask>
PrivilegedTask_Constructor(virt_ptr<PrivilegedTask> self,
                           virt_ptr<const char> taskId,
                           uint32_t accountId)
{
   if (!self) {
      self = virt_cast<PrivilegedTask *>(ghs::malloc(sizeof(PrivilegedTask)));
      if (!self) {
         return nullptr;
      }
   }

   Task_Constructor(virt_cast<Task *>(self), taskId, accountId);
   self->virtualTable = PrivilegedTask::VirtualTable;
   return self;
}

void
PrivilegedTask_Destructor(virt_ptr<PrivilegedTask> self,
                          ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   self->virtualTable = PrivilegedTask::VirtualTable;
   Task_Destructor(virt_cast<Task *>(self), ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

void
Library::registerPrivilegedTaskSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss14PrivilegedTaskFv",
                              static_cast<virt_ptr<PrivilegedTask> (*)(virt_ptr<PrivilegedTask>)>(PrivilegedTask_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss14PrivilegedTaskFPCcUi",
                              static_cast<virt_ptr<PrivilegedTask>(*)(virt_ptr<PrivilegedTask>, virt_ptr<const char>, uint32_t)>(PrivilegedTask_Constructor));
   RegisterFunctionExportName("__dt__Q3_2nn4boss14PrivilegedTaskFv",
                              PrivilegedTask_Destructor);

   RegisterTypeInfo(
      PrivilegedTask,
      "nn::boss::PrivilegedTask",
      {
         "__dt__Q3_2nn4boss14PrivilegedTaskFv",
      },
      {});
}

} // namespace cafe::nn_boss
