#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_path.h"
#include "coreinit_fs_file.h"
#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"

namespace coreinit
{

static fs::File::OpenMode
parseOpenMode(const std::string &str)
{
   auto mode = 0u;

   if (str.find('r') != std::string::npos) {
      mode |= fs::File::Read;
   }

   if (str.find('w') != std::string::npos) {
      mode |= fs::File::Write;
   }

   if (str.find('a') != std::string::npos) {
      mode |= fs::File::Append;
   }

   if (str.find('+') != std::string::npos) {
      mode |= fs::File::Update;
   }

   return static_cast<fs::File::OpenMode>(mode);
}

FSStatus
FSOpenFileAsync(FSClient *client,
                FSCmdBlock *block,
                const char *pathStr,
                const char *modeStr,
                be_val<FSFileHandle> *outHandle,
                uint32_t flags,
                FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();
      auto path = coreinit::internal::translatePath(pathStr);
      auto mode = parseOpenMode(modeStr);

      if (!mode) {
         return FSStatus::AccessError;
      }

      auto fh = fs->openFile(path, mode);

      // Try to create the file if not exists in Write or Append mode
      if (!fh && ((mode & fs::File::Write) || (mode & fs::File::Append))) {
         auto file = fs->makeFile(path);

         if (file) {
            fh = file->open(mode);
         }
      }

      // File not found
      if (!fh) {
         return FSStatus::NotFound;
      }

      *outHandle = client->addOpenFile(fh);
      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *pathStr,
           const char *modeStr,
           be_val<FSFileHandle> *outHandle,
           uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSOpenFileAsync(client, block, pathStr, modeStr, outHandle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      client->removeOpenFile(handle);
      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSCloseFileAsync(client, block, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSReadFileAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t *buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                uint32_t unk1,
                uint32_t flags,
                FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      auto read = file->read(buffer, size, count);
      return static_cast<FSStatus>(read);
   });
   return FSStatus::OK;
}

FSStatus
FSReadFile(FSClient *client,
           FSCmdBlock *block,
           uint8_t *buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           uint32_t unk1,
           uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSReadFileAsync(client, block, buffer, size, count, handle, unk1, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSReadFileWithPosAsync(FSClient *client,
                       FSCmdBlock *block,
                       uint8_t *buffer,
                       uint32_t size,
                       uint32_t count,
                       uint32_t position,
                       FSFileHandle handle,
                       uint32_t unk1,
                       uint32_t flags,
                       FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      auto read = file->read(buffer, size, count, position);
      return static_cast<FSStatus>(read);
   });
   return FSStatus::OK;
}

FSStatus
FSReadFileWithPos(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t *buffer,
                  uint32_t size,
                  uint32_t count,
                  uint32_t position,
                  FSFileHandle handle,
                  uint32_t unk1,
                  uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSReadFileWithPosAsync(client, block, buffer, size, count, position, handle, unk1, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSWriteFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t *buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 uint32_t unk1,
                 uint32_t flags,
                 FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      auto wrote = file->write(buffer, size, count);
      return static_cast<FSStatus>(wrote);
   });
   return FSStatus::OK;
}

FSStatus
FSWriteFile(FSClient *client,
            FSCmdBlock *block,
            uint8_t *buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            uint32_t unk1,
            uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSWriteFileAsync(client, block, buffer, size, count, handle, unk1, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSWriteFileWithPosAsync(FSClient *client,
                        FSCmdBlock *block,
                        uint8_t *buffer,
                        uint32_t size,
                        uint32_t count,
                        uint32_t position,
                        FSFileHandle handle,
                        uint32_t unk1,
                        uint32_t flags,
                        FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      auto wrote = file->write(buffer, size, count, position);
      return static_cast<FSStatus>(wrote);
   });
   return FSStatus::OK;
}

FSStatus
FSWriteFileWithPos(FSClient *client,
                   FSCmdBlock *block,
                   uint8_t *buffer,
                   uint32_t size,
                   uint32_t count,
                   uint32_t position,
                   FSFileHandle handle,
                   uint32_t unk1,
                   uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSWriteFileWithPosAsync(client, block, buffer, size, count, position, handle, unk1, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSIsEofAsync(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t flags,
             FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      if (file->eof()) {
         return FSStatus::End;
      }

      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSIsEof(FSClient *client,
        FSCmdBlock *block,
        FSFileHandle handle,
        uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSIsEofAsync(client, block, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  be_val<uint32_t> *pos,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      *pos = file->tell();
      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             be_val<uint32_t> *pos,
             uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSGetPosFileAsync(client, block, handle, pos, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  uint32_t pos,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      file->seek(pos);
      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t pos,
             uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSSetPosFileAsync(client, block, handle, pos, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSTruncateFileAsync(FSClient *client,
                    FSCmdBlock *block,
                    FSFileHandle handle,
                    uint32_t flags,
                    FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      file->truncate();
      return FSStatus::OK;
   });
   return FSStatus::OK;
}

FSStatus
FSTruncateFile(FSClient *client,
               FSCmdBlock *block,
               FSFileHandle handle,
               uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSTruncateFileAsync(client, block, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

} // namespace coreinit
