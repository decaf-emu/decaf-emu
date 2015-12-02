#pragma once
#include <string>
#include "filesystem_node.h"

namespace fs
{

class FileHandle;

class File : public Node
{
public:
   enum OpenMode
   {
      Read = 1 << 0,
      Write = 1 << 1,
      Append = 1 << 2,
      Update = 1 << 3
   };

public:
   File(const std::string &name) :
      Node(Node::FileNode, name)
   {
   }

   virtual ~File() override = default;

   virtual FileHandle *open(OpenMode mode) = 0;
};

} // namespace fs
