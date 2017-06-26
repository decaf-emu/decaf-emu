#pragma once
#include "usr_cfg_enum.h"
#include "usr_cfg_types.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace kernel
{

namespace ios
{

namespace usr_cfg
{

/**
 * \ingroup kernel_ios_usr_cfg
 * @{
 */

#pragma pack(push, 1)

struct UCReadSysConfigRequest
{
   be_val<uint32_t> count;
   be_val<uint32_t> size;
   UCSysConfig settings[1];
};
CHECK_OFFSET(UCReadSysConfigRequest, 0x0, count);
CHECK_OFFSET(UCReadSysConfigRequest, 0x4, size);
CHECK_OFFSET(UCReadSysConfigRequest, 0x8, settings);
CHECK_SIZE(UCReadSysConfigRequest, 0x5C);

struct UCWriteSysConfigRequest
{
   be_val<uint32_t> count;
   be_val<uint32_t> size;
   UCSysConfig settings[1];
};
CHECK_OFFSET(UCWriteSysConfigRequest, 0x0, count);
CHECK_OFFSET(UCWriteSysConfigRequest, 0x4, size);
CHECK_OFFSET(UCWriteSysConfigRequest, 0x8, settings);
CHECK_SIZE(UCWriteSysConfigRequest, 0x5C);

struct UCRequest
{
   union
   {
      UCReadSysConfigRequest readSysConfigRequest;
      UCWriteSysConfigRequest writeSysConfigRequest;
   };
};


#pragma pack(pop)

/** @} */

} // namespace usr_cfg

} // namespace ios

} // namespace kernel
