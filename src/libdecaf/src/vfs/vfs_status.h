#pragma once
#include "vfs_permissions.h"

#include <cstdint>
#include <string>

namespace vfs
{

struct Status
{
   enum Flags
   {
      None           = 0,
      HasSize        = 1 << 0,
      HasPermissions = 1 << 1,
      IsDirectory    = 1 << 2,
   };

   std::string name;
   GroupId group = 0;
   OwnerId owner = 0;
   Permissions permission = Permissions::NoPermissions;
   std::uintmax_t size = 0;
   int flags = Flags::None;
};

} // namespace vfs
