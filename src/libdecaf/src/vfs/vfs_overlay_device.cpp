#include "vfs_overlay_device.h"
#include "vfs_overlay_directoryiterator.h"

#include <algorithm>

namespace vfs
{

OverlayDevice::OverlayDevice() :
   Device(Device::Overlay)
{
}

Result<std::shared_ptr<Device>>
OverlayDevice::getLinkDevice(const User &user,
                             const Path &path)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->getLinkDevice(user, path);
      if (result.error() != Error::NotFound) {
         return std::move(result);
      }
   }

   return { Error::NotFound };
}

Error
OverlayDevice::makeFolder(const User &user,
                          const Path &path)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::makeFolders(const User &user,
                           const Path &path)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::mountDevice(const User &user,
                           const Path &path,
                           std::shared_ptr<Device> device)
{
   // Overlay devices are read only.
   return { Error::WritePermission };
}

Error
OverlayDevice::mountOverlayDevice(const User &user,
                                  OverlayPriority priority,
                                  const Path &path,
                                  std::shared_ptr<Device> device)
{
   if (path.depth() == 0) {
      mDevices.emplace_back(priority, std::move(device));
      std::sort(mDevices.begin(), mDevices.end(),
                [](const auto &lhs, const auto &rhs)
                {
                   return lhs.first < rhs.first;
                });
      // Maybe we would want to error if we have 2 devices of same priority?
      return Error::Success;
   }

   // Overlay devices are read only.
   return { Error::WritePermission };
}

Error
OverlayDevice::unmountDevice(const User &user,
                             const Path &path)
{
   // Overlay devices are read only.
   return Error::OperationNotSupported;
}

Result<DirectoryIterator>
OverlayDevice::openDirectory(const User &user,
                             const Path &path)
{
   auto error = Error { };
   auto iterator = std::make_shared<OverlayDirectoryIterator>(user, path, this, error);
   if (error != Error::Success) {
      return { error };
   }

   return { DirectoryIterator { std::move(iterator) } };
}

Result<std::unique_ptr<FileHandle>>
OverlayDevice::openFile(const User &user,
                        const Path &path,
                        FileHandle::Mode mode)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->openFile(user, path, mode);
      if (result.error() != Error::NotFound) {
         return std::move(result);
      }
   }

   return { Error::NotFound };
}

Error
OverlayDevice::remove(const User &user,
                      const Path &path)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::rename(const User &user,
                      const Path &src,
                      const Path &dst)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::setGroup(const User &user,
                        const Path &path,
                        GroupId group)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::setOwner(const User &user,
                        const Path &path,
                        OwnerId owner)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Error
OverlayDevice::setPermissions(const User &user,
                              const Path &path,
                              Permissions mode)
{
   // Overlay devices are read only.
   return Error::WritePermission;
}

Result<Status>
OverlayDevice::status(const User &user,
                      const Path &path)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->status(user, path);
      if (result.error() != Error::NotFound) {
         return std::move(result);
      }
   }

   return { Error::NotFound };
}

} // namespace vfs
