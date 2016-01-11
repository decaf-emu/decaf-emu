#include <gsl.h>
#include <mutex>
#include "coreinit.h"
#include "coreinit_fs.h"
#include "coreinit_memory.h"
#include "filesystem/filesystem.h"
#include "system.h"
#include "utils/wfunc_call.h"

struct FSFile
{
   FSFile() :
      file(nullptr),
      handle(-1)
   {
   }

   FSFile(fs::File *file) :
      file(file),
      handle(-1)
   {
   }

   fs::File *file;
   FSFileHandle handle;
};

class FSClient
{
public:
   FSClient()
   {
      // Let's just ensure there is never a file handle 0 just in case
      mOpenFiles.push_back(nullptr);
   }

   FSFileHandle addFile(fs::FileHandle *file)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      // Try use an existing slot
      for (auto i = 1; i < mOpenFiles.size(); ++i) {
         if (mOpenFiles[i] == nullptr) {
            mOpenFiles[i] = file;
            return i;
         }
      }

      // Add a new slot
      auto handle = static_cast<FSFileHandle>(mOpenFiles.size());
      mOpenFiles.push_back(file);
      return handle;
   }

   FSDirectoryHandle addDirectory(fs::FolderHandle *folder)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      // Try use an existing slot
      for (auto i = 1; i < mOpenFolders.size(); ++i) {
         if (mOpenFolders[i] == nullptr) {
            mOpenFolders[i] = folder;
            return i;
         }
      }

      // Add a new slot
      auto handle = static_cast<FSFileHandle>(mOpenFolders.size());
      mOpenFolders.push_back(folder);
      return handle;
   }

   fs::FileHandle *getFile(FSFileHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFiles.size()) {
         return nullptr;
      }

      return mOpenFiles[handle];
   }

   fs::FolderHandle *getDirectory(FSDirectoryHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFolders.size()) {
         return nullptr;
      }

      return mOpenFolders[handle];
   }

   void removeDirectory(FSDirectoryHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFolders.size()) {
         return;
      } else {
         if (mOpenFolders[handle]) {
            mOpenFolders[handle]->close();
         }

         mOpenFolders[handle] = nullptr;
      }
   }

   void removeFile(FSFileHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFiles.size()) {
         return;
      } else {
         if (mOpenFiles[handle]) {
            mOpenFiles[handle]->close();
         }

         mOpenFiles[handle] = nullptr;
      }
   }

private:
   std::mutex mMutex;
   std::vector<fs::FileHandle *> mOpenFiles;
   std::vector<fs::FolderHandle *> mOpenFolders;
};

static_assert(sizeof(FSClient) < 0x1700, "FSClient must be less than 0x1700 bytes");

static fs::Path
gWorkingPath = "/vol/code";

static std::vector<FSClient*>
gClients;


fs::Path
translatePath(const char *path)
{
   if (path[0] == '/') {
      return path;
   } else {
      return gWorkingPath.join(path);
   }
}


void
FSInit()
{
}


void
FSShutdown()
{
}


FSStatus
FSAddClient(FSClient *client,
            uint32_t flags)
{
   return FSAddClientEx(client, 0, flags);
}

FSStatus
FSAddClientEx(FSClient *client,
              uint32_t unk,
              uint32_t flags)
{
   new(client) FSClient();
   gClients.push_back(client);
   return FSStatus::OK;
}


FSStatus
FSDelClient(FSClient *client,
            uint32_t flags)
{
   client->~FSClient();
   gClients.erase(std::remove(gClients.begin(), gClients.end(), client), gClients.end());
   return FSStatus::OK;
}


uint32_t
FSGetClientNum()
{
   return static_cast<uint32_t>(gClients.size());
}


void
FSInitCmdBlock(FSCmdBlock *block)
{
}


FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 FSPriority priority)
{
   return FSStatus::OK;
}


void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info)
{
   // TODO: FSSetStateChangeNotification
}

FSVolumeState
FSGetVolumeState(FSClient *client)
{
   return FSVolumeState::Ready;
}

FSError
FSGetLastErrorCodeForViewer(FSClient *client)
{
   // TODO: Return legit error code
   return FSError::OutOfResources;
}


FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *path,
           const char *mode,
           be_val<FSFileHandle> *outHandle,
           uint32_t flags)
{
   // TODO: Parse open mode
   auto fs = gSystem.getFileSystem();
   auto file = fs->openFile(translatePath(path), fs::File::Read);

   if (!file) {
      return FSStatus::NotFound;
   }

   *outHandle = client->addFile(file);
   return FSStatus::OK;
}


