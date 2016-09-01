#pragma once
#include <mutex>
#include <vector>
#include "coreinit_fs.h"
#include "filesystem/filesystem.h"

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 */

class FSClient
{
public:
   FSClient();

   FSFileHandle
   addOpenFile(fs::FileHandle *file);

   FSDirectoryHandle
   addOpenDirectory(fs::FolderHandle *folder);

   fs::FileHandle *
   getOpenFile(FSFileHandle handle);

   fs::FolderHandle *
   getOpenDirectory(FSDirectoryHandle handle);

   void
   removeOpenDirectory(FSDirectoryHandle handle);

   void
   removeOpenFile(FSFileHandle handle);

   void
   setLastError(FSError error);

   FSError
   getLastError();

   void
   setWorkingPath(fs::Path path);

   fs::Path
   getWorkingPath();

private:
   FSError mLastError;
   std::mutex mMutex;
   std::vector<fs::FileHandle *> mOpenFiles;
   std::vector<fs::FolderHandle *> mOpenFolders;
   fs::Path mWorkingPath;
};

static_assert(sizeof(FSClient) < 0x1700, "FSClient must be less than 0x1700 bytes");

FSStatus
FSAddClient(FSClient *client,
            uint32_t flags);

FSStatus
FSAddClientEx(FSClient *client,
              uint32_t unk,
              uint32_t flags);

FSStatus
FSDelClient(FSClient *client,
            uint32_t flags);

uint32_t
FSGetClientNum();

FSVolumeState
FSGetVolumeState(FSClient *client);

FSError
FSGetErrorCodeForViewer(FSClient *client, FSCmdBlock *block);

FSError
FSGetLastErrorCodeForViewer(FSClient *client);

void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info);

FSAsyncResult*
FSGetAsyncResult(OSMessage *ioMsg);

void
FSCancelCommand(FSClient *client,
                FSCmdBlock *block);

void
FSSetUserData(FSCmdBlock *block,
              void *userData);

void *
FSGetUserData(FSCmdBlock *block);

FSCmdBlock *
FSGetCurrentCmdBlock(FSClient *client);

/** @} */

namespace internal
{

void
handleAsyncCallback(FSAsyncResult *result);

} // namespace internal

} // namespace coreinit
