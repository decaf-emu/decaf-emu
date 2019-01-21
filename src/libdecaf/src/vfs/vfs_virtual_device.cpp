#include "vfs_virtual_device.h"
#include "vfs_virtual_directory.h"
#include "vfs_virtual_directoryiterator.h"
#include "vfs_virtual_file.h"
#include "vfs_virtual_filehandle.h"
#include "vfs_virtual_mounteddevice.h"
#include "vfs_link_device.h"
#include "vfs_overlay_device.h"
#include "vfs_pathiterator.h"

#include <cassert>

namespace vfs
{

VirtualDevice::VirtualDevice(std::string rootDeviceName) :
   Device(Device::Virtual),
   mRootDeviceName(std::move(rootDeviceName)),
   mRoot(std::make_shared<VirtualDirectory>())
{
}

Result<std::shared_ptr<Device>>
VirtualDevice::getLinkDevice(const User &user,
                             const Path &path)
{
   // TODO: This can be optimised to get a link device from the deepest path
   return { std::make_shared<LinkDevice>(shared_from_this(), path) };
}

Error
VirtualDevice::makeFolder(const User &user,
                          const Path &path)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (relativePath.empty()) {
      return Error::AlreadyExists;
   }

   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->makeFolder(user, relativePath);
   }

   if (node->type == VirtualNode::File) {
      return Error::NotDirectory;
   }

   if (relativePath.depth() > 1) {
      return Error::NotFound;
   }

   assert(node->type == VirtualNode::Directory);
   auto parentDirectory = static_cast<VirtualDirectory *>(node.get());
   parentDirectory->children.emplace(relativePath.path(),
                                     std::make_shared<VirtualDirectory>());
   return Error::Success;
}

Error
VirtualDevice::makeFolders(const User &user,
                           const Path &path)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (relativePath.empty()) {
      return Error::AlreadyExists;
   }

   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->makeFolder(user, relativePath);
   }

   if (node->type == VirtualNode::File) {
      return Error::NotDirectory;
   }

   auto parentDirectory = static_cast<VirtualDirectory *>(node.get());
   for (auto &childPath : relativePath) {
      auto childDirectory = new VirtualDirectory();
      parentDirectory->children.emplace(relativePath.path(), childDirectory);
      parentDirectory = childDirectory;
   }

   return Error::Success;
}

Error
VirtualDevice::mountDevice(const User &user,
                           const Path &path,
                           std::shared_ptr<Device> device)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (relativePath.depth() == 0) {
      return Error::AlreadyExists;
   }

   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->mountDevice(user, relativePath, device);
   }

   if (relativePath.depth() > 1) {
      return Error::NotFound;
   }

   if (node->type == VirtualNode::File) {
      return Error::NotDirectory;
   }

   assert(node->type == VirtualNode::Directory);
   auto parentDirectory = static_cast<VirtualDirectory *>(node.get());
   parentDirectory->children.emplace(
      relativePath.path(),
      std::make_shared<VirtualMountedDevice>(std::move(device)));
   return Error::Success;
}

Error
VirtualDevice::mountOverlayDevice(const User &user,
                                  OverlayPriority priority,
                                  const Path &path,
                                  std::shared_ptr<Device> device)
{
   auto[node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());

      if (relativePath.depth() > 0) {
         return mountedDevice->device->mountOverlayDevice(user, priority,
                                                          relativePath,
                                                          device);
      }

      if (mountedDevice->device->type() != Device::Overlay) {
         // Remount the original device under an overlay device with priority 1
         auto overlayDevice = std::make_shared<OverlayDevice>();
         overlayDevice->mountOverlayDevice(user, priority, {},
                                           std::move(mountedDevice->device));
         mountedDevice->device = std::move(overlayDevice);
      }

      auto overlayDevice = static_cast<OverlayDevice *>(mountedDevice->device.get());
      return overlayDevice->mountOverlayDevice(user, priority, {},
                                               std::move(device));
   }

   if (relativePath.depth() > 1) {
      return Error::NotFound;
   }

   if (node->type == VirtualNode::File) {
      return Error::NotDirectory;
   }

   assert(node->type == VirtualNode::Directory);
   auto parentDirectory = static_cast<VirtualDirectory *>(node.get());
   auto overlayDevice = std::make_shared<OverlayDevice>();
   overlayDevice->mountOverlayDevice(user, priority, {}, std::move(device));
   parentDirectory->children.emplace(relativePath.path(),
      std::make_shared<VirtualMountedDevice>(std::move(overlayDevice)));
   return Error::Success;
}

Error
VirtualDevice::unmountDevice(const User &user,
                             const Path &path)
{
   auto [parentNode, relativePath] = findDeepest(user, path, 1);
   if (parentNode->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(parentNode.get());
      return mountedDevice->device->unmountDevice(user, relativePath);
   }

   if (relativePath.depth() > 1) {
      return Error::NotFound;
   }

   if (parentNode->type != VirtualNode::Directory) {
      return Error::NotDirectory;
   }

   // Remove device from parent
   auto parentDirectory = static_cast<VirtualDirectory *>(parentNode.get());
   parentDirectory->children.erase(relativePath.path());
   return Error::Success;
}

Result<DirectoryIterator>
VirtualDevice::openDirectory(const User &user,
                             const Path &path)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->openDirectory(user, relativePath);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   if (node->type != VirtualNode::Directory) {
      return Error::NotDirectory;
   }

   auto dir = std::static_pointer_cast<VirtualDirectory>(node);
   return { DirectoryIterator {
      std::make_shared<VirtualDirectoryIterator>(std::move(dir)) } };
}

