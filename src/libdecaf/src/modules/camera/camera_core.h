#pragma once
#include <common/be_val.h>
#include <common/structsize.h>

namespace camera
{

#pragma pack(push, 1)

using CAMHandle = int32_t;

enum CAMError : int32_t
{
   OK                   = 0,
   GenericError         = -1,
   AlreadyInitialised   = -12,
};

struct CAMMemoryInfo
{
   be_val<uint32_t> unk0;
   be_val<uint32_t> width;
   be_val<uint32_t> height;
};
CHECK_OFFSET(CAMMemoryInfo, 0x00, unk0);
CHECK_OFFSET(CAMMemoryInfo, 0x04, width);
CHECK_OFFSET(CAMMemoryInfo, 0x08, height);
CHECK_SIZE(CAMMemoryInfo, 0x0C);

struct CAMInitInfo
{
   CAMMemoryInfo memInfo;
   be_ptr<void> workMemory;
   be_val<uint32_t> workMemorySize;
   // 0x14 = some function pointer
   UNKNOWN(0x20);
};
CHECK_OFFSET(CAMInitInfo, 0x00, memInfo);
CHECK_OFFSET(CAMInitInfo, 0x0C, workMemory);
CHECK_OFFSET(CAMInitInfo, 0x10, workMemorySize);
CHECK_SIZE(CAMInitInfo, 0x34);

#pragma pack(pop)

CAMHandle
CAMInit(uint32_t id,
        CAMInitInfo *info,
        be_val<CAMError> *error);

int32_t
CAMGetMemReq(CAMMemoryInfo *info);

} // namespace camera
