#include "nn_boss.h"
#include "nn_boss_task.h"
#include "modules/nn_act/nn_act_core.h"

namespace nn
{

namespace boss
{

ghs::VirtualTableEntry *
Task::VirtualTable = nullptr;

ghs::TypeDescriptor *
Task::TypeInfo = nullptr;

Task::Task()
{
}

Task::Task(const char *taskID)
{
   Initialize(taskID);
}

Task::Task(const char *taskID,
           uint32_t accountID)
{
   Initialize(taskID, accountID);
}

Task::Task(uint8_t slot,
           const char *taskID)
{
   Initialize(slot, taskID);
}

Task::~Task()
{
   Finalize();
}

nn::Result
Task::Initialize(const char *taskID)
{
   return Initialize(taskID, 0);
}

nn::Result
Task::Initialize(const char *taskID,
                 uint32_t accountID)
{
   mAccountID = accountID;
   mTaskID = taskID;
   return nn::Result::Success;
}

nn::Result
Task::Initialize(uint8_t slot,
                 const char *taskID)
{
   return Initialize(taskID, nn::act::GetPersistentIdEx(slot));
}

void
Task::Finalize()
{
}

bool
Task::IsRegistered()
{
   return false;
}

uint32_t
Task::GetAccountID()
{
   return mAccountID;
}

void
Task::GetTaskID(TaskID *id)
{
   *id = mTaskID;
}

void
Task::GetTitleID(TitleID *id)
{
   *id = mTitleID;
}

void Module::registerTask()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss4TaskFv", Task);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss4TaskFPCc", Task, const char *);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss4TaskFPCcUi", Task, const char *, uint32_t);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss4TaskFUcPCc", Task, uint8_t, const char *);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn4boss4TaskFv", Task);

   RegisterKernelFunctionName("Initialize__Q3_2nn4boss4TaskFPCc", static_cast<nn::Result (Task::*)(const char *)>(&Task::Initialize));
   RegisterKernelFunctionName("Initialize__Q3_2nn4boss4TaskFPCcUi", static_cast<nn::Result(Task::*)(const char *, uint32_t)>(&Task::Initialize));
   RegisterKernelFunctionName("Initialize__Q3_2nn4boss4TaskFUcPCc", static_cast<nn::Result(Task::*)(uint8_t, const char *)>(&Task::Initialize));
   RegisterKernelFunctionName("Finalize__Q3_2nn4boss4TaskFv", &Task::Finalize);

   RegisterKernelFunctionName("IsRegistered__Q3_2nn4boss4TaskCFv", &Task::IsRegistered);
   RegisterKernelFunctionName("GetAccountID__Q3_2nn4boss4TaskCFv", &Task::GetAccountID);
   RegisterKernelFunctionName("GetTaskID__Q3_2nn4boss4TaskCFv", &Task::GetTaskID);
   RegisterKernelFunctionName("GetTitleID__Q3_2nn4boss4TaskCFv", &Task::GetTitleID);
}

void Module::initialiseTask()
{
   Task::TypeInfo = ghs::internal::makeTypeDescriptor("nn::boss::Task");

   Task::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, Task::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn4boss4TaskFv") },
   });
}

} // namespace boss

} // namespace nn
