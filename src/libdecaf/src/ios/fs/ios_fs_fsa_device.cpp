#include "ios_fs_fsa_device.h"
#include "ios_fs_log.h"

#include "ios/ios.h"
#include "vfs/vfs_error.h"
#include "vfs/vfs_filehandle.h"
#include "vfs/vfs_virtual_device.h"

#include <common/strutils.h>
#include <libcpu/cpu_formatters.h>

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
   case vfs::Error::Permission:
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
FSADevice::appendFile(vfs::User user,
                      phys_ptr<FSARequestAppendFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::appendFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   fsLog->trace("FSADevice::appendFile[{}] count: {} size: {}",
                request->handle, request->count, request->size);

   // Seek to (count*size - 1) past end of file then write 1 byte.
   char emptyByte = 0;
   auto seekResult =
      handle->file->seek(vfs::FileHandle::SeekEnd,
                         (request->count * request->size) - 1);
   if (seekResult != vfs::Error::Success) {
      fsLog->warn("FSADevice::appendFile[{}] unexpected seekResult {}",
                  request->handle, seekResult);
      return translateError(seekResult);
   }

   auto writeResult = handle->file->write(&emptyByte, 1, 1);
   if (!writeResult) {
      fsLog->warn("FSADevice::appendFile[{}] unexpected writeResult {}",
                  request->handle, seekResult);
      return translateError(writeResult.error());
   }

   return static_cast<FSAStatus>(request->count);
}


