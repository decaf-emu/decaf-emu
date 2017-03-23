#pragma once
#include "filesystem_file.h"

namespace fs
{

class FileLink : public File
{
public:
   FileLink(File *file,
            const std::string &name) :
      File(DeviceType::LinkDevice, Permissions::ReadWrite, name),
      mLink(file)
   {
   }

   virtual ~FileLink() override = default;

   File *getLink()
   {
      return mLink;
   }

   virtual FileHandle
   open(OpenMode mode)  final override
   {
      return mLink->open(mode);
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags) override
   {
      mLink->setPermissions(permissions, flags);
   }

private:
   File *mLink;
};

} // namespace fs
