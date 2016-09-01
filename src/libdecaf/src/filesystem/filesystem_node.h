#pragma once
#include <string>
#include "filesystem_permissions.h"

namespace fs
{

class Node
{
public:
   enum NodeType
   {
      InvalidNode,
      FolderNode,
      FileNode,
   };

   enum DeviceType
   {
      UnknownDevice,
      VirtualDevice,
      HostDevice,
      LinkDevice,
   };

   Node(NodeType type,
        DeviceType deviceType,
        Permissions permissions,
        const std::string &name) :
      mType(type),
      mDeviceType(deviceType),
      mPermissions(permissions),
      mName(name)
   {
   }

   virtual ~Node() = default;

   NodeType
   type() const
   {
      return mType;
   }

   DeviceType
   deviceType() const
   {
      return mDeviceType;
   }

   const std::string &
   name() const
   {
      return mName;
   }

   size_t
   size() const
   {
      return mSize;
   }

   void
   setSize(size_t size)
   {
      mSize = size;
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags)
   {
      mPermissions = permissions;
   }

protected:
   bool
   checkPermission(Permissions requested) const
   {
      return (mPermissions & requested) == requested;
   }

protected:
   NodeType mType = InvalidNode;
   DeviceType mDeviceType = UnknownDevice;
   size_t mSize = 0;
   Permissions mPermissions = Permissions::None;
   std::string mName;
};

} // namespace fs
