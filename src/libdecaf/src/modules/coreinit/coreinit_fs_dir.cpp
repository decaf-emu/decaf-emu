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
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();
      auto dir = fs->openFolder(coreinit::internal::translatePath(path));

      if (!dir) {
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
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto fs = kernel::getFileSystem();

      if (!fs->makeFolder(coreinit::internal::translatePath(path))) {
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

} // namespace coreinit
