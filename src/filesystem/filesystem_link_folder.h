#pragma once
#include "filesystem_folder.h"

namespace fs
{

class FolderLink : public Folder
{
public:
   FolderLink(Folder *folder, const std::string &name) :
      Folder(name),
      mLink(folder)
   {
      Node::isLink = true;
   }

   virtual ~FolderLink() override = default;

   virtual Node *addFolder(const std::string &name) final override
   {
      return mLink->addFolder(name);
   }

   virtual Node *addFile(const std::string &name) final override
   {
      return mLink->addFile(name);
   }

   virtual Node *addChild(Node *node) final override
   {
      return mLink->addChild(node);
   }

   virtual bool deleteFolder(const std::string &name) final override
   {
      return mLink->deleteFolder(name);
   }

   virtual bool deleteFile(const std::string &name) final override
   {
      return mLink->deleteFile(name);
   }

   virtual bool deleteChild(Node *node) final override
   {
      return mLink->deleteChild(node);
   }

   virtual Node *findChild(const std::string &name) final override
   {
      return mLink->findChild(name);
   }

   virtual FolderHandle *open() final override
   {
      return mLink->open();
   }

private:
   Folder *mLink;
};

} // namespace fs
