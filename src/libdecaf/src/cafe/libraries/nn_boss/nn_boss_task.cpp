#include "nn_boss.h"
#include "nn_boss_result.h"
#include "nn_boss_task.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/nn_act/nn_act_lib.h"

namespace cafe::nn::boss
{

virt_ptr<hle::VirtualTable> Task::VirtualTable = nullptr;
virt_ptr<hle::TypeDescriptor> Task::TypeDescriptor = nullptr;

Task::Task() :
   mAccountId(0u),
   mTitleId(0ull)
{
   mVirtualTable = Task::VirtualTable;
}

Task::Task(virt_ptr<const char> taskId)
{
   mVirtualTable = Task::VirtualTable;
   Initialize(taskId);
}

Task::Task(virt_ptr<const char> taskId,
           uint32_t accountId)
{
   mVirtualTable = Task::VirtualTable;
   Initialize(taskId, accountId);
}

Task::Task(uint8_t slot,
           virt_ptr<const char> taskId)
{
   mVirtualTable = Task::VirtualTable;
   Initialize(slot, taskId);
}

Task::~Task()
{
   mVirtualTable = Task::VirtualTable;
   Finalize();
}

nn::Result
Task::Initialize(virt_ptr<const char> taskId)
{
   return Initialize(taskId, 0u);
}

nn::Result
Task::Initialize(virt_ptr<const char> taskId,
                 uint32_t accountId)
{
   if (!taskId || strnlen(taskId.getRawPointer(), 8) == 8) {
      return InvalidParameter;
   }

   mAccountId = accountId;
   mTaskId = taskId;
   return Success;
}

nn::Result
Task::Initialize(uint8_t slot,
                 virt_ptr<const char> taskId)
{
   if (!slot) {
      return Initialize(taskId, 0u);
   } else if (auto accountId = nn::act::GetPersistentIdEx(slot)) {
      return Initialize(taskId, accountId);
   } else {
      return InvalidParameter;
   }
}

void
Task::Finalize()
{
   mTitleId = TitleID { };
   mTaskId = TaskID { };
}

bool
Task::IsRegistered()
{
   decaf_warn_stub();
   return false;
}

uint32_t
Task::GetAccountID()
{
   return mAccountId;
}

void
Task::GetTaskID(virt_ptr<TaskID> id)
{
   *id = mTaskId;
}

void
Task::GetTitleID(virt_ptr<TitleID> id)
{
   *id = mTitleId;
}

void
Library::registerTaskSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss4TaskFv",
                             Task);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss4TaskFPCc",
                                 Task, virt_ptr<const char>);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss4TaskFPCcUi",
                                 Task, virt_ptr<const char>, uint32_t);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss4TaskFUcPCc",
                                 Task, uint8_t, virt_ptr<const char>);
   RegisterDestructorExport("__dt__Q3_2nn4boss4TaskFv",
                            Task);

   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFPCc",
                              static_cast<nn::Result (Task::*)(virt_ptr<const char>)>(&Task::Initialize));
   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFPCcUi",
                              static_cast<nn::Result(Task::*)(virt_ptr<const char>, uint32_t)>(&Task::Initialize));
   RegisterFunctionExportName("Initialize__Q3_2nn4boss4TaskFUcPCc",
                              static_cast<nn::Result(Task::*)(uint8_t, virt_ptr<const char>)>(&Task::Initialize));
   RegisterFunctionExportName("Finalize__Q3_2nn4boss4TaskFv",
                              &Task::Finalize);

   RegisterFunctionExportName("IsRegistered__Q3_2nn4boss4TaskCFv",
                              &Task::IsRegistered);
   RegisterFunctionExportName("GetAccountID__Q3_2nn4boss4TaskCFv",
                              &Task::GetAccountID);
   RegisterFunctionExportName("GetTaskID__Q3_2nn4boss4TaskCFv",
                              &Task::GetTaskID);
   RegisterFunctionExportName("GetTitleID__Q3_2nn4boss4TaskCFv",
                              &Task::GetTitleID);

   registerTypeInfo<Task>(
      "nn::boss::Task",
      {
         "__dt__Q3_2nn4boss4TaskFv",
      });
}

} // namespace cafe::nn::boss
