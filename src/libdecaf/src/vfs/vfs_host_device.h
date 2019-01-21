#pragma once
#include "vfs_device.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>

namespace vfs
{

struct HostNodePermission
{
   GroupId group;
   OwnerId owner;
   Permissions permission;
};

class VirtualDevice;

class HostDevice : public Device, public std::enable_shared_from_this<HostDevice>
{
public:
   HostDevice(std::filesystem::path path);

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

   Result<HostNodePermission>
   lookupPermissions(const Path &path) const;

private:
   bool checkReadPermission(const User &user, const Path &path);
   bool checkWritePermission(const User &user, const Path &path);
   bool checkExecutePermission(const User &user, const Path &path);

   std::filesystem::path makeHostPath(const Path &guestPath) const;

   Error translateError(const std::error_code &ec) const;
   Error translateError(const std::system_error &ec) const;

private:
   std::filesystem::path mHostPath;
   std::map<std::string, HostNodePermission> mPermissionsCache;

   //! We want a virtual device backing this host device so we can do things
   //! like mount other devices within a host device. For example this could
   //! be useful if we have MLC / SLC on host device and we want to mount
   //! installed games / patches within there when the user stores their data
   //! external to these system paths.
   std::shared_ptr<VirtualDevice> mVirtualDevice;
};

} // namespace vfs
