#pragma once
#include "vfs_virtual_node.h"

#include <map>
#include <memory>
#include <string>

namespace vfs
{

struct VirtualDirectory : public VirtualNode
{
   using child_map = std::map<std::string, std::shared_ptr<VirtualNode>>;
   using iterator = child_map::iterator;
   using const_iterator = child_map::const_iterator;

   VirtualDirectory() :
      VirtualNode(VirtualNode::Directory)
   {
   }

   iterator begin()
   {
      return children.begin();
   }

   iterator end()
   {
      return children.end();
   }

   child_map children;
};

} // namespace vfs
