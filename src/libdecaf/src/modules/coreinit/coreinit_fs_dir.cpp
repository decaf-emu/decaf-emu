#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_dir.h"
#include "coreinit_fs_path.h"
#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"
#include <gsl.h>

namespace coreinit
{

FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirectoryHandle> *handle,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   auto pathLength = strlen(path);

   if (pathLength >= FSCmdBlock::MaxPathLength) {
      return FSStatus::FatalError;
   }

   std::memcpy(block->path, path, pathLength);
   block->path[pathLength] = 0;

   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();
      auto realPath = internal::translatePath(client, block->path);
      auto dir = fs->openFolder(realPath);

      if (!dir) {
         gLog->debug("Could not open directory '{}'", realPath.path());
         return FSStatus::NotFound;
      }

      *handle = client->addOpenDirectory(dir);
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirectoryHandle> *handle,
          uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSOpenDirAsync(client, block, path, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirectoryHandle handle,
                uint32_t flags,
                FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto directory = client->getOpenDirectory(handle);

      if (!directory) {
         return FSStatus::FatalError;
      }

      client->removeOpenDirectory(handle);
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirectoryHandle handle,
           uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSCloseDirAsync(client, block, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSMakeDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   auto pathLength = strlen(path);

   if (pathLength >= FSCmdBlock::MaxPathLength) {
      return FSStatus::FatalError;
   }

   std::memcpy(block->path, path, pathLength);
   block->path[pathLength] = 0;

   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();

      if (!fs->makeFolder(internal::translatePath(client, block->path))) {
         return FSStatus::FatalError;
      }

      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSMakeDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSMakeDirAsync(client, block, path, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirectoryHandle handle,
               FSDirectoryEntry *entry,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto directory = client->getOpenDirectory(handle);
      auto folderEntry = fs::FolderEntry{};

      if (!directory) {
         return FSStatus::FatalError;
      }

      if (!directory->read(folderEntry)) {
         return FSStatus::End;
      }

      // Clear entry
      memset(entry, 0, sizeof(FSDirectoryEntry));

      // Copy name
      auto nameSize = std::min<size_t>(folderEntry.name.size(), 255);
      std::memcpy(entry->name, folderEntry.name.c_str(), nameSize);
      entry->name[nameSize] = '\0';

      // Copy data
      entry->info.size = gsl::narrow_cast<uint32_t>(folderEntry.size);

      if (folderEntry.type == fs::FolderEntry::Folder) {
         entry->info.flags |= FSStat::Directory;
      }

      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirectoryHandle handle,
          FSDirectoryEntry *entry,
          uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSReadDirAsync(client, block, handle, entry, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSRewindDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSDirectoryHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto directory = client->getOpenDirectory(handle);

      if (!directory) {
         return FSStatus::FatalError;
      }

      directory->rewind();
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSRewindDir(FSClient *client,
            FSCmdBlock *block,
            FSDirectoryHandle handle,
            uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSRewindDirAsync(client, block, handle, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSRemoveAsync(FSClient *client,
              FSCmdBlock *block,
              const char *path,
              uint32_t flags,
              FSAsyncData *asyncData)
{
   auto pathLength = strlen(path);

   if (pathLength >= FSCmdBlock::MaxPathLength) {
      return FSStatus::FatalError;
   }

   std::memcpy(block->path, path, pathLength);
   block->path[pathLength] = 0;

   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();

      if (!fs->remove(block->path)) {
         return FSStatus::NotFound;
      }

      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSRemove(FSClient *client,
         FSCmdBlock *block,
         const char *path,
         uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSRemoveAsync(client, block, path, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSRenameAsync(FSClient *client,
              FSCmdBlock *block,
              const char *src,
              const char *dst,
              uint32_t flags,
              FSAsyncData *asyncData)
{
   if (!src || !dst) {
      return FSStatus::FatalError;
   }

   auto srcLength = strlen(src);
   auto dstLength = strlen(dst);

   if (srcLength >= FSCmdBlock::MaxPathLength) {
      return FSStatus::FatalError;
   }

   if (dstLength >= FSCmdBlock::MaxPathLength) {
      return FSStatus::FatalError;
   }

   std::memcpy(block->path, src, srcLength);
   block->path[srcLength] = 0;

   std::memcpy(block->path2, dst, dstLength);
   block->path2[dstLength] = 0;

   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();
      auto srcPath = internal::translatePath(client, block->path);
      auto dstPath = internal::translatePath(client, block->path2);

      return internal::translateError(fs->move(srcPath, dstPath));
   });

   return FSStatus::OK;
}

FSStatus
FSRename(FSClient *client,
         FSCmdBlock *block,
         const char *src,
         const char *dst,
         uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSRenameAsync(client, block, src, dst, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSGetFreeSpaceSizeAsync(FSClient *client,
                        FSCmdBlock *block,
                        const char *path,
                        uint64_t *freeSpace,
                        uint32_t flags,
                        FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      *freeSpace = 0x00FFFFFF;
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSGetFreeSpaceSize(FSClient *client,
                   FSCmdBlock *block,
                   const char *path,
                   uint64_t *freeSpace,
                   uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSGetFreeSpaceSizeAsync(client, block, path, freeSpace, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSFlushQuotaAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSFlushQuota(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSFlushQuotaAsync(client, block, path, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

} // namespace coreinit
