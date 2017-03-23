#pragma once
#include "filesystem/filesystem_path.h"
#include "modules/coreinit/coreinit_fs.h"
#include "nn_save_core.h"

namespace nn
{

namespace save
{

using coreinit::FSAsyncData;
using coreinit::FSClient;
using coreinit::FSCmdBlock;
using coreinit::FSDirHandle;

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

SaveStatus
SAVEMakeDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            uint32_t flags);

SaveStatus
SAVEMakeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 uint32_t flags,
                 FSAsyncData *asyncData);

SaveStatus
SAVEOpenDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            be_val<FSDirHandle> *handle,
            uint32_t flags);

SaveStatus
SAVEOpenDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 be_val<FSDirHandle> *handle,
                 uint32_t flags,
                 FSAsyncData *asyncData);

SaveStatus
SAVEGetFreeSpaceSizeAsync(FSClient *client,
                          FSCmdBlock *block,
                          uint8_t account,
                          uint64_t *freeSpace,
                          uint32_t flags,
                          FSAsyncData *asyncData);

SaveStatus
SAVEGetFreeSpaceSize(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t account,
                     uint64_t *freeSpace,
                     uint32_t flags);

SaveStatus
SAVEFlushQuotaAsync(FSClient *client,
                    FSCmdBlock *block,
                    uint8_t account,
                    uint32_t flags,
                    FSAsyncData *asyncData);

SaveStatus
SAVEFlushQuota(FSClient *client,
               FSCmdBlock *block,
               uint8_t account,
               uint32_t flags);

SaveStatus
SAVERenameAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *src,
                const char *dst,
                uint32_t flags,
                FSAsyncData *asyncData);

SaveStatus
SAVERename(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *src,
           const char *dst,
           uint32_t flags);

namespace internal
{

fs::Path
getSaveDirectory(uint32_t account);

fs::Path
getSavePath(uint32_t account,
            const char *path);

} // namespace internal

} // namespace save

} // namespace nn
