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
   for (auto &[priority, device] : mDevices) {
      auto result = device->makeFolder(user, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::makeFolders(const User &user,
                           const Path &path)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->makeFolders(user, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::mountDevice(const User &user,
                           const Path &path,
                           std::shared_ptr<Device> device)
{
   if (path.depth() == 0) {
      // Should be using mountOverlayDevice, not mountDevice...
      return Error::OperationNotSupported;
   }

   for (auto &[priority, device] : mDevices) {
      auto result = device->mountDevice(user, path, device);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::mountOverlayDevice(const User &user,
                                  OverlayPriority priority,
                                  const Path &path,
                                  std::shared_ptr<Device> device)
{
   if (path.depth() == 0) {
      for (auto itr = mDevices.begin(); itr != mDevices.end(); ++itr) {
         if (itr->first == priority) {
            return Error::AlreadyExists;
         }
      }

      mDevices.emplace_back(priority, std::move(device));
      std::sort(mDevices.begin(), mDevices.end(),
                [](const auto &lhs, const auto &rhs)
                {
                   return lhs.first > rhs.first;
                });
      return Error::Success;
   }

   for (auto &[priority, device] : mDevices) {
      auto result = device->mountOverlayDevice(user, priority, path, device);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::unmountDevice(const User &user,
                             const Path &path)
{
   if (mDevices.empty()) {
      return Error::NotFound;
   }

   if (path.depth() == 0) {
      // Should be using unmountOverlayDevice, not mountDevice...
      return Error::OperationNotSupported;
   }

   for (auto &[priority, device] : mDevices) {
      auto result = device->unmountDevice(user, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::unmountOverlayDevice(const User &user,
                                    OverlayPriority priority,
                                    const Path &path)
{
   if (path.depth() == 0) {
      for (auto itr = mDevices.begin(); itr != mDevices.end(); ++itr) {
         if (itr->first == priority) {
            mDevices.erase(itr);
            return Error::Success;
         }
      }

      return Error::NotFound;
   }

   for (auto &[priority, device] : mDevices) {
      auto result = device->unmountOverlayDevice(user, priority, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
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
   for (auto &[priority, device] : mDevices) {
      auto result = device->remove(user, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::rename(const User &user,
                      const Path &src,
                      const Path &dst)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->rename(user, src, dst);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::setGroup(const User &user,
                        const Path &path,
                        GroupId group)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->setGroup(user, path, group);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::setOwner(const User &user,
                        const Path &path,
                        OwnerId owner)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->setOwner(user, path, owner);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
}

Error
OverlayDevice::setPermissions(const User &user,
                              const Path &path,
                              Permissions mode)
{
   for (auto &[priority, device] : mDevices) {
      auto result = device->setPermissions(user, path, mode);
      if (result == Error::Success) {
         return result;
      }
   }

   return Error::NotFound;
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