FSStatus
FSOpenFileAsync(FSClient *client,
                FSCmdBlock *block,
                const char *path,
                const char *mode,
                be_val<FSFileHandle> *outHandle,
                uint32_t flags,
                FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSOpenFile(client, block, path, mode, outHandle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirectoryHandle> *handle,
          uint32_t flags)
{
   auto fs = gSystem.getFileSystem();
   auto dir = fs->openFolder(translatePath(path));

   if (!dir) {
      return FSStatus::NotFound;
   }

   *handle = client->addDirectory(dir);
   return FSStatus::OK;
}


FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirectoryHandle> *handle,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSOpenDir(client, block, path, handle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirectoryHandle handle,
          FSDirectoryEntry *entry,
          uint32_t flags)
{
   auto directory = client->getDirectory(handle);
   fs::FolderEntry folderEntry;

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
}


FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirectoryHandle handle,
               FSDirectoryEntry *entry,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSReadDir(client, block, handle, entry, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirectoryHandle handle,
           uint32_t flags)
{
   auto directory = client->getDirectory(handle);

   if (!directory) {
      return FSStatus::FatalError;
   }

   directory->close();
   client->removeDirectory(handle);
   return FSStatus::OK;
}


FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirectoryHandle handle,
                uint32_t flags,
                FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSCloseDir(client, block, handle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *stat,
          uint32_t flags)
{
   fs::FolderEntry entry;
   auto fs = gSystem.getFileSystem();
   auto file = fs->findEntry(translatePath(path), entry);

   if (!file) {
      return FSStatus::NotFound;
   }

   memset(stat, 0, sizeof(FSStat));
   stat->size = gsl::narrow_cast<uint32_t>(entry.size);
   return FSStatus::OK;
}


FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *stat,
               uint32_t flags,
               FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSGetStat(client, block, path, stat, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *stat,
              uint32_t flags)
{
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   memset(stat, 0, sizeof(FSStat));
   stat->size = static_cast<uint32_t>(file->size());
   return FSStatus::OK;
}


FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *stat,
                   uint32_t flags,
                   FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSGetStatFile(client, block, handle, stat, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
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
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   auto read = file->read(buffer, size * count);
   return static_cast<FSStatus>(read);
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
   assert(asyncData->callback);
   auto result = FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
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
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   auto read = file->read(buffer, size * count, position);
   return static_cast<FSStatus>(read);
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
   assert(asyncData->callback);
   auto result = FSReadFileWithPos(client, block, buffer, size, count, position, handle, unk1, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             be_val<uint32_t> *pos,
             uint32_t flags)
{
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   *pos = file->tell();
   return FSStatus::OK;
}


FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  be_val<uint32_t> *pos,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSGetPosFile(client, block, handle, pos, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t pos,
             uint32_t flags)
{
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   file->seek(pos);
   return FSStatus::OK;
}


FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  uint32_t pos,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSSetPosFile(client, block, handle, pos, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


FSStatus
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            uint32_t flags)
{
   auto file = client->getFile(handle);

   if (!file) {
      return FSStatus::FatalError;
   }

   file->close();
   client->removeFile(handle);
   return FSStatus::OK;
}


FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSCloseFile(client, block, handle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}


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
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            uint32_t flags)
{
   gWorkingPath = translatePath(path);
   return FSStatus::OK;
}


FSStatus
FSChangeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 const char *path,
                 uint32_t flags,
                 FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSChangeDir(client, block, path, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

void
CoreInit::registerFileSystemFunctions()
{
   RegisterKernelFunction(FSInit);
   RegisterKernelFunction(FSShutdown);
   RegisterKernelFunction(FSAddClient);
   RegisterKernelFunction(FSAddClientEx);
   RegisterKernelFunction(FSDelClient);
   RegisterKernelFunction(FSGetClientNum);
   RegisterKernelFunction(FSInitCmdBlock);
   RegisterKernelFunction(FSSetCmdPriority);
   RegisterKernelFunction(FSSetStateChangeNotification);
   RegisterKernelFunction(FSGetVolumeState);
   RegisterKernelFunction(FSGetLastErrorCodeForViewer);

   RegisterKernelFunction(FSGetCwd);
   RegisterKernelFunction(FSChangeDir);
   RegisterKernelFunction(FSChangeDirAsync);

   RegisterKernelFunction(FSGetStat);
   RegisterKernelFunction(FSGetStatAsync);
   RegisterKernelFunction(FSGetStatFile);
   RegisterKernelFunction(FSGetStatFileAsync);
   RegisterKernelFunction(FSOpenDir);
   RegisterKernelFunction(FSOpenDirAsync);
   RegisterKernelFunction(FSReadDir);
   RegisterKernelFunction(FSReadDirAsync);
   RegisterKernelFunction(FSCloseDir);
   RegisterKernelFunction(FSCloseDirAsync);
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSGetPosFileAsync);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSSetPosFileAsync);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
}
