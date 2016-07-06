#include <algorithm>
#include "common/emuassert.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_memheap.h"
#include "coreinit_internal_appio.h"
#include "ppcutils/wfunc_call.h"

namespace coreinit
{

static std::vector<FSClient*>
sClients;

FSClient::FSClient()
{
   // Let's just ensure there is never a file handle 0 just in case it's not a valid handle
   mOpenFiles.push_back(nullptr);
}


FSFileHandle
FSClient::addOpenFile(fs::FileHandle *file)
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


FSDirectoryHandle
FSClient::addOpenDirectory(fs::FolderHandle *folder)
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


fs::FileHandle *
FSClient::getOpenFile(FSFileHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFiles.size()) {
      return nullptr;
   }

   return mOpenFiles[handle];
}


fs::FolderHandle *
FSClient::getOpenDirectory(FSDirectoryHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFolders.size()) {
      return nullptr;
   }

   return mOpenFolders[handle];
}


void
FSClient::removeOpenDirectory(FSDirectoryHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFolders.size()) {
      return;
   } else {
      if (mOpenFolders[handle]) {
         mOpenFolders[handle]->close();
      }

      delete mOpenFolders[handle];
      mOpenFolders[handle] = nullptr;
   }
}


void
FSClient::removeOpenFile(FSFileHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFiles.size()) {
      return;
   } else {
      if (mOpenFiles[handle]) {
         mOpenFiles[handle]->close();
      }

      delete mOpenFiles[handle];
      mOpenFiles[handle] = nullptr;
   }
}


void
FSClient::setLastError(FSError error)
{
   mLastError = error;
}


FSError
FSClient::getLastError()
{
   return mLastError;
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
   sClients.push_back(client);
   return FSStatus::OK;
}


FSStatus
FSDelClient(FSClient *client,
            uint32_t flags)
{
   client->~FSClient();
   sClients.erase(std::remove(sClients.begin(), sClients.end(), client), sClients.end());
   return FSStatus::OK;
}


uint32_t
FSGetClientNum()
{
   return static_cast<uint32_t>(sClients.size());
}


FSVolumeState
FSGetVolumeState(FSClient *client)
{
   return FSVolumeState::Ready;
}


FSError
FSGetLastErrorCodeForViewer(FSClient *client)
{
   return client->getLastError();
}


void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info)
{
   // TODO: FSSetStateChangeNotification
}

FSAsyncResult *
FSGetAsyncResult(OSMessage *ioMsg)
{
   return static_cast<FSAsyncResult *>(ioMsg->message.get());
}

void
FSCancelCommand(FSClient *client,
                FSCmdBlock *block)
{
   // TODO: FSCancelCommand
}

namespace internal
{

void
handleAsyncCallback(FSAsyncResult *result)
{
   auto callback = static_cast<FSAsyncCallback>(result->userParams.callback);
   callback(result->client, result->block, result->status, result->userParams.param);
}

} // namespace internal

} // namespace coreinit