FSAStatus
FSADevice::changeDir(vfs::User user,
                     phys_ptr<FSARequestChangeDir> request)
{
   mWorkingPath = translatePath(phys_addrof(request->path));
   fsLog->debug("FSADevice::changeDir path: {}, new working path is: {}",
                phys_addrof(request->path).get(), mWorkingPath.path());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::changeMode(vfs::User user,
                      phys_ptr<FSARequestChangeMode> request)
{
   auto path = translatePath(phys_addrof(request->path));
   fsLog->debug("FSADevice::changeMode path: {} mode1: {} mode2: {}",
                path.path(), request->mode1, request->mode2);
   // TODO: Call mFS->setPermissions
   return FSAStatus::OK;
}


FSAStatus
FSADevice::closeDir(vfs::User user,
                    phys_ptr<FSARequestCloseDir> request)
{
   fsLog->trace("FSADevice::closeDir[{}]", request->handle);
   return removeHandle(request->handle, Handle::Directory);
}


FSAStatus
FSADevice::closeFile(vfs::User user,
                     phys_ptr<FSARequestCloseFile> request)
{
   fsLog->trace("FSADevice::closeFile[{}]", request->handle);
   return removeHandle(request->handle, Handle::File);
}


FSAStatus
FSADevice::flushFile(vfs::User user,
                     phys_ptr<FSARequestFlushFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::fluseFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   fsLog->trace("FSADevice::flushFile[{}] success", request->handle);
   handle->file->flush();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::flushQuota(vfs::User user,
                      phys_ptr<FSARequestFlushQuota> request)
{
   auto path = translatePath(phys_addrof(request->path));
   fsLog->trace("FSADevice::flushQuota path: {} success", path.path());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getCwd(vfs::User user,
                  phys_ptr<FSAResponseGetCwd> response)
{
   auto cwd = mWorkingPath.path();
   if (cwd.empty() || cwd.back() != '/') {
      cwd.push_back('/');
   }
   string_copy(phys_addrof(response->path).get(), cwd.c_str(),
               response->path.size());
   response->path[response->path.size() - 1] = char { 0 };
   fsLog->trace("FSADevice::getCwd cwd: {}", cwd);
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
      if (auto result = mFS->status(user, path); !result) {
         fsLog->debug("FSADevice::getInfoByQuery cmd: FreeSpaceSize path: {} error: {}",
                      path.path(), result.error());
         response->freeSpaceSize = 0ull;
      } else {
         fsLog->debug("FSADevice::getInfoByQuery cmd: FreeSpaceSize path: {}",
                      path.path());
         response->freeSpaceSize = 4096ull * 1024 * 1024;
      }
      break;
   }
   case FSAQueryInfoType::Stat:
   {
      auto result = mFS->status(user, path);
      if (!result) {
         fsLog->debug("FSADevice::getInfoByQuery cmd: Stat path: {} error: {}",
                      path.path(), result.error());
         return translateError(result.error());
      }

      fsLog->debug("FSADevice::getInfoByQuery cmd: Stat path: {}", path.path());
      translateStat(*result, phys_addrof(response->stat));
      break;
   }
   default:
      fsLog->warn("FSADevice::getInfoByQuery unsupported cmd: {} with path: {}",
                  request->type, path.path());
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
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::getPosFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   auto result = handle->file->tell();
   if (!result) {
      fsLog->warn("FSADevice::getPosFile[{}] failed with error {}",
                   request->handle, result.error());
      return translateError(result.error());
   }

   response->pos = static_cast<uint32_t>(*result);
   fsLog->trace("FSADevice::getPosFile[{}] pos: {}",
                request->handle, response->pos);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::isEof(vfs::User user,
                 phys_ptr<FSARequestIsEof> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::isEof[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   auto result = handle->file->eof();
   if (!result) {
      fsLog->warn("FSADevice::isEof[{}] eof failed with error {}",
                  request->handle, result.error());
      return translateError(result.error());
   }

   if (*result) {
      status = FSAStatus::EndOfFile;
   } else {
      status = FSAStatus::OK;
   }

   fsLog->trace("FSADevice::isEof[{}] eof: {}",
                request->handle, *result ? "true" : "false");
   return status;
}


FSAStatus
FSADevice::makeDir(vfs::User user,
                   phys_ptr<FSARequestMakeDir> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->makeFolder(user, path);
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::makeDir path: {} permission: {} failed with "
                   "error {}", path.path(), request->permission, error);
      return translateError(error);
   }

   fsLog->debug("FSADevice::makeDir path: {} permission: {} success",
                path.path(), request->permission);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::makeQuota(vfs::User user,
                     phys_ptr<FSARequestMakeQuota> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->makeFolder(user, path);
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::makeQuota path: {} mode: {} size: {} "
                   "failed with error {}",
                   path.path(), request->mode, request->size, error);
      return translateError(error);
   }

   fsLog->debug("FSADevice::makeQuota path: {} mode: {} size: {} success",
                path.path(), request->mode, request->size);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::mount(vfs::User user,
                 phys_ptr<FSARequestMount> request)
{
   auto sourcePath = translatePath(phys_addrof(request->path));
   auto targetPath = translatePath(phys_addrof(request->target));
   auto linkDevice = mFS->getLinkDevice(user, sourcePath);
   if (!linkDevice) {
      fsLog->debug("FSADevice::mount source: {} target: {} getLinkDevice "
                   "failed with error {}",
                   sourcePath.path(), targetPath.path(), linkDevice.error());
      return translateError(linkDevice.error());
   }

   auto error = mFS->mountDevice(user, targetPath,
                                 std::static_pointer_cast<vfs::Device>(*linkDevice));
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::mount source: {} target: {} mountDevice "
                   "failed with error {}",
                   sourcePath.path(), targetPath.path(), error);
      return translateError(error);
   }

   fsLog->debug("FSADevice::mount source: {} target: {} success",
                sourcePath.path(), targetPath.path());
   return FSAStatus::OK;
}

