#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

namespace nn
{

namespace boss
{

class TaskID
{
public:
   TaskID();
   TaskID(const char *id);
   TaskID(TaskID *other);

   TaskID *operator =(const char *id);

   bool operator ==(const char *id);
   bool operator ==(TaskID *other);

   bool operator !=(const char *id);
   bool operator !=(TaskID *other);

   operator const char *();

private:
   char mTaskID[8];

protected:
   CHECK_MEMBER_OFFSET_START
   CHECK_OFFSET(TaskID, 0x00, mTaskID);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(TaskID, 8);

} // namespace boss

} // namespace nn
