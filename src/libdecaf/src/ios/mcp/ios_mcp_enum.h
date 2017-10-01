#ifndef IOS_MCP_ENUM_H
#define IOS_MCP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(mcp)

ENUM_BEG(PMCommand, uint32_t)
   ENUM_VALUE(GetResourceManagerId,       0xE0)
   ENUM_VALUE(RegisterResourceManager,    0xE1)
ENUM_END(PMCommand)

ENUM_BEG(ResourceManagerRegistrationState, uint32_t)
   ENUM_VALUE(Invalid,                    0)
   ENUM_VALUE(Registered,                 1)
   ENUM_VALUE(NotRegistered,              2)
   ENUM_VALUE(Resumed,                    3)
   ENUM_VALUE(Suspended,                  4)
   ENUM_VALUE(Pending,                    5)
   ENUM_VALUE(Failed,                     6)
ENUM_END(ResourceManagerRegistrationState)

ENUM_NAMESPACE_END(mcp)

ENUM_NAMESPACE_END(ios)

#include <common/enum_end.h>

#endif // ifdef IOS_MCP_ENUM_H
