#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_userconfig User Config
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct UCSysConfig
{
   char name[32];
   UNKNOWN(32);
   be_val<uint32_t> unk1; // access rights? 0x777
   be_val<UCDataType> dataType;
   be_val<uint32_t> unk3; // usually 0x00?
   be_val<uint32_t> dataSize;
   be_ptr<void> data;
};
CHECK_SIZE(UCSysConfig, 0x54);

#pragma pack(pop)

IOHandle
UCOpen();

void
UCClose(IOHandle handle);

IOError
UCReadSysConfig(IOHandle handle,
                uint32_t count,
                UCSysConfig *settings);

IOError
UCWriteSysConfig(IOHandle handle,
                 uint32_t count,
                 UCSysConfig *settings);

/** @} */

} // namespace coreinit
