#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::nn::boss
{

class TaskID
{
public:
   TaskID();
   TaskID(virt_ptr<const char> id);
   TaskID(virt_ptr<TaskID> other);

   virt_ptr<TaskID> operator =(virt_ptr<const char> id);

   bool operator ==(virt_ptr<const char> id);
   bool operator ==(virt_ptr<TaskID> other);

   bool operator !=(virt_ptr<const char> id);
   bool operator !=(virt_ptr<TaskID> other);

   operator virt_ptr<const char>();

private:
   be2_array<char, 8> mTaskID;

protected:
   CHECK_MEMBER_OFFSET_START
      CHECK_OFFSET(TaskID, 0x00, mTaskID);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(TaskID, 8);

} // namespace cafe::nn::boss
