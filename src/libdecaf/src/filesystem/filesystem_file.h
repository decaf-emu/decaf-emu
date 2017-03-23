#pragma once
#include <string>
#include "filesystem_node.h"
#include "filesystem_filehandle.h"

namespace fs
{

class File : public Node
{
public:
   enum OpenMode
   {
      Read     = 1 << 0,
      Write    = 1 << 1,
      Append   = 1 << 2,
      Update   = 1 << 3,
   };

public:
   File(DeviceType deviceType,
        Permissions permissions,
        const std::string &name) :
      Node(Node::FileNode, deviceType, permissions, name)
   {
   }

   virtual ~File() override = default;

   virtual FileHandle
   open(OpenMode mode) = 0;

protected:
   bool
   checkOpenPermissions(OpenMode mode)
   {
      auto valid = true;

      if (mode & File::Read) {
         valid &= !!(mPermissions & Permissions::Read);
      }

      if (mode & File::Write) {
         valid &= !!(mPermissions & Permissions::Write);
      }

      if (mode & File::Append) {
         valid &= !!(mPermissions & Permissions::Write);
      }

      if (mode & File::Update) {
         valid &= !!(mPermissions & Permissions::Read);
      }

      return valid;
   }
};

} // namespace fs