FSAStatus
FSADevice::mountWithProcess(vfs::User user,
                            phys_ptr<FSARequestMountWithProcess> request)
{
   auto sourcePath = translatePath(phys_addrof(request->path));
   auto targetPath = translatePath(phys_addrof(request->target));
   auto linkDevice = mFS->getLinkDevice(user, sourcePath);
   if (!linkDevice) {
      fsLog->debug("FSADevice::mountWithProcess source: {} target: {} "
                   "priority: {} getLinkDevice failed with error {}",
                   sourcePath.path(), targetPath.path(), request->priority,
                   linkDevice.error());
      return translateError(linkDevice.error());
   }

   auto error =
      mFS->mountOverlayDevice(
         user,
         static_cast<vfs::OverlayPriority>(request->priority),
         targetPath,
         std::static_pointer_cast<vfs::Device>(*linkDevice));
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::mountWithProcess source: {} target: {} "
                   "priority: {} mountOverlayDevice failed with error {}",
                   sourcePath.path(), targetPath.path(), request->priority,
                   linkDevice.error());
      return translateError(error);
   }

   fsLog->debug("FSADevice::mountWithProcess source: {} target: {} "
                "priority: {} success",
                sourcePath.path(), targetPath.path(), request->priority);
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
      fsLog->debug("FSADevice::openDir path: {} failed with error {}",
                   path.path(), result.error());
      return translateError(result.error());
   }

   response->handle = mapHandle(*result);
   fsLog->debug("FSADevice::openDir[{}] path: {} handle: {}",
                response->handle, path.path(), response->handle);
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
      fsLog->debug("FSADevice::openFile path: {} mode: {} failed with error {}",
                   path.path(), mode, result.error());
      return translateError(result.error());
   }

   response->handle = mapHandle(std::move(*result));
   fsLog->debug("FSADevice::openFile[{}] path: {} mode: {} handle: {}",
                response->handle, path.path(), mode, response->handle);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readDir(vfs::User user,
                   phys_ptr<FSARequestReadDir> request,
                   phys_ptr<FSAResponseReadDir> response)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFolderHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::readDir[{}] mapFolderHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   auto result = handle->directory.readEntry();
   if (!result) {
      fsLog->debug("FSADevice::readDir[{}] readEntry failed with error {}",
                   request->handle, result.error());
      return translateError(result.error());
   }

   translateStat(*result, phys_addrof(response->entry.stat));
   string_copy(phys_addrof(response->entry.name).get(),
               response->entry.name.size(),
               result->name.c_str(),
               result->name.size());
   fsLog->debug("FSADevice::readDir[{}] name: {}",
                request->handle, result->name);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readFile(vfs::User user,
                    phys_ptr<FSARequestReadFile> request,
                    phys_ptr<uint8_t> buffer,
                    uint32_t bufferLen)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::readFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   if (request->readFlags & FSAReadFlag::ReadWithPos) {
      handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   }

   auto result = handle->file->read(buffer.get(), request->size, request->count);
   if (!result) {
      if (request->readFlags & FSAReadFlag::ReadWithPos) {
         fsLog->debug("FSADevice::readFile[{}] size: {} count: {} withPos: {} "
                      "failed with error {}",
                      request->handle, request->size, request->count,
                      request->pos, result.error());
      } else {
         fsLog->debug("FSADevice::readFile[{}] size: {} count: {} failed with "
                      "error {}",
                      request->handle, request->size, request->count,
                      result.error());
      }
      return translateError(result.error());
   }

   auto bytesRead = (*result) * request->size;
   if (request->readFlags & FSAReadFlag::ReadWithPos) {
      fsLog->trace("FSADevice::readFile[{}] size: {} count: {} withPos: {} "
                   "read bytes: {}",
                   request->handle, request->size, request->count, request->pos,
                   bytesRead);
   } else {
      fsLog->trace("FSADevice::readFile[{}] size: {} count: {} read bytes: {}",
                   request->handle, request->size, request->count, bytesRead);
   }
   return static_cast<FSAStatus>(bytesRead);
}


FSAStatus
FSADevice::remove(vfs::User user,
                  phys_ptr<FSARequestRemove> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->remove(user, path);
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::remove path: {} failed with error {}",
                   path.path(), error);
   } else {
      fsLog->debug("FSADevice::remove path: {} success", path.path());
   }
   return translateError(error);
}


FSAStatus
FSADevice::rename(vfs::User user,
                  phys_ptr<FSARequestRename> request)
{
   auto src = translatePath(phys_addrof(request->oldPath));
   auto dst = translatePath(phys_addrof(request->newPath));
   auto error = mFS->rename(user, src, dst);
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::rename source: {} target: {} failed with error {}",
                   src.path(), dst.path(), error);
   } else {
      fsLog->debug("FSADevice::rename source: {} target: {} success",
                   src.path(), dst.path());
   }
   return translateError(error);
}


FSAStatus
FSADevice::rewindDir(vfs::User user,
                     phys_ptr<FSARequestRewindDir> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFolderHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::rewindDir[{}] mapFolderHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   auto error = handle->directory.rewind();
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::rewindDir[{}] failed with error {}",
                   error, request->handle);
   } else {
      fsLog->debug("FSADevice::rewindDir[{}] success", request->handle);
   }

   return translateError(error);
}


