#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_path.h"
#include "coreinit_fs_stat.h"
#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"
#include <gsl.h>

namespace coreinit
{

FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *stat,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto entry = fs::FolderEntry {};
      auto fs = kernel::getFileSystem();
      auto file = fs->findEntry(coreinit::internal::translatePath(path), entry);

      if (!file) {
         return FSStatus::NotFound;
      }

      std::memset(stat, 0, sizeof(FSStat));
      stat->size = gsl::narrow_cast<uint32_t>(entry.size);
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *stat,
          uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSGetStatAsync(client, block, path, stat, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *stat,
                   uint32_t flags,
                   FSAsyncData *asyncData)
{
   internal::queueFsWork(client, block, asyncData, [=]() {
      auto file = client->getOpenFile(handle);

      if (!file) {
         return FSStatus::FatalError;
      }

      std::memset(stat, 0, sizeof(FSStat));
      stat->size = static_cast<uint32_t>(file->size());
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *stat,
              uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSGetStatFileAsync(client, block, handle, stat, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

} // namespace coreinit
