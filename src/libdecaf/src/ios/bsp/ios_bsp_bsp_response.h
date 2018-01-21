#pragma once
#include "ios_bsp_enum.h"
#include <libcpu/be2_struct.h>

namespace ios::bsp
{

/**
 * \ingroup ios_dev_bsp
 * @{
 */

#pragma pack(push, 1)

struct BSPResponseGetHardwareVersion
{
   be2_val<HardwareVersion> hardwareVersion;
};
CHECK_OFFSET(BSPResponseGetHardwareVersion, 0x00, hardwareVersion);
CHECK_SIZE(BSPResponseGetHardwareVersion, 0x04);

struct BSPResponse
{
   union
   {
      be2_struct<BSPResponseGetHardwareVersion> getHardwareVersion;
      UNKNOWN(0x200);
   };
};
CHECK_SIZE(BSPResponse, 0x200);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
