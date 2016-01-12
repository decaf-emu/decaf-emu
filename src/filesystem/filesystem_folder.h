#pragma once
#include <string>
#include "filesystem_node.h"

namespace fs
{

class FolderHandle;

class Folder : public Node
{
public:
   Folder(const std::string &name) :
      Node(Node::FolderNode, name)
   {
   }

   virtual ~Folder() override = default;

   virtual Node *addFolder(const std::string &name) = 0;
   virtual Node *addFile(const std::string &name) = 0;
   virtual Node *addChild(Node *node) = 0;

   virtual bool deleteFolder(const std::string &name) = 0;
   virtual bool deleteFile(const std::string &name) = 0;
   virtual bool deleteChild(Node *node) = 0;

   virtual Node *findChild(const std::string &name) = 0;

   virtual FolderHandle *open() = 0;
};

} // namespace fs
