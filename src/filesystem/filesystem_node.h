#pragma once
#include <string>

namespace fs
{

class Node
{
public:
   enum NodeType
   {
      Invalid,
      FolderNode,
      FileNode,
   };

   Node(NodeType type, const std::string &name) :
      type(type),
      name(name)
   {
   }

   virtual ~Node() = default;

   NodeType type = Invalid;
   std::string name;
   size_t size = 0;
};

} // namespace fs
