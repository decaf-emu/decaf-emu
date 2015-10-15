#pragma once
#include <string>

namespace fs
{

struct FileSystemNode
{
   enum Type
   {
      FolderNode,
      FileNode
   };

   enum HostType
   {
      VirtualNode,
      HostNode
   };

   FileSystemNode(Type type, HostType hostType, std::string name) :
      type(type),
      hostType(hostType),
      name(name)
   {
   }

   virtual ~FileSystemNode()
   {
   }

   Type type;
   HostType hostType;
   std::string name;
};

} // namespace fs
