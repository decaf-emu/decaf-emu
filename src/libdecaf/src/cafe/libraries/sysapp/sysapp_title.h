#pragma once
#include "cafe/libraries/coreinit/coreinit_mcp.h"
#include "sysapp_enum.h"

#include <cstdint>

namespace cafe::sysapp
{

uint64_t
SYSGetSystemApplicationTitleId(SystemAppId id);

uint64_t
SYSGetSystemApplicationTitleIdByProdArea(SystemAppId id,
                                         coreinit::MCPRegion region);

} // namespace cafe::sysapp
