#include "nn_boss.h"
#include "nn_boss_taskid.h"

#include <common/strutils.h>

namespace cafe::nn_boss
{

TaskID::TaskID()
{
   mTaskID[0] = char { 0 };
}

TaskID::TaskID(virt_ptr<const char> id)
{
   string_copy(virt_addrof(mTaskID).get(), mTaskID.size(), id.get(), 7);
   mTaskID[7] = char { 0 };
}

TaskID::TaskID(virt_ptr<TaskID> other)
{
   std::memcpy(virt_addrof(mTaskID).get(),
               virt_addrof(other->mTaskID).get(),
               8);
}

virt_ptr<TaskID>
TaskID::operator =(virt_ptr<const char> id)
{
   string_copy(virt_addrof(mTaskID).get(), mTaskID.size(), id.get(), 7);
   mTaskID[7] = char { 0 };
   return virt_this(this);
}

bool
TaskID::operator ==(virt_ptr<const char> id)
{
   return std::strncmp(virt_addrof(mTaskID).get(), id.get(), 8) == 0;
}

bool
TaskID::operator ==(virt_ptr<TaskID> other)
{
   return std::strncmp(virt_addrof(mTaskID).get(),
                       virt_addrof(other->mTaskID).get(),
                       8) == 0;
}

bool
TaskID::operator !=(virt_ptr<const char> id)
{
   return !(*this == id);
}

bool TaskID::operator !=(virt_ptr<TaskID> other)
{
   return !(*this == other);
}

TaskID::operator virt_ptr<const char>()
{
   return virt_addrof(mTaskID);
}

void
Library::registerTaskIdSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss6TaskIDFv",
                             TaskID);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss6TaskIDFPCc",
                                 TaskID, virt_ptr<const char>);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss6TaskIDFRCQ3_2nn4boss6TaskID",
                                 TaskID, virt_ptr<TaskID>);
   RegisterFunctionExportName("__as__Q3_2nn4boss6TaskIDFPCc",
                              static_cast<virt_ptr<TaskID> (TaskID::*)(virt_ptr<const char>)>(&TaskID::operator =));
   RegisterFunctionExportName("__eq__Q3_2nn4boss6TaskIDCFPCc",
                              static_cast<bool (TaskID::*)(virt_ptr<const char>)>(&TaskID::operator ==));
   RegisterFunctionExportName("__eq__Q3_2nn4boss6TaskIDCFRCQ3_2nn4boss6TaskID",
                              static_cast<bool (TaskID::*)(virt_ptr<TaskID>)>(&TaskID::operator ==));
   RegisterFunctionExportName("__ne__Q3_2nn4boss6TaskIDCFPCc",
                              static_cast<bool (TaskID::*)(virt_ptr<const char>)>(&TaskID::operator !=));
   RegisterFunctionExportName("__opPCc__Q3_2nn4boss6TaskIDCFv",
                              &TaskID::operator virt_ptr<const char>);
}

} // namespace cafe::nn_boss
