#pragma once
#include "nn_boss_task.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::PrivilegedTask::GetAction(const(void))
nn::boss::PrivilegedTask::SetHttpOption((ushort))
nn::boss::PrivilegedTask::SetPermission((uchar))
nn::boss::PrivilegedTask::SetUserAgentMode((nn::boss::UserAgentMode))
*/

namespace cafe::nn_boss
{

struct PrivilegedTask : Task
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(PrivilegedTask, 0x20);

virt_ptr<PrivilegedTask>
PrivilegedTask_Constructor(virt_ptr<PrivilegedTask> self);

virt_ptr<PrivilegedTask>
PrivilegedTask_Constructor(virt_ptr<PrivilegedTask> self,
                           virt_ptr<const char> taskId,
                           uint32_t accountId);
void
PrivilegedTask_Destructor(virt_ptr<PrivilegedTask> self,
                          ghs::DestructorFlags flags);

} // namespace cafe::nn_boss
