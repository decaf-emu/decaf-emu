#pragma once
#include "filesystem_folder.h"

namespace fs
{

class FolderLink : public Folder
{
public:
   FolderLink(Folder *folder,
              const std::string &name) :
      Folder(DeviceType::LinkDevice, Permissions::ReadWrite, name),
      mLink(folder)
   {
   }

   virtual ~FolderLink() override = default;

   Folder *
   getLink()
   {
      return mLink;
   }

   virtual Result<Folder *>
   addFolder(const std::string &name) final override
   {
      return mLink->addFolder(name);
   }

   virtual Result<Error>
   remove(const std::string &name) final override
   {
      return mLink->remove(name);
   }

   virtual Node *
   findChild(const std::string &name) final override
   {
      return mLink->findChild(name);
   }

   virtual Result<FileHandle>
   openFile(const std::string &name,
            File::OpenMode mode) override
   {
      return mLink->openFile(name, mode);
   }

   virtual Result<FolderHandle>
   openDirectory() final override
   {
      return mLink->openDirectory();
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags) override
   {
      mLink->setPermissions(permissions, flags);
   }

private:
   Folder *mLink;
};

} // namespace fs
