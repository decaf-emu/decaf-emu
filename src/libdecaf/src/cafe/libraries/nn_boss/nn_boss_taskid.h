#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct TaskID
{
   be2_array<char, 8> value;
};
CHECK_OFFSET(TaskID, 0, value);
CHECK_SIZE(TaskID, 8);

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self);

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self,
                   virt_ptr<const char> id);

virt_ptr<TaskID>
TaskID_Constructor(virt_ptr<TaskID> self,
                   virt_ptr<TaskID> other);

virt_ptr<TaskID>
TaskID_OperatorAssign(virt_ptr<TaskID> self,
                      virt_ptr<const char> id);

bool
TaskID_OperatorEqual(virt_ptr<TaskID> self,
                     virt_ptr<const char> id);

bool
TaskID_OperatorEqual(virt_ptr<TaskID> self,
                     virt_ptr<TaskID> other);

bool
TaskID_OperatorNotEqual(virt_ptr<TaskID> self,
                        virt_ptr<const char> id);

bool
TaskID_OperatorNotEqual(virt_ptr<TaskID> self,
                        virt_ptr<TaskID> other);

virt_ptr<const char>
TaskID_OperatorCastConstCharPtr(virt_ptr<TaskID> self);

} // namespace cafe::nn_boss
