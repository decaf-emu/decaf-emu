#pragma once
#include <string>
#include "filesystem_node.h"
#include "filesystem_error.h"
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include "filesystem_folderhandle.h"

namespace fs
{

class Folder : public Node
{
public:
   Folder(DeviceType type,
          Permissions permissions,
          const std::string &name) :
      Node(Node::FolderNode, type, permissions, name)
   {
   }

   virtual ~Folder() override = default;

   virtual Node *
   addFolder(const std::string &name) = 0;

   virtual Node *
   findChild(const std::string &name) = 0;

   virtual FolderHandle
   openDirectory() = 0;

   virtual FileHandle
   openFile(const std::string &name,
            File::OpenMode mode) = 0;

   virtual bool
   remove(const std::string &name) = 0;
};

} // namespace fs
