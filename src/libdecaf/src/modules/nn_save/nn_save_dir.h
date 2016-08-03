#pragma once
#include <string>
#include "nn_save_core.h"
#include "filesystem/filesystem_path.h"
#include "modules/coreinit/coreinit_fs.h"

namespace nn
{

namespace save
{

using coreinit::FSAsyncData;
using coreinit::FSClient;
using coreinit::FSCmdBlock;
using coreinit::FSDirectoryHandle;
using coreinit::FSStatus;

SaveStatus
SAVEInitSaveDir(uint8_t userID);

SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID,
                           const char *dir,
                           char *buffer,
                           uint32_t bufferSize);

SaveStatus
SAVEGetSharedSaveDataPath(uint64_t titleID,
                          const char *dir,
                          char *buffer,
                          uint32_t bufferSize);

FSStatus
SAVEMakeDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            uint32_t flags);

FSStatus
SAVEMakeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
SAVEOpenDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            be_val<FSDirectoryHandle> *handle,
            uint32_t flags);

FSStatus
SAVEOpenDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 be_val<FSDirectoryHandle> *handle,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
SAVEGetFreeSpaceSizeAsync(FSClient *client,
                          FSCmdBlock *block,
                          uint8_t account,
                          uint64_t *freeSpace,
                          uint32_t flags,
                          FSAsyncData *asyncData);

FSStatus
SAVEGetFreeSpaceSize(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t account,
                     uint64_t *freeSpace,
                     uint32_t flags);

FSStatus
SAVEFlushQuotaAsync(FSClient *client,
                    FSCmdBlock *block,
                    uint8_t account,
                    uint32_t flags,
                    FSAsyncData *asyncData);

FSStatus
SAVEFlushQuota(FSClient *client,
               FSCmdBlock *block,
               uint8_t account,
               uint32_t flags);

namespace internal
{

fs::Path
getSaveDirectory(uint32_t account);

fs::Path
getSavePath(uint32_t account, const char *path);

} // namespace internal

} // namespace save

} // namespace nn
