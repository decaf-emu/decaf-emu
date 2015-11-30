#pragma once
#include "filesystem_folder.h"
#include "filesystem_host_file.h"
#include "filesystem_host_folderhandle.h"
#include "filesystem_host_path.h"
#include "filesystem_virtual_folder.h"

namespace fs
{

class HostFolder : public Folder
{
public:
   HostFolder(const HostPath &path, const std::string &name) :
      Folder(name),
      mPath(path),
      mVirtual(name)
   {
   }

   virtual ~HostFolder()
   {
   }

   virtual Node *addFolder(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node) {
         node = new VirtualFolder(name);
         // TODO: Maybe create real host folder
         addChild(node);
      }

      return node;
   }

   virtual Node *addChild(Node *node) override
   {
      mVirtual.addChild(node);
      return node;
   }

   virtual Node *findChild(const std::string &name) override;

   virtual FolderHandle *open() override
   {
      auto handle = new HostFolderHandle(mPath, mVirtual.open());

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return handle;
   }

private:
   HostPath mPath;
   VirtualFolder mVirtual;
};

} // namespace fs
