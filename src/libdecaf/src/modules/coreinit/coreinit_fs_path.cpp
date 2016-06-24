#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_path.h"
#include "filesystem/filesystem.h"

namespace coreinit
{

static fs::Path
gWorkingPath = "/";


FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *buffer,
         uint32_t bufferSize,
         uint32_t flags)
{
   auto &path = gWorkingPath.path();
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
   internal::queueFsWork(client, block, asyncData, [=]() {
      gWorkingPath = coreinit::internal::translatePath(path);
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
translatePath(const char *path)
{
   if (path[0] == '/') {
      return path;
   } else {
      return gWorkingPath.join(path);
   }
}

} // namespace internal

} // namespace coreinit
