#pragma once

namespace vfs
{

using GroupId = unsigned int;
using OwnerId = unsigned int;
using OverlayPriority = int;

enum Permissions
{
   NoPermissions = 0,

   OtherExecute = 0x001,
   OtherWrite = 0x002,
   OtherRead = 0x004,

   GroupExecute = 0x010,
   GroupWrite = 0x020,
   GroupRead = 0x040,

   OwnerExecute = 0x100,
   OwnerWrite = 0x200,
   OwnerRead = 0x400,
};

inline Permissions operator |(Permissions lhs, Permissions rhs)
{
   return static_cast<Permissions>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline Permissions operator &(Permissions lhs, Permissions rhs)
{
   return static_cast<Permissions>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

struct User
{
   OwnerId id = 0;
   GroupId group = 0;
};

} // namespace vfs
