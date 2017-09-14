#include "fsa_device.h"
#include "kernel/kernel_filesystem.h"

#include <cstring>

namespace ios
{

namespace dev
{

namespace fsa
{

Error
FSADevice::open(OpenMode mode)
{
   mFS = kernel::getFileSystem();
   return Error::OK;
}


Error
FSADevice::close()
{
   return Error::OK;
}


Error
FSADevice::read(void *buffer,
                size_t length)
{
   return static_cast<Error>(FSAStatus::UnsupportedCmd);
}


Error
FSADevice::write(void *buffer,
                 size_t length)
{
   return static_cast<Error>(FSAStatus::UnsupportedCmd);
}


Error
FSADevice::ioctl(uint32_t cmd,
                 void *inBuf,
                 size_t inLen,
                 void *outBuf,
                 size_t outLen)
{
   auto request = reinterpret_cast<FSARequest *>(inBuf);
   auto response = reinterpret_cast<FSAResponse *>(outBuf);
   auto result = FSAStatus::OK;
   decaf_check(inLen == sizeof(FSARequest));
   decaf_check(outLen == sizeof(FSAResponse));

   if (request->emulatedError < 0) {
      return static_cast<Error>(request->emulatedError.value());
   }

   switch (static_cast<FSACommand>(cmd)) {
   case FSACommand::ChangeDir:
      result = changeDir(&request->changeDir);
      break;
   case FSACommand::CloseDir:
      result = closeDir(&request->closeDir);
      break;
   case FSACommand::CloseFile:
      result = closeFile(&request->closeFile);
      break;
   case FSACommand::FlushFile:
      result = flushFile(&request->flushFile);
      break;
   case FSACommand::FlushQuota:
      result = flushQuota(&request->flushQuota);
      break;
   case FSACommand::GetCwd:
      result = getCwd(&response->getCwd);
      break;
   case FSACommand::GetInfoByQuery:
      result = getInfoByQuery(&request->getInfoByQuery, &response->getInfoByQuery);
      break;
   case FSACommand::GetPosFile:
      result = getPosFile(&request->getPosFile, &response->getPosFile);
      break;
   case FSACommand::IsEof:
      result = isEof(&request->isEof);
      break;
   case FSACommand::MakeDir:
      result = makeDir(&request->makeDir);
      break;
   case FSACommand::OpenDir:
      result = openDir(&request->openDir, &response->openDir);
      break;
   case FSACommand::OpenFile:
      result = openFile(&request->openFile, &response->openFile);
      break;
   case FSACommand::ReadDir:
      result = readDir(&request->readDir, &response->readDir);
      break;
   case FSACommand::Remove:
      result = remove(&request->remove);
      break;
   case FSACommand::Rename:
      result = rename(&request->rename);
      break;
   case FSACommand::RewindDir:
      result = rewindDir(&request->rewindDir);
      break;
   case FSACommand::SetPosFile:
      result = setPosFile(&request->setPosFile);
      break;
   case FSACommand::StatFile:
      result = statFile(&request->statFile, &response->statFile);
      break;
   case FSACommand::TruncateFile:
      result = truncateFile(&request->truncateFile);
      break;
   default:
      result = FSAStatus::UnsupportedCmd;
   }

   return static_cast<Error>(result);
}


Error
FSADevice::ioctlv(uint32_t cmd,
                  size_t vecIn,
                  size_t vecOut,
                  IoctlVec *vec)
{
   auto request = phys_ptr<FSARequest> { vec[0].paddr };
   auto result = FSAStatus::OK;
   decaf_check(vec[0].len == sizeof(FSARequest));

   if (request->emulatedError < 0) {
      return static_cast<Error>(request->emulatedError.value());
   }

   switch (static_cast<FSACommand>(cmd)) {
   case FSACommand::ReadFile:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 2);
      auto buffer = phys_ptr<uint8_t> { vec[1].paddr };
      auto length = vec[1].len;
      result = readFile(&request->readFile, buffer.getRawPointer(), length);
      break;
   }
   case FSACommand::WriteFile:
   {
      decaf_check(vecIn == 2);
      decaf_check(vecOut == 1);
      auto buffer = phys_ptr<uint8_t> { vec[1].paddr };
      auto length = vec[1].len;
      result = writeFile(&request->writeFile, buffer.getRawPointer(), length);
      break;
   }
   case FSACommand::Mount:
   {
      decaf_check(vecIn == 2);
      decaf_check(vecOut == 1);
      result = mount(&request->mount);
      break;
   }
   default:
      result = FSAStatus::UnsupportedCmd;
   }

