#pragma once
#include "camera_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::camera
{

#pragma pack(push, 1)

using CAMHandle = int32_t;

struct CAMMemoryInfo
{
   be2_val<uint32_t> unk0;
   be2_val<uint32_t> width;
   be2_val<uint32_t> height;
};
CHECK_OFFSET(CAMMemoryInfo, 0x00, unk0);
CHECK_OFFSET(CAMMemoryInfo, 0x04, width);
CHECK_OFFSET(CAMMemoryInfo, 0x08, height);
CHECK_SIZE(CAMMemoryInfo, 0x0C);

struct CAMInitInfo
{
   CAMMemoryInfo memInfo;
   be2_virt_ptr<void> workMemory;
   be2_val<uint32_t> workMemorySize;
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
        virt_ptr<CAMInitInfo> info,
        virt_ptr<CAMError> outError);

void
CAMExit(CAMHandle handle);

int32_t
CAMOpen(CAMHandle handle);

int32_t
CAMClose(CAMHandle handle);

int32_t
CAMGetMemReq(virt_ptr<CAMMemoryInfo> info);

} // namespace cafe::camera
