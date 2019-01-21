#include "ios_fs_fsa_device.h"
#include "ios/ios.h"

#include "vfs/vfs_error.h"
#include "vfs/vfs_filehandle.h"
#include "vfs/vfs_virtual_device.h"

#include <common/strutils.h>

namespace ios::fs::internal
{

FSADevice::FSADevice() :
   mFS(ios::getFileSystem())
{
}

FSAStatus
FSADevice::translateError(vfs::Error error) const
{
   switch (error) {
   case vfs::Error::Success:
      return FSAStatus::OK;
   case vfs::Error::EndOfDirectory:
      return FSAStatus::EndOfDir;
   case vfs::Error::EndOfFile:
      return FSAStatus::EndOfFile;
   case vfs::Error::AlreadyExists:
      return FSAStatus::AlreadyExists;
   case vfs::Error::InvalidPath:
      return FSAStatus::InvalidPath;
   case vfs::Error::InvalidSeekDirection:
   case vfs::Error::InvalidSeekPosition:
   case vfs::Error::InvalidTruncatePosition:
      return FSAStatus::InvalidParam;
   case vfs::Error::NotDirectory:
      return FSAStatus::NotDir;
   case vfs::Error::NotFile:
      return FSAStatus::NotFile;
   case vfs::Error::NotFound:
      return FSAStatus::NotFound;
   case vfs::Error::NotOpen:
      return FSAStatus::InvalidFileHandle;
   case vfs::Error::OperationNotSupported:
      return FSAStatus::UnsupportedCmd;
   case vfs::Error::ExecutePermission:
   case vfs::Error::ReadOnly:
   case vfs::Error::ReadPermission:
   case vfs::Error::WritePermission:
      return FSAStatus::PermissionError;
   default:
      return FSAStatus::MediaError;
   }
}


vfs::Path
FSADevice::translatePath(phys_ptr<const char> path) const
{
   if (path[0] == '/') {
      return { path.get() };
   } else {
      return mWorkingPath / path.get();
   }
}


vfs::FileHandle::Mode
FSADevice::translateMode(phys_ptr<const char> mode) const
{
   auto result = 0u;

   if (std::strchr(mode.get(), 'r')) {
      result |= vfs::FileHandle::Read;
   }

   if (std::strchr(mode.get(), 'w')) {
      result |= vfs::FileHandle::Write;
   }

   if (std::strchr(mode.get(), 'a')) {
      result |= vfs::FileHandle::Append;
   }

   if (std::strchr(mode.get(), '+')) {
      result |= vfs::FileHandle::Update;
   }

   return static_cast<vfs::FileHandle::Mode>(result);
}


void
FSADevice::translateStat(const vfs::Status &entry,
                         phys_ptr<FSAStat> stat) const
{
   std::memset(stat.get(), 0, sizeof(FSAStat));

   if (entry.flags == vfs::Status::IsDirectory) {
      stat->flags |= FSAStatFlags::Directory;
   }

   if (entry.flags == vfs::Status::HasSize) {
      stat->size = static_cast<uint32_t>(entry.size);
   }

   // TODO: Fill out this.
   stat->permission = 0x660u;
   stat->owner = 0u;
   stat->group = 0u;
   stat->entryId = 0u;
   stat->created = 0;
   stat->modified = 0;
}


int32_t
FSADevice::mapHandle(std::unique_ptr<vfs::FileHandle> file)
{
   auto index = 0;

   for (index = 0; index < mHandles.size(); ++index) {
      if (mHandles[index].type == Handle::Unused) {
         mHandles[index].type = Handle::File;
         mHandles[index].file = std::move(file);
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.emplace_back(std::move(file));
   }

   return index + 1;
}


int32_t
FSADevice::mapHandle(vfs::DirectoryIterator folder)
{
   auto index = 0;

   for (index = 0; index < mHandles.size(); ++index) {
      if (mHandles[index].type == Handle::Unused) {
         mHandles[index].type = Handle::Directory;
         mHandles[index].directory = std::move(folder);
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.emplace_back(std::move(folder));
   }

   return index + 1;
}


FSAStatus
FSADevice::mapFileHandle(int32_t index,
                         Handle *&handle)
{
   if (index <= 0 || index > mHandles.size()) {
      return FSAStatus::InvalidFileHandle;
   }

   if (mHandles[index - 1].type != Handle::File) {
      return FSAStatus::NotFile;
   }

   handle = &mHandles[index - 1];
   return FSAStatus::OK;
}


FSAStatus
FSADevice::mapFolderHandle(int32_t index,
                           Handle *&handle)
{
   if (index <= 0 || index > mHandles.size()) {
      return FSAStatus::InvalidDirHandle;
   }

   if (mHandles[index - 1].type != Handle::Directory) {
      return FSAStatus::NotFile;
   }

   handle = &mHandles[index - 1];
   return FSAStatus::OK;
}


FSAStatus
FSADevice::removeHandle(int32_t index,
                        Handle::Type type)
{
   if (index <= 0 || index > mHandles.size()) {
      if (type == Handle::File) {
         return FSAStatus::InvalidFileHandle;
      } else {
         return FSAStatus::InvalidDirHandle;
      }
   }

   auto &handle = mHandles[index - 1];
   if (handle.type != type) {
      if (type == Handle::File) {
         return FSAStatus::NotFile;
      } else {
         return FSAStatus::NotDir;
      }
   }

   handle.type = Handle::Unused;
   handle.file = {};
   handle.directory = {};
   return FSAStatus::OK;
}


FSAStatus
FSADevice::changeDir(vfs::User user,
                     phys_ptr<FSARequestChangeDir> request)
{
   mWorkingPath = translatePath(phys_addrof(request->path));
   return FSAStatus::OK;
}


FSAStatus
FSADevice::closeDir(vfs::User user,
                    phys_ptr<FSARequestCloseDir> request)
{
   return removeHandle(request->handle, Handle::Directory);
}


FSAStatus
FSADevice::closeFile(vfs::User user,
                     phys_ptr<FSARequestCloseFile> request)
{
   return removeHandle(request->handle, Handle::File);
}


FSAStatus
FSADevice::flushFile(vfs::User user,
                     phys_ptr<FSARequestFlushFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   handle->file->flush();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::flushQuota(vfs::User user,
                      phys_ptr<FSARequestFlushQuota> request)
{
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getCwd(vfs::User user,
                  phys_ptr<FSAResponseGetCwd> response)
{
   string_copy(phys_addrof(response->path).get(),
               mWorkingPath.path().c_str(),
               response->path.size() - 1);
   response->path[response->path.size() - 1] = char { 0 };
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getInfoByQuery(vfs::User user,
                          phys_ptr<FSARequestGetInfoByQuery> request,
                          phys_ptr<FSAResponseGetInfoByQuery> response)
{
   auto path = translatePath(phys_addrof(request->path));

   switch (request->type) {
   case FSAQueryInfoType::FreeSpaceSize:
   {
      response->freeSpaceSize = 128ull * 1024 * 1024;
      break;
   }
   case FSAQueryInfoType::Stat:
   {
      auto result = mFS->status(user, path);
      if (!result) {
         return translateError(result.error());
      }

      translateStat(*result, phys_addrof(response->stat));
      break;
   }
   default:
      return FSAStatus::UnsupportedCmd;
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::getPosFile(vfs::User user,
                      phys_ptr<FSARequestGetPosFile> request,
                      phys_ptr<FSAResponseGetPosFile> response)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   auto result = handle->file->tell();
   if (!result) {
      return translateError(result.error());
   }

   response->pos = static_cast<uint32_t>(*result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::isEof(vfs::User user,
                 phys_ptr<FSARequestIsEof> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   auto result = handle->file->eof();
   if (!result) {
      return translateError(result.error());
   }

   if (*result) {
      error = FSAStatus::EndOfFile;
   } else {
      error = FSAStatus::OK;
   }

   return error;
}


FSAStatus
FSADevice::makeDir(vfs::User user,
                   phys_ptr<FSARequestMakeDir> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->makeFolder(user, path);
   if (error != vfs::Error::Success) {
      return translateError(error);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::makeQuota(vfs::User user,
                     phys_ptr<FSARequestMakeQuota> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->makeFolder(user, path);
   if (error != vfs::Error::Success) {
      return translateError(error);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::mount(vfs::User user,
                 phys_ptr<FSARequestMount> request)
{
   auto linkDevice = mFS->getLinkDevice(user, translatePath(phys_addrof(request->path)));
   if (!linkDevice) {
      return translateError(linkDevice.error());
   }

   auto error = mFS->mountDevice(user,
                                 translatePath(phys_addrof(request->target)),
                                 std::static_pointer_cast<vfs::Device>(*linkDevice));
   if (error != vfs::Error::Success) {
      return translateError(error);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::mountWithProcess(vfs::User user,
                            phys_ptr<FSARequestMountWithProcess> request)
{
   auto linkDevice = mFS->getLinkDevice(user, translatePath(phys_addrof(request->path)));
   if (!linkDevice) {
      return translateError(linkDevice.error());
   }

   auto error = mFS->mountDevice(user,
                                 translatePath(phys_addrof(request->target)),
                                 std::static_pointer_cast<vfs::Device>(*linkDevice));
   if (error != vfs::Error::Success) {
      return translateError(error);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::openDir(vfs::User user,
                   phys_ptr<FSARequestOpenDir> request,
                   phys_ptr<FSAResponseOpenDir> response)
{
   auto path = translatePath(phys_addrof(request->path));
   auto result = mFS->openDirectory(user, path);
   if (!result) {
      return translateError(result.error());
   }

   response->handle = mapHandle(*result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::openFile(vfs::User user,
                    phys_ptr<FSARequestOpenFile> request,
                    phys_ptr<FSAResponseOpenFile> response)
{
   auto path = translatePath(phys_addrof(request->path));
   auto mode = translateMode(phys_addrof(request->mode));
   auto result = mFS->openFile(user, path, mode);
   if (!result) {
      return translateError(result.error());
   }

   response->handle = mapHandle(std::move(*result));
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readDir(vfs::User user,
                   phys_ptr<FSARequestReadDir> request,
                   phys_ptr<FSAResponseReadDir> response)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFolderHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   auto result = handle->directory.readEntry();
   if (!result) {
      return translateError(result.error());
   }

   translateStat(*result, phys_addrof(response->entry.stat));
   string_copy(phys_addrof(response->entry.name).get(),
               response->entry.name.size(),
               result->name.c_str(),
               result->name.size());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readFile(vfs::User user,
                    phys_ptr<FSARequestReadFile> request,
                    phys_ptr<uint8_t> buffer,
                    uint32_t bufferLen)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   if (request->readFlags & FSAReadFlag::ReadWithPos) {
      handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   }

   auto result = handle->file->read(buffer.get(), request->size, request->count);
   if (!result) {
      return translateError(result.error());
   }

   auto bytesRead = (*result) * request->size;
   return static_cast<FSAStatus>(bytesRead);
}


FSAStatus
FSADevice::remove(vfs::User user,
                  phys_ptr<FSARequestRemove> request)
{
   auto path = translatePath(phys_addrof(request->path));
   return translateError(mFS->remove(user, path));
}


FSAStatus
FSADevice::rename(vfs::User user,
                  phys_ptr<FSARequestRename> request)
{
   auto src = translatePath(phys_addrof(request->oldPath));
   auto dst = translatePath(phys_addrof(request->newPath));
   return translateError(mFS->rename(user, src, dst));
}


FSAStatus
FSADevice::rewindDir(vfs::User user,
                     phys_ptr<FSARequestRewindDir> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFolderHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   return translateError(handle->directory.rewind());
}


FSAStatus
FSADevice::setPosFile(vfs::User user,
                      phys_ptr<FSARequestSetPosFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::statFile(vfs::User user,
                    phys_ptr<FSARequestStatFile> request,
                    phys_ptr<FSAResponseStatFile> response)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   std::memset(phys_addrof(response->stat).get(), 0, sizeof(response->stat));
   response->stat.flags = static_cast<FSAStatFlags>(0);
   response->stat.permission = 0x660u;
   response->stat.owner = 0u;
   response->stat.group = 0u;
   response->stat.entryId = 0u;
   response->stat.created = 0;
   response->stat.modified = 0;

   auto result = handle->file->size();
   if (result) {
      response->stat.size = static_cast<uint32_t>(*result);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::truncateFile(vfs::User user,
                        phys_ptr<FSARequestTruncateFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   auto result = handle->file->truncate();
   if (!result) {
      return translateError(result.error());
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::unmount(vfs::User user,
                   phys_ptr<FSARequestUnmount> request)
{
   auto path = translatePath(phys_addrof(request->path));
   return translateError(mFS->remove(user, path));
}


FSAStatus
FSADevice::unmountWithProcess(vfs::User user,
                              phys_ptr<FSARequestUnmountWithProcess> request)
{
   auto path = translatePath(phys_addrof(request->path));
   return translateError(mFS->remove(user, path));
}


FSAStatus
FSADevice::writeFile(vfs::User user,
                     phys_ptr<FSARequestWriteFile> request,
                     phys_ptr<const uint8_t> buffer,
                     uint32_t bufferLen)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto error = mapFileHandle(request->handle, handle);
   if (error < 0) {
      return error;
   }

   if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
      handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   }

   auto result = handle->file->write(buffer.get(), request->size, request->count);
   if (!result) {
      return translateError(result.error());
   }

   auto bytesWritten = (*result) * request->size;
   return static_cast<FSAStatus>(bytesWritten);
}

} // namespace ios::fs::internal
