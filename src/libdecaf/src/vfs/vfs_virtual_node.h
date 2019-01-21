#pragma once
#include "vfs_permissions.h"

namespace vfs
{

struct VirtualNode
{
   enum Type
   {
      File,
      Directory,
      MountedDevice,
   };

   VirtualNode(Type type) :
      type(type)
   {
   }

   Type type;
   GroupId group = 0;
   OwnerId owner = 0;
   Permissions permission = Permissions::OtherWrite | Permissions::OtherRead;
};

} // namespace vfs
