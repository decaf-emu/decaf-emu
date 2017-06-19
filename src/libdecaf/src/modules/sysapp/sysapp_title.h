#pragma once
#include "modules/coreinit/coreinit_mcp.h"
#include "sysapp_enum.h"

#include <cstdint>

namespace sysapp
{

uint64_t
SYSGetSystemApplicationTitleId(SystemAppId id);

uint64_t
SYSGetSystemApplicationTitleIdByProdArea(SystemAppId id,
                                         coreinit::MCPRegion region);

} // namespace sysapp
