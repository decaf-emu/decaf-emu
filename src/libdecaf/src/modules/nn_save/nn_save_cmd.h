#pragma once
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
using coreinit::FSErrorFlag;
using coreinit::FSFileHandle;
using coreinit::FSStat;

SaveStatus
SAVEChangeGroupAndOthersMode(FSClient *client,
                             FSCmdBlock *block,
                             uint8_t account,
                             const char *path,
                             uint32_t mode,
                             FSErrorFlag errorMask);

SaveStatus
SAVEFlushQuota(FSClient *client,
               FSCmdBlock *block,
               uint8_t account,
               FSErrorFlag errorMask);

SaveStatus
SAVEFlushQuotaAsync(FSClient *client,
                    FSCmdBlock *block,
                    uint8_t account,
                    FSErrorFlag errorMask,
                    FSAsyncData *asyncData);

SaveStatus
SAVEGetFreeSpaceSize(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t account,
                     be_val<uint64_t> *freeSpace,
                     FSErrorFlag errorMask);

SaveStatus
SAVEGetFreeSpaceSizeAsync(FSClient *client,
                          FSCmdBlock *block,
                          uint8_t account,
                          be_val<uint64_t> *freeSpace,
                          FSErrorFlag errorMask,
                          FSAsyncData *asyncData);

SaveStatus
SAVEGetStat(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            FSStat *stat,
            FSErrorFlag errorMask);

SaveStatus
SAVEGetStatAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 FSStat *stat,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData);

SaveStatus
SAVEMakeDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            FSErrorFlag errorMask);

SaveStatus
SAVEMakeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData);

SaveStatus
SAVEOpenDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            be_val<FSDirHandle> *handle,
            FSErrorFlag errorMask);

SaveStatus
SAVEOpenDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 be_val<FSDirHandle> *handle,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData);

SaveStatus
SAVEOpenFile(FSClient *client,
             FSCmdBlock *block,
             uint8_t account,
             const char *path,
             const char *mode,
             be_val<FSFileHandle> *handle,
             FSErrorFlag errorMask);

SaveStatus
SAVEOpenFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t account,
                  const char *path,
                  const char *mode,
                  be_val<FSFileHandle> *handle,
                  FSErrorFlag errorMask,
                  FSAsyncData *asyncData);

SaveStatus
SAVERemove(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *path,
           FSErrorFlag errorMask);

SaveStatus
SAVERemoveAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *path,
                FSErrorFlag errorMask,
                FSAsyncData *asyncData);

SaveStatus
SAVERename(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *src,
           const char *dst,
           FSErrorFlag errorMask);

SaveStatus
SAVERenameAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *src,
                const char *dst,
                FSErrorFlag errorMask,
                FSAsyncData *asyncData);

} // namespace save

} // namespace nn