   return static_cast<Error>(result);
}


FSAStatus
FSADevice::translateError(fs::Error error) const
{
   switch (error) {
   case fs::Error::OK:
      return FSAStatus::OK;
   case fs::Error::UnsupportedOperation:
      return FSAStatus::UnsupportedCmd;
   case fs::Error::NotFound:
      return FSAStatus::NotFound;
   case fs::Error::AlreadyExists:
      return FSAStatus::AlreadyExists;
   case fs::Error::InvalidPermission:
      return FSAStatus::PermissionError;
   case fs::Error::NotFile:
      return FSAStatus::NotFile;
   case fs::Error::NotDirectory:
      return FSAStatus::NotDir;
   default:
      return FSAStatus::UnsupportedCmd;
   }
}


fs::Path
FSADevice::translatePath(const char *path) const
{
   if (path[0] == '/') {
      return path;
   } else {
      return mWorkingPath.join(path);
   }
}


fs::File::OpenMode
FSADevice::translateMode(const char *mode) const
{
   auto result = 0u;

   if (std::strchr(mode, 'r')) {
      result |= fs::File::Read;
   }

   if (std::strchr(mode, 'w')) {
      result |= fs::File::Write;
   }

   if (std::strchr(mode, 'a')) {
      result |= fs::File::Append;
   }

   if (std::strchr(mode, '+')) {
      result |= fs::File::Update;
   }

   return static_cast<fs::File::OpenMode>(result);
}


void
FSADevice::translateStat(const fs::FolderEntry &entry,
                         FSAStat *stat) const
{
   std::memset(stat, 0, sizeof(FSAStat));

   if (entry.type == fs::FolderEntry::Folder) {
      stat->flags = FSAStatFlags::Directory;
   } else {
      stat->flags = static_cast<FSAStatFlags>(0);
   }

   stat->permission = 0x660;
   stat->owner = 0;
   stat->group = 0;
   stat->size = static_cast<uint32_t>(entry.size);
   stat->entryId = 0;
   stat->created = 0;
   stat->modified = 0;
}


uint32_t
FSADevice::mapHandle(fs::FileHandle file)
{
   auto index = 0u;

   for (index = 0u; index < mHandles.size(); ++index) {
      if (mHandles[index].type == FSAHandle::Unused) {
         mHandles[index].type = FSAHandle::File;
         mHandles[index].file = file;
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.push_back({ FSAHandle::File, file, nullptr });
   }

   return index + 1;
}


uint32_t
FSADevice::mapHandle(fs::FolderHandle folder)
{
   auto index = 0u;

   for (index = 0u; index < mHandles.size(); ++index) {
      if (mHandles[index].type == FSAHandle::Unused) {
         mHandles[index].type = FSAHandle::Folder;
         mHandles[index].folder = folder;
         break;
      }
   }

   if (index == mHandles.size()) {
      mHandles.push_back({ FSAHandle::Folder, nullptr, folder });
   }

   return index + 1;
}


FSAStatus
FSADevice::mapHandle(uint32_t index,
                     fs::FileHandle &file)
{
   if (index == 0 || index > mHandles.size()) {
      return FSAStatus::InvalidFileHandle;
   }

   auto &handle = mHandles[index - 1];

   if (handle.type != FSAHandle::File) {
      return FSAStatus::NotFile;
   }

   file = handle.file;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::mapHandle(uint32_t index,
                     fs::FolderHandle &folder)
{
   if (index == 0 || index > mHandles.size()) {
      return FSAStatus::InvalidDirHandle;
   }

   auto &handle = mHandles[index - 1];

   if (handle.type != FSAHandle::Folder) {
      return FSAStatus::NotDir;
   }

   folder = handle.folder;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::removeHandle(uint32_t index,
                        FSAHandle::Type type)
{
   if (index == 0 || index > mHandles.size()) {
      if (type == FSAHandle::File) {
         return FSAStatus::InvalidFileHandle;
      } else {
         return FSAStatus::InvalidDirHandle;
      }
   }

   auto &handle = mHandles[index - 1];

   if (handle.type != type) {
      if (type == FSAHandle::File) {
         return FSAStatus::NotFile;
      } else {
         return FSAStatus::NotDir;
      }
   }

   handle.type = FSAHandle::Unused;
   handle.file = nullptr;
   handle.folder = nullptr;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::changeDir(FSARequestChangeDir *request)
{
   mWorkingPath = translatePath(request->path);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::closeDir(FSARequestCloseDir *request)
{
   auto dir = fs::FolderHandle {};
   auto error = mapHandle(request->handle, dir);

   if (error < 0) {
      return error;
   }

   dir->close();
   return removeHandle(request->handle, FSAHandle::Folder);
}


FSAStatus
FSADevice::closeFile(FSARequestCloseFile *request)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->close();
   return removeHandle(request->handle, FSAHandle::File);
}


FSAStatus
FSADevice::flushFile(FSARequestFlushFile *request)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->flush();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::flushQuota(FSARequestFlushQuota *request)
{
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getCwd(FSAResponseGetCwd *response)
{
   std::strncpy(response->path, mWorkingPath.path().c_str(), FSAMaxPathLength);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::getInfoByQuery(FSARequestGetInfoByQuery *request,
                          FSAResponseGetInfoByQuery *response)
{
   auto path = translatePath(request->path);

   switch (request->type) {
   case FSAQueryInfoType::FreeSpaceSize:
   {
      response->freeSpaceSize = 128 * 1024 * 1024;
      break;
   }
   case FSAQueryInfoType::Stat:
   {
      auto result = mFS->findEntry(path);

      if (!result) {
         return translateError(result);
      }

      translateStat(result.value(), &response->stat);
      break;
   }
   default:
      return FSAStatus::UnsupportedCmd;
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::getPosFile(FSARequestGetPosFile *request,
                      FSAResponseGetPosFile *response)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   response->pos = static_cast<uint32_t>(file->tell());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::isEof(FSARequestIsEof *request)
{
   auto file = fs::FileHandle {};
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
FSADevice::makeDir(FSARequestMakeDir *request)
{
   auto path = translatePath(request->path);

   if (!mFS->makeFolder(path)) {
      return FSAStatus::PermissionError;
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::mount(FSARequestMount *request)
{
   auto devicePath = translatePath(request->path);
   auto targetPath = translatePath(request->target);

   // Device is already mounted as a filesystem node, so we just make a link to it
   auto result = mFS->makeLink(targetPath, devicePath);

   return translateError(result);
}


FSAStatus
FSADevice::openDir(FSARequestOpenDir *request,
                   FSAResponseOpenDir *response)
{
   auto path = translatePath(request->path);
   auto result = mFS->openFolder(path);

   if (!result) {
      return translateError(result);
   }

   response->handle = mapHandle(result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::openFile(FSARequestOpenFile *request,
                    FSAResponseOpenFile *response)
{
   auto path = translatePath(request->path);
   auto mode = translateMode(request->mode);
   auto result = mFS->openFile(path, mode);

   if (!result) {
      return translateError(result);
   }

   response->handle = mapHandle(result);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readDir(FSARequestReadDir *request,
                   FSAResponseReadDir *response)
{
   auto entry = fs::FolderEntry { };
   auto folder = fs::FolderHandle {};
   auto error = mapHandle(request->handle, folder);

   if (error < 0) {
      return error;
   }

   if (!folder->read(entry)) {
      return FSAStatus::EndOfDir;
   }

   translateStat(entry, &response->entry.stat);
   std::strcpy(response->entry.name, entry.name.c_str());
   return FSAStatus::OK;
}


FSAStatus
FSADevice::readFile(FSARequestReadFile *request,
                    uint8_t *buffer,
                    uint32_t bufferLen)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   if (request->readFlags & FSAReadFlag::ReadWithPos) {
      file->seek(request->pos);
   }

   auto elemsRead = file->read(buffer, request->size, request->count);
   auto bytesRead = elemsRead * request->size;
   return static_cast<FSAStatus>(bytesRead);
}


FSAStatus
FSADevice::remove(FSARequestRemove *request)
{
   auto path = translatePath(request->path);
   return translateError(mFS->remove(path));
}


FSAStatus
FSADevice::rename(FSARequestRename *request)
{
   auto src = translatePath(request->oldPath);
   auto dst = translatePath(request->newPath);
   auto result = mFS->move(src, dst);

   if (!result) {
      return translateError(result);
   }

   return FSAStatus::OK;
}


FSAStatus
FSADevice::rewindDir(FSARequestRewindDir *request)
{
   auto folder = fs::FolderHandle {};
   auto error = mapHandle(request->handle, folder);

   if (error < 0) {
      return error;
   }

   folder->rewind();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::setPosFile(FSARequestSetPosFile *request)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->seek(request->pos);
   return FSAStatus::OK;
}


FSAStatus
FSADevice::statFile(FSARequestStatFile *request,
                    FSAResponseStatFile *response)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   std::memset(&response->stat, 0, sizeof(response->stat));
   response->stat.flags = static_cast<FSAStatFlags>(0);
   response->stat.permission = 0x660;
   response->stat.owner = 0;
   response->stat.group = 0;
   response->stat.size = file->size();
   response->stat.entryId = 0;
   response->stat.created = 0;
   response->stat.modified = 0;
   return FSAStatus::OK;
}


FSAStatus
FSADevice::truncateFile(FSARequestTruncateFile *request)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   file->truncate();
   return FSAStatus::OK;
}


FSAStatus
FSADevice::unmount(FSARequestUnmount *request)
{
   auto path = translatePath(request->path);
   auto result = mFS->remove(path);
   return translateError(result);
}


FSAStatus
FSADevice::writeFile(FSARequestWriteFile *request,
                     const uint8_t *buffer,
                     uint32_t bufferLen)
{
   auto file = fs::FileHandle {};
   auto error = mapHandle(request->handle, file);

   if (error < 0) {
      return error;
   }

   if (request->writeFlags & FSAWriteFlag::WriteWithPos) {
      file->seek(request->pos);
   }

   auto elemsWritten = file->write(buffer, request->size, request->count);
   auto bytesWritten = elemsWritten * request->size;
   return static_cast<FSAStatus>(bytesWritten);
}

} // namespace fsa

} // namespace dev

} // namespace ios
