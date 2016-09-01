#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_path.h"
#include "filesystem/filesystem.h"

namespace coreinit
{

FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *buffer,
         uint32_t bufferSize,
         uint32_t flags)
{
   auto &path = client->getWorkingPath().path();
   auto size = path.size();

   if (size >= bufferSize) {
      return FSStatus::FatalError;
   }

   std::memcpy(buffer, path.c_str(), size);
   buffer[size] = '\0';
   return FSStatus::OK;
}

FSStatus
FSChangeDirAsync(FSClient *client,
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
   if (block->path[pathLength - 1] != '/') {
      block->path[pathLength] = '/';
      pathLength++;
   }
   block->path[pathLength] = 0;

   internal::queueFsWork(client, block, asyncData, [=]() {
      client->setWorkingPath(internal::translatePath(client, block->path));
      return FSStatus::OK;
   });

   return FSStatus::OK;
}

FSStatus
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            uint32_t flags)
{
   auto asyncData = internal::prepareSyncOp(client, block);
   FSChangeDirAsync(client, block, path, flags, asyncData);
   return internal::resolveSyncOp(client, block);
}

namespace internal
{

fs::Path
translatePath(FSClient *client,
              const char *path)
{
   if (path[0] == '/') {
      return path;
   } else {
      return client->getWorkingPath().join(path);
   }
}

} // namespace internal

} // namespace coreinit
