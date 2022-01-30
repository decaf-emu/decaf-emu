#include "nn_boss.h"
#include "nn_boss_almightytask.h"
#include "nn_boss_privilegedtask.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/boss/nn_boss_result.h"

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> AlmightyTask::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> AlmightyTask::TypeDescriptor = nullptr;

virt_ptr<AlmightyTask>
AlmightyTask_Constructor(virt_ptr<AlmightyTask> self)
{
   if (!self) {
      self = virt_cast<AlmightyTask *>(ghs::malloc(sizeof(AlmightyTask)));
      if (!self) {
         return nullptr;
      }
   }

   PrivilegedTask_Constructor(virt_cast<PrivilegedTask *>(self));
   self->virtualTable = AlmightyTask::VirtualTable;
   return self;
}

void
AlmightyTask_Destructor(virt_ptr<AlmightyTask> self,
                        ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   self->virtualTable = AlmightyTask::VirtualTable;
   PrivilegedTask_Destructor(virt_cast<PrivilegedTask *>(self),
                             ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
AlmightyTask_Initialize(virt_ptr<AlmightyTask> self,
                        virt_ptr<TitleID> titleId,
                        virt_ptr<const char> taskId,
                        uint32_t accountId)
{
   if (!taskId) {
      return nn::boss::ResultInvalidParameter;
   }

   self->titleId = *titleId;
   TaskID_OperatorAssign(virt_addrof(self->taskId), taskId);
   self->accountId = accountId;
   return nn::boss::ResultSuccess;
}

void
Library::registerAlmightyTaskSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss12AlmightyTaskFv",
                              AlmightyTask_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss12AlmightyTaskFv",
                              AlmightyTask_Destructor);

   RegisterFunctionExportName("Initialize__Q3_2nn4boss12AlmightyTaskFQ3_2nn4boss7TitleIDPCcUi",
                              AlmightyTask_Initialize);

   RegisterTypeInfo(
      AlmightyTask,
      "nn::boss::AlmightyTask",
      {
         "__dt__Q3_2nn4boss12AlmightyTaskFv",
      },
      {});
}

} // namespace cafe::nn_boss
