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
using coreinit::FSFileHandle;
using coreinit::FSStat;

SaveStatus
SAVEOpenFile(FSClient *client,
             FSCmdBlock *block,
             uint8_t account,
             const char *path,
             const char *mode,
             be_val<FSFileHandle> *handle,
             uint32_t flags);

SaveStatus
SAVEOpenFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t account,
                  const char *path,
                  const char *mode,
                  be_val<FSFileHandle> *handle,
                  uint32_t flags,
                  FSAsyncData *asyncData);

SaveStatus
SAVERemoveAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *path,
                uint32_t flags,
                FSAsyncData *asyncData);

SaveStatus
SAVERemove(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *path,
           uint32_t flags);

SaveStatus
SAVEGetStatAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 FSStat *stat,
                 uint32_t flags,
                 FSAsyncData *asyncData);

SaveStatus
SAVEGetStat(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            FSStat *stat,
            uint32_t flags);

SaveStatus
SAVEChangeGroupAndOthersMode(FSClient *client,
                             FSCmdBlock *block,
                             uint8_t account,
                             const char *path,
                             uint32_t mode,
                             uint32_t flags);

} // namespace save

} // namespace nn
