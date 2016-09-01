#pragma once

namespace fs
{

namespace Permissions_
{
enum Value
{
   None        = 0,
   Read        = 1 << 0,
   Write       = 1 << 1,
   ReadWrite   = Read | Write,
};
}
using Permissions = Permissions_::Value;

namespace PermissionFlags_
{
enum Value
{
   None        = 0,
   Recursive   = 1 << 0,
};
}
using PermissionFlags = PermissionFlags_::Value;

} // namespace fs