Result<std::unique_ptr<FileHandle>>
VirtualDevice::openFile(const User &user,
                        const Path &path,
                        FileHandle::Mode mode)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      if (relativePath.depth() == 0) {
         return Error::NotFile;
      }

      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->openFile(user, relativePath, mode);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   if (node->type != VirtualNode::File) {
      return Error::NotFile;
   }

   auto file = std::static_pointer_cast<VirtualFile>(node);
   return { std::make_unique<VirtualFileHandle>(std::move(file), mode) };
}

Error
VirtualDevice::remove(const User &user,
                      const Path &path)
{
   auto [parentNode, relativePath] = findDeepest(user, path, 1);
   if (parentNode->type == VirtualNode::MountedDevice) {
      // Forward remove into mounted device
      auto mountedDevice = static_cast<VirtualMountedDevice *>(parentNode.get());
      return mountedDevice->device->remove(user, relativePath);
   }

   if (relativePath.depth() > 1) {
      return Error::NotFound;
   }

   if (parentNode->type != VirtualNode::Directory) {
      // Can only erase from a directory
      return Error::NotDirectory;
   }

   // Remove device from parent
   auto parentDirectory = static_cast<VirtualDirectory *>(parentNode.get());
   parentDirectory->children.erase(relativePath.path());
   return Error::Success;
}

Error
VirtualDevice::rename(const User &user,
                      const Path &src,
                      const Path &dst)
{
   auto [srcParent, srcRelativePath] = findDeepest(user, src, 1);
   auto [dstParent, dstRelativePath] = findDeepest(user, dst, 1);

   if (srcParent->type == VirtualNode::MountedDevice ||
       dstParent->type == VirtualNode::MountedDevice) {
      if (srcParent != dstParent) {
         // Cannot move across different mounted devices.
         return Error::OperationNotSupported;
      }

      auto mountedDevice = static_cast<VirtualMountedDevice *>(srcParent.get());
      return mountedDevice->device->rename(user, srcRelativePath,
                                           dstRelativePath);
   }

   if (srcRelativePath.depth() > 1 || dstRelativePath.depth() > 1) {
      return Error::NotFound;
   }

   if (srcParent->type != VirtualNode::Directory ||
       dstParent->type != VirtualNode::Directory) {
      // Can only move items between directories..
      return Error::NotDirectory;
   }

   // Move node from src parent to dst parent
   auto srcParentDirectory = static_cast<VirtualDirectory *>(srcParent.get());
   auto dstParentDirectory = static_cast<VirtualDirectory *>(dstParent.get());

   dstParentDirectory->children[dstRelativePath.path()] =
      std::move(srcParentDirectory->children[srcRelativePath.path()]);
   srcParentDirectory->children.erase(srcRelativePath.path());
   return Error::OperationNotSupported;
}

Error
VirtualDevice::setGroup(const User &user,
                        const Path &path,
                        GroupId group)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->setGroup(user, relativePath, group);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   node->group = group;
   return Error::Success;
}

Error
VirtualDevice::setOwner(const User &user,
                        const Path &path,
                        OwnerId owner)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->setOwner(user, relativePath, owner);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   node->owner = owner;
   return Error::Success;
}

Error
VirtualDevice::setPermissions(const User &user,
                              const Path &path,
                              Permissions mode)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->setPermissions(user, relativePath, mode);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   node->permission = mode;
   return Error::Success;
}

Result<Status>
VirtualDevice::status(const User &user, const Path &path)
{
   auto [node, relativePath] = findDeepest(user, path);
   if (node->type == VirtualNode::MountedDevice) {
      auto mountedDevice = static_cast<VirtualMountedDevice *>(node.get());
      return mountedDevice->device->status(user, relativePath);
   }

   if (relativePath.depth() > 0) {
      return Error::NotFound;
   }

   auto status = Status { };
   status.name = std::prev(std::end(path))->path();

   if (node->type == VirtualNode::Directory) {
      status.flags |= Status::IsDirectory;
   }

   if (node->type == VirtualNode::File) {
      auto file = static_cast<VirtualFile *>(node.get());
      status.size = file->data.size();
   }

   status.group = node->group;
   status.owner = node->owner;
   status.permission = node->permission;
   status.flags |= Status::HasPermissions;
   return status;
}

std::pair<std::shared_ptr<VirtualNode>, Path>
VirtualDevice::findDeepest(const User &user,
                           Path path,
                           int parentLevel)
{
   if (path.empty()) {
      return { mRoot, {} };
   }

   if (!mRootDeviceName.empty()) {
      auto pathRoot = std::begin(path);
      if (pathRoot->path() != mRootDeviceName) {
         return { nullptr, {} };
      }

      path = Path { std::next(pathRoot), std::end(path) };
      if (path.empty()) {
         return { mRoot, {} };
      }
   }

   auto dir = mRoot;
   auto itr = path.begin();
   auto last = std::prev(path.end(), parentLevel);
   for (; itr != last; ++itr) {
      auto name = itr->path();
      auto childItr = dir->children.find(itr->path());
      if (childItr == dir->children.end()) {
         // We have reached as far as we could.
         break;
      }

      auto child = childItr->second;
      if (child->type == VirtualNode::Directory) {
         // Iterate through directory
         dir = std::static_pointer_cast<VirtualDirectory>(child);
      } else if (child->type == VirtualNode::MountedDevice ||
                  child->type == VirtualNode::File) {
         // Cannot iterate through device or a file
         return { child, Path { ++itr, path.end() } };
      }
   }

   return { dir, Path { itr, path.end() } };
}

} // namespace vfs
