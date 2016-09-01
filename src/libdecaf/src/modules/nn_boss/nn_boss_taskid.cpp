#include "nn_boss.h"
#include "nn_boss_taskid.h"

namespace nn
{

namespace boss
{

TaskID::TaskID()
{
   decaf_warn_stub();

   std::memset(mTaskID, 0, 8);
}

TaskID::TaskID(const char *id)
{
   decaf_warn_stub();

   std::strncpy(mTaskID, id, 8);
}

TaskID::TaskID(TaskID *other)
{
   decaf_warn_stub();

   std::memcpy(mTaskID, other->mTaskID, 8);
}

TaskID *
TaskID::operator =(const char *id)
{
   decaf_warn_stub();

   std::strncpy(mTaskID, id, 8);
   return this;
}

bool
TaskID::operator ==(const char *id)
{
   decaf_warn_stub();

   return std::strncmp(mTaskID, id, 8) == 0;
}

bool
TaskID::operator ==(TaskID *other)
{
   decaf_warn_stub();

   return std::strncmp(mTaskID, other->mTaskID, 8) == 0;
}

bool
TaskID::operator !=(const char *id)
{
   decaf_warn_stub();

   return std::strncmp(mTaskID, id, 8) != 0;
}

bool
TaskID::operator !=(TaskID *other)
{
   decaf_warn_stub();

   return std::strncmp(mTaskID, other->mTaskID, 8) != 0;
}

TaskID::operator const char *()
{
   decaf_warn_stub();

   return mTaskID;
}

void
Module::registerTaskID()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss6TaskIDFv", TaskID);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss6TaskIDFPCc", TaskID, const char *);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss6TaskIDFRCQ3_2nn4boss6TaskID", TaskID, TaskID *);
   RegisterKernelFunctionName("__as__Q3_2nn4boss6TaskIDFPCc", static_cast<TaskID *(TaskID::*)(const char *)>(&TaskID::operator =));
   RegisterKernelFunctionName("__eq__Q3_2nn4boss6TaskIDCFPCc", static_cast<bool (TaskID::*)(const char* )>(&TaskID::operator ==));
   RegisterKernelFunctionName("__eq__Q3_2nn4boss6TaskIDCFRCQ3_2nn4boss6TaskID", static_cast<bool (TaskID::*)(TaskID *)>(&TaskID::operator ==));
   RegisterKernelFunctionName("__ne__Q3_2nn4boss6TaskIDCFPCc", static_cast<bool (TaskID::*)(const char*)>(&TaskID::operator !=));
   RegisterKernelFunctionName("__opPCc__Q3_2nn4boss6TaskIDCFv", &TaskID::operator const char*);
}

} // namespace boss

} // namespace nn
