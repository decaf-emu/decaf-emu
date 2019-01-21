#include "vfs_link_device.h"

namespace vfs
{

LinkDevice::LinkDevice(std::shared_ptr<Device> device,
                       Path path) :
   Device(Device::Link),
   mDevice(device),
   mPath(path)
{
}

Result<std::shared_ptr<Device>>
LinkDevice::getLinkDevice(const User &user,
                          const Path &path)
{
   if (path.depth() == 0) {
      return { shared_from_this() };
   } else {
      return mDevice->getLinkDevice(user, mPath / path);
   }
}

Error
LinkDevice::makeFolder(const User &user,
                       const Path &path)
{
   return mDevice->makeFolder(user, mPath / path);
}

Error
LinkDevice::makeFolders(const User &user,
                        const Path &path)
{
   return mDevice->makeFolders(user, mPath / path);
}

Error
LinkDevice::mountDevice(const User &user,
                        const Path &path,
                        std::shared_ptr<Device> device)
{
   return mDevice->mountDevice(user, mPath / path, device);
}

Error
LinkDevice::mountOverlayDevice(const User &user,
                               OverlayPriority priority,
                               const Path &path,
                               std::shared_ptr<Device> device)
{
   return mDevice->mountOverlayDevice(user, priority, mPath / path,
                                      std::move(device));
}

Error
LinkDevice::unmountDevice(const User &user,
                          const Path &path)
{
   return mDevice->unmountDevice(user, mPath / path);
}

Result<DirectoryIterator>
LinkDevice::openDirectory(const User &user,
                          const Path &path)
{
   return mDevice->openDirectory(user, mPath / path);
}

Result<std::unique_ptr<FileHandle>>
LinkDevice::openFile(const User &user,
                     const Path &path,
                     FileHandle::Mode mode)
{
   return mDevice->openFile(user, mPath / path, mode);
}

Error
LinkDevice::remove(const User &user,
                   const Path &path)
{
   return mDevice->remove(user, mPath / path);
}

Error
LinkDevice::rename(const User &user,
                   const Path &src,
                   const Path &dst)
{
   return mDevice->rename(user, mPath / src, mPath / dst);
}

Error
LinkDevice::setGroup(const User &user,
                     const Path &path,
                     GroupId group)
{
   return mDevice->setGroup(user, mPath / path, group);
}

Error
LinkDevice::setOwner(const User &user,
                     const Path &path,
                     OwnerId owner)
{
   return mDevice->setOwner(user, mPath / path, owner);
}

Error
LinkDevice::setPermissions(const User &user,
                           const Path &path,
                           Permissions mode)
{
   return mDevice->setPermissions(user, mPath / path, mode);
}

Result<Status>
LinkDevice::status(const User &user,
                   const Path &path)
{
   return mDevice->status(user, mPath / path);
}

} // namespace vfs
