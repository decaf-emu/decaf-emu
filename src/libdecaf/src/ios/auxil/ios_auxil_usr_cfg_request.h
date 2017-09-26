#pragma once
#include "ios_auxil_enum.h"
#include "ios_auxil_usr_cfg_types.h"

#include <cstdint>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::auxil
{

/**
 * \ingroup ios_auxil
 * @{
 */

#pragma pack(push, 1)

struct UCReadSysConfigRequest
{
   be2_val<uint32_t> count;
   be2_val<uint32_t> size;
   be2_struct<UCSysConfig> settings[1]; // Size=N
};
CHECK_OFFSET(UCReadSysConfigRequest, 0x0, count);
CHECK_OFFSET(UCReadSysConfigRequest, 0x4, size);
CHECK_OFFSET(UCReadSysConfigRequest, 0x8, settings);
CHECK_SIZE(UCReadSysConfigRequest, 0x5C);

struct UCWriteSysConfigRequest
{
   be2_val<uint32_t> count;
   be2_val<uint32_t> size;
   be2_struct<UCSysConfig> settings[1]; // Size=N
};
CHECK_OFFSET(UCWriteSysConfigRequest, 0x0, count);
CHECK_OFFSET(UCWriteSysConfigRequest, 0x4, size);
CHECK_OFFSET(UCWriteSysConfigRequest, 0x8, settings);
CHECK_SIZE(UCWriteSysConfigRequest, 0x5C);

struct UCRequest
{
   union
   {
      be2_struct<UCReadSysConfigRequest> readSysConfigRequest;
      be2_struct<UCWriteSysConfigRequest> writeSysConfigRequest;
   };
};


#pragma pack(pop)

/** @} */

} // namespace namespace ios::auxil
