#pragma once
#include "coreinit_ios.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

namespace SCILanguage
{
enum Lang
{
   English = 0x01,
};
}

struct UCSysConfig
{
   char name[32];
   UNKNOWN(32);
   be_val<uint32_t> unk1; // access rights? 0x777
   be_val<uint32_t> data_size;
   be_val<uint32_t> unk3; // usually 0x00?
   be_val<uint32_t> unk4; // count? 0x03
   be_ptr<void> data;
};
CHECK_SIZE(UCSysConfig, 0x54);

#pragma pack(pop)

IOHandle
UCOpen();

void
UCClose(IOHandle handle);

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings);