FSAStatus
FSADevice::setPosFile(vfs::User user,
                      phys_ptr<FSARequestSetPosFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::setPosFile[{}] mapFileHandle failed with error {}",
                  request->handle, status);
      return status;
   }

   handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   fsLog->trace("FSADevice::setPosFile[{}] pos: {}",
                request->handle, request->pos);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::statFile(vfs::User user,
                    phys_ptr<FSARequestStatFile> request,
                    phys_ptr<FSAResponseStatFile> response)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::statFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
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

   fsLog->trace("FSADevice::statFile[{}] size: {}",
                request->handle, response->stat.size);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::truncateFile(vfs::User user,
                        phys_ptr<FSARequestTruncateFile> request)
{
   auto handle = static_cast<Handle *>(nullptr);
   auto status = mapFileHandle(request->handle, handle);
   if (status < 0) {
      fsLog->info("FSADevice::truncateFile[{}] mapFileHandle failed with status {}",
                  request->handle, status);
      return status;
   }

   auto result = handle->file->truncate();
   if (!result) {
      fsLog->info("FSADevice::truncateFile[{}] truncate failed with error {}",
                  request->handle, result.error());
      return translateError(result.error());
   }

   fsLog->trace("FSADevice::truncateFile[{}] success", request->handle);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::unmount(vfs::User user,
                   phys_ptr<FSARequestUnmount> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto error = mFS->unmountDevice(user, path);
   if (error != vfs::Error::Success) {
      fsLog->debug("FSADevice::unmount path: {} failed with error {}",
                   path.path(), error);
   } else {
      fsLog->debug("FSADevice::unmount path: {} success", path.path());
   }
   return translateError(error);
}


FSAStatus
FSADevice::unmountWithProcess(vfs::User user,
                              phys_ptr<FSARequestUnmountWithProcess> request)
{
   auto path = translatePath(phys_addrof(request->path));

   if (request->priority == FSAMountPriority::UnmountAll) {
      // unmountDevice will unmount the base overlay device, thus unmounting
      // all overlay devices at path.
      auto error = mFS->unmountDevice(user, path);
      if (error != vfs::Error::Success) {
         fsLog->debug("FSADevice::unmountWithProcess path: {} priority: {} "
                      "failed with error {}",
                      path.path(), request->priority, error);
      } else {
         fsLog->debug("FSADevice::unmountWithProcess path: {} priority: {} ",
                      "success", path.path(), request->priority);
      }
      return translateError(error);
   } else {
      fsLog->warn("FSADevice::unmountWithProcess path: {} priority: {} "
                   "unsupported",
                   path.path(), request->priority);
      // TODO: unmount overlay device with priority request->priority
      return FSAStatus::UnsupportedCmd;
   }
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
      fsLog->info("FSADevice::writeFile[{}] mapFileHandle failed with error {}",
                  request->handle, error);
      return error;
   }

   if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
      handle->file->seek(vfs::FileHandle::SeekStart, request->pos);
   }

   auto result = handle->file->write(buffer.get(), request->size, request->count);
   if (!result) {
      if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
         fsLog->debug("FSADevice::writeFile[{}] size: {} count: {} withPos: {} "
                      "failed with error {}",
                      request->handle, request->size, request->count,
                      request->pos, result.error());
      } else {
         fsLog->debug("FSADevice::writeFile[{}] size: {} count: {} failed with "
                      "error {}",
                      request->handle, request->size, request->count,
                      result.error());
      }
      return translateError(result.error());
   }

   auto bytesWritten = (*result) * request->size;
   if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
      fsLog->trace("FSADevice::writeFile[{}] size: {} count: {} withPos: {} "
                   "write bytes: {}",
                   request->handle, request->size, request->count, request->pos,
                   bytesWritten);
   } else {
      fsLog->trace("FSADevice::writeFile[{}] size: {} count: {} write bytes: {}",
                   request->handle, request->size, request->count, bytesWritten);
   }
   return static_cast<FSAStatus>(bytesWritten);
}

} // namespace ios::fs::internal
