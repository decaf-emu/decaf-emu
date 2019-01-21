#include "vfs_host_device.h"
#include "vfs_host_directoryiterator.h"
#include "vfs_host_filehandle.h"
#include "vfs_link_device.h"
#include "vfs_virtual_device.h"

#include <common/platform.h>
#ifdef PLATFORM_WINDOWS
#include <winerror.h>
#endif

namespace vfs
{

HostDevice::HostDevice(std::filesystem::path path) :
   Device(Device::Host),
   mHostPath(std::move(path)),
   mVirtualDevice(std::make_shared<VirtualDevice>())
{
}

Result<std::shared_ptr<Device>>
HostDevice::getLinkDevice(const User &user, const Path &path)
{
   return { std::make_shared<LinkDevice>(shared_from_this(), path) };
}

Error
HostDevice::makeFolder(const User &user, const Path &path)
{
   if (mVirtualDevice) {
      mVirtualDevice->makeFolder(user, path);
   }

   if (!checkWritePermission(user, path)) {
      return { Error::ReadPermission };
   }

   auto error = std::error_code { };
   if (std::filesystem::create_directory(makeHostPath(path), error)) {
      return Error::Success;
   }

   return translateError(error);
}

Error
HostDevice::makeFolders(const User &user, const Path &path)
{
   if (mVirtualDevice) {
      mVirtualDevice->makeFolders(user, path);
   }

   if (!checkWritePermission(user, path)) {
      return { Error::ReadPermission };
   }

   auto error = std::error_code { };
   if (std::filesystem::create_directories(makeHostPath(path), error)) {
      return Error::Success;
   }

   return translateError(error);
}

Error
HostDevice::mountDevice(const User &user, const Path &path, std::shared_ptr<Device> device)
{
   return mVirtualDevice->mountDevice(user, path, device);
}

Error
HostDevice::mountOverlayDevice(const User &user, OverlayPriority priority, const Path &path, std::shared_ptr<Device> device)
{
   return mVirtualDevice->mountOverlayDevice(user, priority, path, device);
}

Error
HostDevice::unmountDevice(const User &user, const Path &path)
{
   return mVirtualDevice->unmountDevice(user, path);
}

Result<DirectoryIterator>
HostDevice::openDirectory(const User &user, const Path &path)
{
   if (!checkReadPermission(user, path)) {
      return { Error::ReadPermission };
   }

   auto ec = std::error_code { };
   auto itr = std::filesystem::directory_iterator { makeHostPath(path), ec };
   if (!ec) {
      return { DirectoryIterator {
         std::make_shared<HostDirectoryIterator>(this, std::move(itr)) } };
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->openDirectory(user, path);
      if (result) {
         return result;
      }
   }

   return translateError(ec);
}

static std::string
translateOpenMode(FileHandle::Mode mode)
{
   auto result = std::string { };
   if (mode & FileHandle::Read) {
      result += "r";
   }

   if (mode & FileHandle::Write) {
      result += "w";
   }

   if (mode & FileHandle::Append) {
      result += "a";
   }

   result += "b";

   if (mode & FileHandle::Update) {
      result += "+";
   }

   return result;
}

Result<std::unique_ptr<FileHandle>>
HostDevice::openFile(const User &user, const Path &path, FileHandle::Mode mode)
{
   // Check file permissions
   if ((mode & FileHandle::Read) || (mode & FileHandle::Update)) {
      if (!checkReadPermission(user, path)) {
         return { Error::ReadPermission };
      }
   }

   if ((mode & FileHandle::Write) || (mode & FileHandle::Update)) {
      if (!checkWritePermission(user, path)) {
         return { Error::WritePermission };
      }
   }

   if (mode & FileHandle::Execute) {
      if (!checkExecutePermission(user, path)) {
         return { Error::ExecutePermission };
      }
   }

   auto hostMode = translateOpenMode(mode);
#ifdef PLATFORM_WINDOWS
   auto handle = static_cast<FILE *>(nullptr);
   fopen_s(&handle, makeHostPath(path).string().c_str(), hostMode.c_str());
#else
   auto handle = fopen(makeHostPath(path).string().c_str(), hostMode.c_str());
#endif
   if (handle) {
      return { std::make_unique<HostFileHandle>(handle, mode) };
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->openFile(user, path, mode);
      if (result) {
         return result;
      }
   }

   // TODO: Translate fopen errno
   return { Error::NotFound };
}

Error
HostDevice::remove(const User &user, const Path &path)
{
   if (!checkWritePermission(user, path)) {
      return { Error::WritePermission };
   }

   auto ec = std::error_code { };
   if (std::filesystem::remove(makeHostPath(path), ec)) {
      return Error::Success;
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->remove(user, path);
      if (result == Error::Success) {
         return result;
      }
   }

   return translateError(ec);
}

Error
HostDevice::rename(const User &user, const Path &src, const Path &dst)
{
   if (!checkReadPermission(user, src)) {
      return { Error::ReadPermission };
   }

   if (!checkWritePermission(user, dst)) {
      return { Error::WritePermission };
   }

   auto ec = std::error_code { };
   std::filesystem::rename(makeHostPath(src), makeHostPath(dst), ec);
   if (!ec) {
      return Error::Success;
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->rename(user, src, dst);
      if (result == Error::Success) {
         return result;
      }
   }

   return translateError(ec);
}

Error
HostDevice::setGroup(const User &user, const Path &path, GroupId group)
{
   auto ec = std::error_code { };
   if (std::filesystem::exists(makeHostPath(path), ec)) {
      mPermissionsCache[path.path()].group = group;
      return Error::Success;
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->setGroup(user, path, group);
      if (result == Error::Success) {
         return result;
      }
   }

   return translateError(ec);
}

Error
HostDevice::setOwner(const User &user, const Path &path, OwnerId owner)
{
   auto ec = std::error_code { };
   if (std::filesystem::exists(makeHostPath(path), ec)) {
      mPermissionsCache[path.path()].owner = owner;
      return Error::Success;
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->setOwner(user, path, owner);
      if (result == Error::Success) {
         return result;
      }
   }

   return translateError(ec);
}

Error
HostDevice::setPermissions(const User &user, const Path &path, Permissions mode)
{
   auto ec = std::error_code { };
   if (std::filesystem::exists(makeHostPath(path), ec)) {
      mPermissionsCache[path.path()].permission = mode;
      return Error::Success;
   }

   if (mVirtualDevice) {
      auto result = mVirtualDevice->setPermissions(user, path, mode);
      if (result == Error::Success) {
         return result;
      }
   }

   return translateError(ec);
}

Result<Status>
HostDevice::status(const User &user, const Path &path)
{
   if (!checkReadPermission(user, path)) {
      return { Error::ReadPermission };
   }

   auto ec = std::error_code { };
   auto hostPath = makeHostPath(path);
   auto hostStatus = std::filesystem::status(hostPath, ec);
   if (ec) {
      auto result = mVirtualDevice->status(user, path);
      if (result) {
         return result;
      }

      return { translateError(ec) };
   }

   if (!std::filesystem::exists(hostStatus)) {
      return { Error::NotFound };
   }

   auto status = Status { };
   if (std::filesystem::is_directory(hostStatus)) {
      status.flags |= Status::IsDirectory;
   }

   if (auto size = std::filesystem::file_size(hostPath, ec); !ec) {
      status.size = size;
      status.flags |= Status::HasSize;
   }

   if (auto result = lookupPermissions(path); result) {
      auto &permissions = *result;
      status.group = permissions.group;
      status.owner = permissions.owner;
      status.permission = permissions.permission;
      status.flags |= Status::HasPermissions;
   }

   return { status };
}

Result<HostNodePermission>
HostDevice::lookupPermissions(const Path &path) const
{
   auto itr = mPermissionsCache.find(path.path());
   if (itr == mPermissionsCache.end()) {
      return { Error::NotFound };
   }

   return { itr->second };
}

bool
HostDevice::checkReadPermission(const User &user, const Path &path)
{
   auto itr = mPermissionsCache.find(path.path());
   if (itr == mPermissionsCache.end()) {
      // No permissions mapped for this file, assume valid.
      return true;
   }

   auto &permissions = itr->second;
   if (user.id == permissions.owner) {
      if (permissions.permission & Permissions::OwnerRead) {
         return true;
      }
   }

   if (user.group == permissions.group) {
      if (permissions.permission & Permissions::GroupRead) {
         return true;
      }
   }

   if (permissions.permission & Permissions::OtherRead) {
      return true;
   }

   return false;
}

bool
HostDevice::checkWritePermission(const User &user, const Path &path)
{
   auto itr = mPermissionsCache.find(path.path());
   if (itr == mPermissionsCache.end()) {
      // No permissions mapped for this file, assume valid.
      return true;
   }

   auto &permissions = itr->second;
   if (user.id == permissions.owner) {
      if (permissions.permission & Permissions::OwnerWrite) {
         return true;
      }
   }

   if (user.group == permissions.group) {
      if (permissions.permission & Permissions::GroupWrite) {
         return true;
      }
   }

   if (permissions.permission & Permissions::OtherWrite) {
      return true;
   }

   return false;
}

bool
HostDevice::checkExecutePermission(const User &user, const Path &path)
{
   auto itr = mPermissionsCache.find(path.path());
   if (itr == mPermissionsCache.end()) {
      // No permissions mapped for this file, assume valid.
      return true;
   }

   auto &permissions = itr->second;
   if (user.id == permissions.owner) {
      if (permissions.permission & Permissions::OwnerExecute) {
         return true;
      }
   }

   if (user.group == permissions.group) {
      if (permissions.permission & Permissions::GroupExecute) {
         return true;
      }
   }

   if (permissions.permission & Permissions::OtherExecute) {
      return true;
   }

   return false;
}

std::filesystem::path
HostDevice::makeHostPath(const Path &guestPath) const
{
   return mHostPath / guestPath.path();
}

Error
HostDevice::translateError(const std::error_code &ec) const
{
   if (!ec) {
      return Error::Success;
   }

   if (ec.category() == std::system_category()) {
      return translateError(std::system_error { ec });
   }

   return Error::GenericError;
}

Error
HostDevice::translateError(const std::system_error &ec) const
{
#ifdef PLATFORM_WINDOWS
   switch (ec.code().value()) {
   case ERROR_FILE_NOT_FOUND:
   case ERROR_PATH_NOT_FOUND:
      return Error::NotFound;
   }
#endif

   return Error::GenericError;
}

} // namespace vfs
