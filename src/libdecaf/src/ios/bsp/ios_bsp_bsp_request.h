#pragma once
#include <libcpu/be2_struct.h>

namespace ios::bsp
{

/**
 * \ingroup ios_dev_bsp
 * @{
 */

#pragma pack(push, 1)

struct BSPRequest
{
   UNKNOWN(0x48);
};
CHECK_SIZE(BSPRequest, 0x48);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
