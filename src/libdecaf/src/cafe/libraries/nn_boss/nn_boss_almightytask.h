#pragma once
#include "nn_boss_privilegedtask.h"
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::AlmightyTask::GetTaskRecord((nn::boss::TaskRecord *,uint *))
nn::boss::AlmightyTask::SetHttpOption((ushort))
nn::boss::AlmightyTask::SetPermission((uchar))
*/

namespace cafe::nn_boss
{

struct AlmightyTask : PrivilegedTask
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(AlmightyTask, 0x20);

virt_ptr<AlmightyTask>
AlmightyTask_Constructor(virt_ptr<AlmightyTask> self);

void
AlmightyTask_Destructor(virt_ptr<AlmightyTask> self,
                        ghs::DestructorFlags flags);

nn::Result
AlmightyTask_Initialize(virt_ptr<AlmightyTask> self,
                        virt_ptr<TitleID> titleId,
                        virt_ptr<const char> taskId,
                        uint32_t accountId);

} // namespace cafe::nn_boss
