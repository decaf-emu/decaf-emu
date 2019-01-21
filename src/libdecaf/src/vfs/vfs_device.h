#pragma once
#include "vfs_directoryiterator.h"
#include "vfs_error.h"
#include "vfs_filehandle.h"
#include "vfs_result.h"
#include "vfs_path.h"
#include "vfs_permissions.h"
#include "vfs_status.h"

#include <memory>

namespace vfs
{

class Device
{
public:
   enum Type
   {
      Host,
      Virtual,
      Overlay,
      Link,
   };

   Device(Type type) :
      mType(type)
   {
   }

   virtual Result<std::shared_ptr<Device>> getLinkDevice(const User &user, const Path &path) = 0;
   virtual Error makeFolder(const User &user, const Path &path) = 0;
   virtual Error makeFolders(const User &user, const Path &path) = 0;
   virtual Error mountDevice(const User &user, const Path &path, std::shared_ptr<Device> device) = 0;
   virtual Error mountOverlayDevice(const User &user, OverlayPriority priority, const Path &path, std::shared_ptr<Device> device) = 0;
   virtual Error unmountDevice(const User &user, const Path &path) = 0;
   virtual Result<DirectoryIterator> openDirectory(const User &user, const Path &path) = 0;
   virtual Result<std::unique_ptr<FileHandle>> openFile(const User &user, const Path &path, FileHandle::Mode mode) = 0;
   virtual Error remove(const User &user, const Path &path) = 0;
   virtual Error rename(const User &user, const Path &src, const Path &dst) = 0;
   virtual Error setGroup(const User &user, const Path &path, GroupId group) = 0;
   virtual Error setOwner(const User &user, const Path &path, OwnerId owner) = 0;
   virtual Error setPermissions(const User &user, const Path &path, Permissions mode) = 0;
   virtual Result<Status> status(const User &user, const Path &path) = 0;

   Type type()
   {
      return mType;
   }

private:
   Type mType;
};

} // namespace vfs
