#include <mutex>
#include "coreinit.h"
#include "coreinit_fs.h"
#include "coreinit_memory.h"
#include "filesystem.h"
#include "system.h"

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
   return FSStatus::OK;
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
FSGetStat(FSClient *client, FSCmdBlock *block, const char *filepath, FSStat *stat, uint32_t flags)
{
   return FSStatus::NotFound;
}

FSStatus
FSGetStatFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSStat *stat, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   stat->size = static_cast<uint32_t>(fsFile->file->size());
   return FSStatus::OK;
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
FSCloseFile(FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t flags)
{
   auto fsFile = gOpenFiles.get(handle);

   if (!fsFile) {
      return FSStatus::FatalError;
   }

   gOpenFiles.close(handle);
   return FSStatus::OK;
}

void
CoreInit::registerFileSystemFunctions()
{
   RegisterSystemFunction(FSInit);
   RegisterSystemFunction(FSShutdown);
   RegisterSystemFunction(FSAddClient);
   RegisterSystemFunction(FSInitCmdBlock);
   RegisterSystemFunction(FSSetCmdPriority);
   RegisterSystemFunction(FSGetStat);
   RegisterSystemFunction(FSGetStatFile);
   RegisterSystemFunction(FSSetStateChangeNotification);
   RegisterSystemFunction(FSOpenFile);
   RegisterSystemFunction(FSReadFile);
   RegisterSystemFunction(FSCloseFile);
}
