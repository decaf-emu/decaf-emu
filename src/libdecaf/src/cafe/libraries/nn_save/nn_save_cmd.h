#pragma once
#include "cafe/libraries/coreinit/coreinit_fs.h"
#include "nn_save_path.h"

namespace cafe::nn::save
{

using coreinit::FSAsyncData;
using coreinit::FSClient;
using coreinit::FSCmdBlock;
using coreinit::FSDirHandle;
using coreinit::FSErrorFlag;
using coreinit::FSFileHandle;
using coreinit::FSStat;

SaveStatus
SAVEChangeGroupAndOthersMode(virt_ptr<FSClient> client,
                             virt_ptr<FSCmdBlock> block,
                             uint8_t account,
                             virt_ptr<const char> path,
                             uint32_t mode,
                             FSErrorFlag errorMask);

SaveStatus
SAVEFlushQuota(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               uint8_t account,
               FSErrorFlag errorMask);

SaveStatus
SAVEFlushQuotaAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    uint8_t account,
                    FSErrorFlag errorMask,
                    virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEGetFreeSpaceSize(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     uint8_t account,
                     virt_ptr<uint64_t> freeSpace,
                     FSErrorFlag errorMask);

SaveStatus
SAVEGetFreeSpaceSizeAsync(virt_ptr<FSClient> client,
                          virt_ptr<FSCmdBlock> block,
                          uint8_t account,
                          virt_ptr<uint64_t> freeSpace,
                          FSErrorFlag errorMask,
                          virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEGetStat(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            virt_ptr<FSStat> stat,
            FSErrorFlag errorMask);

SaveStatus
SAVEGetStatAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 virt_ptr<FSStat> stat,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEGetStatOtherApplication(virt_ptr<FSClient> client,
                            virt_ptr<FSCmdBlock> block,
                            uint64_t titleId,
                            uint8_t account,
                            virt_ptr<const char> path,
                            virt_ptr<FSStat> stat,
                            FSErrorFlag errorMask);

SaveStatus
SAVEGetStatOtherApplicationAsync(virt_ptr<FSClient> client,
                                 virt_ptr<FSCmdBlock> block,
                                 uint64_t titleId,
                                 uint8_t account,
                                 virt_ptr<const char> path,
                                 virt_ptr<FSStat> stat,
                                 FSErrorFlag errorMask,
                                 virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEMakeDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            FSErrorFlag errorMask);

SaveStatus
SAVEMakeDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEOpenDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            virt_ptr<FSDirHandle> handle,
            FSErrorFlag errorMask);

SaveStatus
SAVEOpenDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 virt_ptr<FSDirHandle> handle,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVEOpenFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             uint8_t account,
             virt_ptr<const char> path,
             virt_ptr<const char> mode,
             virt_ptr<FSFileHandle> handle,
             FSErrorFlag errorMask);

SaveStatus
SAVEOpenFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  uint8_t account,
                  virt_ptr<const char> path,
                  virt_ptr<const char> mode,
                  virt_ptr<FSFileHandle> handle,
                  FSErrorFlag errorMask,
                  virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVERemove(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           uint8_t account,
           virt_ptr<const char> path,
           FSErrorFlag errorMask);

SaveStatus
SAVERemoveAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                uint8_t account,
                virt_ptr<const char> path,
                FSErrorFlag errorMask,
                virt_ptr<FSAsyncData> asyncData);

SaveStatus
SAVERename(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           uint8_t account,
           virt_ptr<const char> src,
           virt_ptr<const char> dst,
           FSErrorFlag errorMask);

SaveStatus
SAVERenameAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                uint8_t account,
                virt_ptr<const char> src,
                virt_ptr<const char> dst,
                FSErrorFlag errorMask,
                virt_ptr<FSAsyncData> asyncData);

} // namespace cafe::nn::save
