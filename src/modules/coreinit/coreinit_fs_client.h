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

private:
   FSError mLastError;
   std::mutex mMutex;
   std::vector<fs::FileHandle *> mOpenFiles;
   std::vector<fs::FolderHandle *> mOpenFolders;
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
FSGetLastErrorCodeForViewer(FSClient *client);

void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info);

/** @} */

namespace internal
{

void
doAsyncFileCallback(FSClient *client,
                    FSCmdBlock *block,
                    FSStatus result,
                    FSAsyncData *asyncData);

} // namespace internal

} // namespace coreinit
