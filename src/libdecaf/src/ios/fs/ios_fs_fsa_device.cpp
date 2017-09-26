#include "ios_fs_fsa_device.h"

namespace ios::fs::internal
{

using FSError = ::fs::Error;
using Path = ::fs::Path;
using File = ::fs::File;
using FileSystem = ::fs::FileSystem;
using FileHandle = ::fs::FileHandle;
using FolderEntry = ::fs::FolderEntry;
using FolderHandle = ::fs::FolderHandle;

FSAStatus
FSADevice::translateError(FSError error) const
{
   switch (error) {
   case FSError::OK:
      return FSAStatus::OK;
   case FSError::UnsupportedOperation:
      return FSAStatus::UnsupportedCmd;
   case FSError::NotFound:
      return FSAStatus::NotFound;
   case FSError::AlreadyExists:
      return FSAStatus::AlreadyExists;
   case FSError::InvalidPermission:
      return FSAStatus::PermissionError;
   case FSError::NotFile:
      return FSAStatus::NotFile;
   case FSError::NotDirectory:
      return FSAStatus::NotDir;
   default:
      return FSAStatus::UnsupportedCmd;
   }
}


Path
FSADevice::translatePath(phys_ptr<const char> path) const
{
   if (path[0] == '/') {
      return { path.getRawPointer() };
   } else {
      return mWorkingPath.join(path.getRawPointer());
   }
}


File::OpenMode
FSADevice::translateMode(phys_ptr<const char> mode) const
{
   auto result = 0u;

   if (std::strchr(mode.getRawPointer(), 'r')) {
      result |= File::Read;
   }

   if (std::strchr(mode.getRawPointer(), 'w')) {
      result |= File::Write;
   }

   if (std::strchr(mode.getRawPointer(), 'a')) {
      result |= File::Append;
   }

   if (std::strchr(mode.getRawPointer(), '+')) {
      result |= File::Update;
   }

   return static_cast<File::OpenMode>(result);
}


void
FSADevice::translateStat(const FolderEntry &entry,
                         phys_ptr<FSAStat> stat) const
{
   std::memset(stat.getRawPointer(), 0, sizeof(FSAStat));

   if (entry.type == FolderEntry::Folder) {
      stat->flags = FSAStatFlags::Directory;
   } else {
      stat->flags = static_cast<FSAStatFlags>(0);
   }

   stat->permission = 0x660u;
   stat->owner = 0u;
   stat->group = 0u;
   stat->size = static_cast<uint32_t>(entry.size);
   stat->entryId = 0u;
   stat->created = 0;
   stat->modified = 0;
}


int32_t
FSADevice::mapHandle(FileHandle file)
{
   auto index = 0;

   for (index = 0; index < mHandles.size(); ++index) {
      if (mHandles[index].type == Handle::Unused) {
         mHandles[index].type = Handle::File;
         mHandles[index].file = file;
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.push_back({ Handle::File, file, nullptr });
   }

   return index + 1;
}


int32_t
FSADevice::mapHandle(FolderHandle folder)
{
   auto index = 0;

   for (index = 0; index < mHandles.size(); ++index) {
      if (mHandles[index].type == Handle::Unused) {
         mHandles[index].type = Handle::Folder;
         mHandles[index].folder = folder;
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.push_back({ Handle::Folder, nullptr, folder });
   }

   return index + 1;
}


FSAStatus
FSADevice::mapHandle(int32_t index,
                     FileHandle &file)
{
   if (index <= 0 || index > mHandles.size()) {
      return FSAStatus::InvalidFileHandle;
   }

   auto &handle = mHandles[index - 1];

   if (handle.type != Handle::File) {
      return FSAStatus::NotFile;
   }

   file = handle.file;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::mapHandle(int32_t index,
                     FolderHandle &folder)
{
   if (index <= 0 || index > mHandles.size()) {
      return FSAStatus::InvalidDirHandle;
   }

   auto &handle = mHandles[index - 1];

   if (handle.type != Handle::Folder) {
      return FSAStatus::NotDir;
   }

   folder = handle.folder;
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
   handle.file = nullptr;
   handle.folder = nullptr;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::changeDir(phys_ptr<FSARequestChangeDir> request)
{
   mWorkingPath = translatePath(phys_addrof(request->path));
   return FSAStatus::OK;
}


FSAStatus
FSADevice::closeDir(phys_ptr<FSARequestCloseDir> request)
{
   auto dir = FolderHandle {};
   auto error = mapHandle(request->handle, dir);

   if (error < 0) {
      return error;
   }

   dir->close();
   return removeHandle(request->handle, Handle::Folder);
}


FSAStatus
FSADevice::closeFile(phys_ptr<FSARequestCloseFile> request)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->close();
   return removeHandle(request->handle, Handle::File);
}


FSAStatus
FSADevice::flushFile(phys_ptr<FSARequestFlushFile> request)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->flush();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::flushQuota(phys_ptr<FSARequestFlushQuota> request)
{
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getCwd(phys_ptr<FSAResponseGetCwd> response)
{
   std::strncpy(phys_addrof(response->path).getRawPointer(),
                mWorkingPath.path().c_str(),
                response->path.size() - 1);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getInfoByQuery(phys_ptr<FSARequestGetInfoByQuery> request,
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
      auto result = mFS->findEntry(path);

      if (!result) {
         return translateError(result);
      }

      translateStat(result.value(), phys_addrof(response->stat));
      break;
   }
   default:
      return FSAStatus::UnsupportedCmd;
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::getPosFile(phys_ptr<FSARequestGetPosFile> request,
                      phys_ptr<FSAResponseGetPosFile> response)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   response->pos = static_cast<uint32_t>(file->tell());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::isEof(phys_ptr<FSARequestIsEof> request)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   if (file->eof()) {
      error = FSAStatus::EndOfFile;
   } else {
      error = FSAStatus::OK;
   }

   return error;
}


FSAStatus
FSADevice::makeDir(phys_ptr<FSARequestMakeDir> request)
{
   auto path = translatePath(phys_addrof(request->path));

   if (!mFS->makeFolder(path)) {
      return FSAStatus::PermissionError;
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::mount(phys_ptr<FSARequestMount> request)
{
   auto devicePath = translatePath(phys_addrof(request->path));
   auto targetPath = translatePath(phys_addrof(request->target));

   // Device is already mounted as a filesystem node, so we just make a link to it
   auto result = mFS->makeLink(targetPath, devicePath);

   return translateError(result);
}


FSAStatus
FSADevice::openDir(phys_ptr<FSARequestOpenDir> request,
                   phys_ptr<FSAResponseOpenDir> response)
{
   auto path = translatePath(phys_addrof(request->path));
   auto result = mFS->openFolder(path);

   if (!result) {
      return translateError(result);
   }

   response->handle = mapHandle(result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::openFile(phys_ptr<FSARequestOpenFile> request,
                    phys_ptr<FSAResponseOpenFile> response)
{
   auto path = translatePath(phys_addrof(request->path));
   auto mode = translateMode(phys_addrof(request->mode));
   auto result = mFS->openFile(path, mode);

   if (!result) {
      return translateError(result);
   }

   response->handle = mapHandle(result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readDir(phys_ptr<FSARequestReadDir> request,
                   phys_ptr<FSAResponseReadDir> response)
{
   auto entry = FolderEntry {};
   auto folder = FolderHandle {};
   auto error = mapHandle(request->handle, folder);

   if (error < 0) {
      return error;
   }

   if (!folder->read(entry)) {
      return FSAStatus::EndOfDir;
   }

   translateStat(entry, phys_addrof(response->entry.stat));
   std::strcpy(phys_addrof(response->entry.name).getRawPointer(), entry.name.c_str());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readFile(phys_ptr<FSARequestReadFile> request,
                    phys_ptr<uint8_t> buffer,
                    uint32_t bufferLen)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   if (request->readFlags & FSAReadFlag::ReadWithPos) {
      file->seek(request->pos);
   }

   auto elemsRead = file->read(buffer.getRawPointer(), request->size, request->count);
   auto bytesRead = elemsRead * request->size;
   return static_cast<FSAStatus>(bytesRead);
}


FSAStatus
FSADevice::remove(phys_ptr<FSARequestRemove> request)
{
   auto path = translatePath(phys_addrof(request->path));
   return translateError(mFS->remove(path));
}


FSAStatus
FSADevice::rename(phys_ptr<FSARequestRename> request)
{
   auto src = translatePath(phys_addrof(request->oldPath));
   auto dst = translatePath(phys_addrof(request->newPath));
   auto result = mFS->move(src, dst);

   if (!result) {
      return translateError(result);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::rewindDir(phys_ptr<FSARequestRewindDir> request)
{
   auto folder = FolderHandle {};
   auto error = mapHandle(request->handle, folder);

   if (error < 0) {
      return error;
   }

   folder->rewind();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::setPosFile(phys_ptr<FSARequestSetPosFile> request)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->seek(request->pos);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::statFile(phys_ptr<FSARequestStatFile> request,
                    phys_ptr<FSAResponseStatFile> response)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   std::memset(phys_addrof(response->stat).getRawPointer(), 0, sizeof(response->stat));
   response->stat.flags = static_cast<FSAStatFlags>(0);
   response->stat.permission = 0x660u;
   response->stat.owner = 0u;
   response->stat.group = 0u;
   response->stat.size = static_cast<uint32_t>(file->size());
   response->stat.entryId = 0u;
   response->stat.created = 0;
   response->stat.modified = 0;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::truncateFile(phys_ptr<FSARequestTruncateFile> request)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->truncate();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::unmount(phys_ptr<FSARequestUnmount> request)
{
   auto path = translatePath(phys_addrof(request->path));
   auto result = mFS->remove(path);
   return translateError(result);
}


FSAStatus
FSADevice::writeFile(phys_ptr<FSARequestWriteFile> request,
                     phys_ptr<const uint8_t> buffer,
                     uint32_t bufferLen)
{
   auto file = FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
      file->seek(request->pos);
   }

   auto elemsWritten = file->write(buffer.getRawPointer(), request->size, request->count);
   auto bytesWritten = elemsWritten * request->size;
   return static_cast<FSAStatus>(bytesWritten);
}

} // namespace ios::fs::internal
