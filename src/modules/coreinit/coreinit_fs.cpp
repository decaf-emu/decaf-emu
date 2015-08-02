#include <mutex>
#include "coreinit.h"
#include "coreinit_fs.h"
#include "coreinit_memory.h"
#include "filesystem.h"
#include "system.h"
#include "interpreter.h"

struct FSFile
{
   FSFileHandle handle;
   FileSystem::File file;
};

class OpenFileManager
{
public:
   FSFileHandle
   add(FSFile *file)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      // Try use an existing slot
      for (auto i = 0; i < mOpenFiles.size(); ++i) {
         if (mOpenFiles[i] == nullptr) {
            file->handle = i;
            mOpenFiles[i] = file;
            return i;
         }
      }

      // Add a new slot
      file->handle = static_cast<FSFileHandle>(mOpenFiles.size());
      mOpenFiles.push_back(file);

      return file->handle;
   }

   FSFile *
   get(FSFileHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFiles.size()) {
         return nullptr;
      } else {
         return mOpenFiles[handle];
      }
   }

   void
   close(FSFileHandle handle)
   {
      std::lock_guard<std::mutex> lock(mMutex);

      if (handle > mOpenFiles.size()) {
         return;
      } else {
         delete mOpenFiles[handle];
         mOpenFiles[handle] = nullptr;
      }
   }

private:
   std::mutex mMutex;
   std::vector<FSFile *> mOpenFiles;
};

static OpenFileManager
gOpenFiles;

static std::string
gWorkingDirectory = "/vol/code";

static std::vector<FSClient*>
gClients;

void
FSInit()
{
}

void
FSShutdown()
{
}

FSStatus
FSAddClient(FSClient *client, uint32_t flags)
{
   gClients.push_back(client);
   return FSStatus::OK;
}

FSStatus
FSDelClient(FSClient *client, uint32_t flags)
{
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
FSSetCmdPriority(FSCmdBlock *block, FSPriority priority)
{
   return FSStatus::OK;
}

void
FSSetStateChangeNotification(FSClient *client, FSStateChangeInfo *info)
{
   // TODO: FSSetStateChangeNotification
}

FSStatus
FSOpenFile(FSClient *client, FSCmdBlock *block, const char *path, const char *mode, be_val<FSFileHandle> *outHandle, uint32_t flags)
{
   auto fs = gSystem.getFileSystem();
   auto file = fs->openFile(path, FileSystem::Input | FileSystem::Binary);

   if (!file) {
      return FSStatus::NotFound;
   }

   // Create FSFile and get handle
   auto fsFile = new FSFile {};
   fsFile->file = std::move(file);

   auto handle = gOpenFiles.add(fsFile);
   *outHandle = static_cast<uint32_t>(handle);

   return FSStatus::OK;
}

FSStatus
FSOpenFileAsync(FSClient *client, FSCmdBlock *block, const char *path, const char *mode, be_val<FSFileHandle> *outHandle, uint32_t flags, FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSOpenFile(client, block, path, mode, outHandle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

FSStatus
FSGetStat(FSClient *client, FSCmdBlock *block, const char *filepath, FSStat *stat, uint32_t flags)
{
   auto fs = gSystem.getFileSystem();
   auto file = fs->openFile(filepath, FileSystem::Input | FileSystem::Binary);

   if (!file) {
      return FSStatus::NotFound;
   }

   stat->flags = 0;
   stat->size = static_cast<uint32_t>(file->size());
   file->close();
   return FSStatus::OK;
}

FSStatus
FSGetStatFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSStat *stat, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   stat->flags = 0;
   stat->size = static_cast<uint32_t>(fsFile->file->size());
   return FSStatus::OK;
}

FSStatus
FSGetStatFileAsync(FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSStat *stat, uint32_t flags, FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSGetStatFile(client, block, handle, stat, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

FSStatus
FSReadFile(FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   auto read = fsFile->file->read(reinterpret_cast<char*>(buffer), size * count);
   return static_cast<FSStatus>(read);
}

FSStatus
FSReadFileAsync(FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags, FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

FSStatus
FSReadFileWithPos(FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, uint32_t position, FSFileHandle handle, uint32_t unk1, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   fsFile->file->seek(position);
   auto read = fsFile->file->read(reinterpret_cast<char*>(buffer), size * count);
   return static_cast<FSStatus>(read);
}

FSStatus
FSReadFileWithPosAsync(FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, uint32_t position, FSFileHandle handle, uint32_t unk1, uint32_t flags, FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSReadFileWithPos(client, block, buffer, size, count, position, handle, unk1, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

FSStatus
FSGetPosFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, be_val<uint32_t> *pos, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   *pos = fsFile->file->tell();
   return FSStatus::OK;
}

FSStatus
FSSetPosFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t pos, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   fsFile->file->seek(pos);
   return FSStatus::OK;
}

FSStatus
FSCloseFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   gOpenFiles.close(handle);
   return FSStatus::OK;
}

FSStatus
FSCloseFileAsync(FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t flags, FSAsyncData *asyncData)
{
   assert(asyncData->callback);
   auto result = FSCloseFile(client, block, handle, flags);
   FSAsyncCallback cb = static_cast<uint32_t>(asyncData->callback);
   cb(client, block, result, asyncData->param);
   return result;
}

FSStatus
FSGetCwd(FSClient *client, FSCmdBlock *block, char *buffer, uint32_t bufferSize, uint32_t flags)
{
   if (bufferSize < gWorkingDirectory.size() + 1) {
      return FSStatus::Cancelled;
   }

   memcpy(buffer, gWorkingDirectory.c_str(), gWorkingDirectory.size() + 1);
   return FSStatus::OK;
}

void
CoreInit::registerFileSystemFunctions()
{
   RegisterKernelFunction(FSInit);
   RegisterKernelFunction(FSShutdown);
   RegisterKernelFunction(FSAddClient);
   RegisterKernelFunction(FSDelClient);
   RegisterKernelFunction(FSGetClientNum);
   RegisterKernelFunction(FSInitCmdBlock);
   RegisterKernelFunction(FSSetCmdPriority);
   RegisterKernelFunction(FSGetStat);
   RegisterKernelFunction(FSGetStatFile);
   RegisterKernelFunction(FSGetStatFileAsync);
   RegisterKernelFunction(FSSetStateChangeNotification);
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
   RegisterKernelFunction(FSGetCwd);
}
