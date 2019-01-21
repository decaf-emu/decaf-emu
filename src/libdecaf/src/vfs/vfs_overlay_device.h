#pragma once
#include "vfs_device.h"
#include "vfs_permissions.h"
#include "vfs_result.h"

#include <memory>
#include <vector>
#include <utility>

namespace vfs
{

class OverlayDevice : public Device
{
   using device_list = std::vector<std::pair<OverlayPriority, std::shared_ptr<Device>>>;

public:
   using iterator = device_list::iterator;
   using const_iterator = device_list::const_iterator;

   OverlayDevice();

   Result<std::shared_ptr<Device>>
   getLinkDevice(const User &user, const Path &path) override;

   Error
   makeFolder(const User &user, const Path &path) override;

   Error
   makeFolders(const User &user, const Path &path) override;

   Error
   mountDevice(const User &user, const Path &path,
               std::shared_ptr<Device> device) override;

   Error
   mountOverlayDevice(const User &user, OverlayPriority priority,
                      const Path &path,
                      std::shared_ptr<Device> device) override;

   Error
   unmountDevice(const User &user, const Path &path) override;

   Result<DirectoryIterator>
   openDirectory(const User &user, const Path &path) override;

   Result<std::unique_ptr<FileHandle>>
   openFile(const User &user, const Path &path,
            FileHandle::Mode mode) override;

   Error
   remove(const User &user, const Path &path) override;

   Error
   rename(const User &user, const Path &src, const Path &dst) override;

   Error
   setGroup(const User &user, const Path &path, GroupId group) override;

   Error
   setOwner(const User &user, const Path &path, OwnerId owner) override;

   Error
   setPermissions(const User &user, const Path &path, Permissions mode) override;

   Result<Status>
   status(const User &user, const Path &path) override;

   iterator begin()
   {
      return mDevices.begin();
   }

   iterator end()
   {
      return mDevices.end();
   }

   const_iterator begin() const
   {
      return mDevices.begin();
   }

   const_iterator end() const
   {
      return mDevices.end();
   }

private:
   device_list mDevices;
};

} // namespace vfs
