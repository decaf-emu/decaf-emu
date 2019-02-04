#include "nn_boss.h"
#include "nn_boss_task.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "cafe/libraries/nn_act/nn_act_lib.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> Task::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> Task::TypeDescriptor = nullptr;

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self)
{
   if (!self) {
      self = virt_cast<Task *>(ghs::malloc(sizeof(Task)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = Task::VirtualTable;
   TaskID_Constructor(virt_addrof(self->taskId));
   TitleID_Constructor(virt_addrof(self->titleId));
   std::memset(virt_addrof(self->taskId).get(), 0, sizeof(TaskID));
   return self;
}

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 virt_ptr<const char> taskId)
{
   self = Task_Constructor(self);
   if (self) {
      Task_Initialize(self, taskId);
   }

   return self;
}

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 virt_ptr<const char> taskId,
                 uint32_t accountId)
{
   self = Task_Constructor(self);
   if (self) {
      Task_Initialize(self, taskId, accountId);
   }

   return self;
}

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 uint8_t slot,
                 virt_ptr<const char> taskId)
{
   self = Task_Constructor(self);
   if (self) {
      Task_Initialize(self, slot, taskId);
   }

   return self;
}

void
Task_Destructor(virt_ptr<Task> self,
                ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   self->virtualTable = Task::VirtualTable;
   Task_Finalize(self);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
Task_Initialize(virt_ptr<Task> self,
                virt_ptr<const char> taskId)
{
   return Task_Initialize(self, taskId, 0u);
}

nn::Result
Task_Initialize(virt_ptr<Task> self,
                virt_ptr<const char> taskId,
                uint32_t accountId)
{
   if (!taskId || strnlen(taskId.get(), 8) == 8) {
      return ResultInvalidParameter;
   }

   self->accountId = accountId;
   TaskID_OperatorAssign(virt_addrof(self->taskId), taskId);
   return ResultSuccess;
}

nn::Result
Task_Initialize(virt_ptr<Task> self,
                uint8_t slot,
                virt_ptr<const char> taskId)
{
   if (!slot) {
      return Task_Initialize(self, taskId, 0u);
   } else if (auto accountId = nn_act::GetPersistentIdEx(slot)) {
      return Task_Initialize(self, taskId, accountId);
   }

   return ResultInvalidParameter;
}

void
Task_Finalize(virt_ptr<Task> self)
{
   decaf_warn_stub();
   TitleID_Constructor(virt_addrof(self->titleId));
}

bool
Task_IsRegistered(virt_ptr<Task> self)
{
   decaf_warn_stub();
   return false;
}

uint32_t
Task_GetAccountID(virt_ptr<Task> self)
{
   return self->accountId;
}

void
Task_GetTaskID(virt_ptr<Task> self,
               virt_ptr<TaskID> id)
{
   *id = self->taskId;
}

void
Task_GetTitleID(virt_ptr<Task> self,
                virt_ptr<TitleID> id)
{
   *id = self->titleId;
}

void
Library::registerTaskSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss4TaskFv",
                              static_cast<virt_ptr<Task> (*)(virt_ptr<Task>)>(Task_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss4TaskFPCc",
                              static_cast<virt_ptr<Task>(*)(virt_ptr<Task>, virt_ptr<const char>)>(Task_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss4TaskFPCcUi",
                              static_cast<virt_ptr<Task>(*)(virt_ptr<Task>, virt_ptr<const char>, uint32_t)>(Task_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss4TaskFUcPCc",
                              static_cast<virt_ptr<Task>(*)(virt_ptr<Task>, uint8_t, virt_ptr<const char>)>(Task_Constructor));
   RegisterFunctionExportName("__dt__Q3_2nn4boss4TaskFv",
                              Task_Destructor);

   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFPCc",
                              static_cast<nn::Result (*)(virt_ptr<Task>, virt_ptr<const char>)>(Task_Initialize));
   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFPCcUi",
                              static_cast<nn::Result (*)(virt_ptr<Task>, virt_ptr<const char>, uint32_t)>(Task_Initialize));
   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFUcPCc",
                              static_cast<nn::Result (*)(virt_ptr<Task>, uint8_t, virt_ptr<const char>)>(Task_Initialize));
   RegisterFunctionExportName("Finalize__Q3_2nn4boss4TaskFv",
                              Task_Finalize);

   RegisterFunctionExportName("IsRegistered__Q3_2nn4boss4TaskCFv",
                              Task_IsRegistered);
   RegisterFunctionExportName("GetAccountID__Q3_2nn4boss4TaskCFv",
                              Task_GetAccountID);
   RegisterFunctionExportName("GetTaskID__Q3_2nn4boss4TaskCFv",
                              Task_GetTaskID);
   RegisterFunctionExportName("GetTitleID__Q3_2nn4boss4TaskCFv",
                              Task_GetTitleID);

   registerTypeInfo<Task>(
      "nn::boss::Task",
      {
         "__dt__Q3_2nn4boss4TaskFv",
      });
}

} // namespace cafe::nn_boss
