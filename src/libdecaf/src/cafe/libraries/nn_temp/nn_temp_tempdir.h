#pragma once
#include "nn_temp_enum.h"
#include "cafe/libraries/coreinit/coreinit_fs.h"

#include <libcpu/be2_struct.h>
#include <string>

namespace cafe::nn_temp
{

#pragma pack(push, 1)

using TEMPDirId = uint64_t;
constexpr auto DirPathMaxLength = 0x20u;
constexpr auto GlobalPathMaxLength = 0x40u;

struct TEMPDeviceInfo
{
   be2_val<uint64_t> dirId;
   be2_array<char, GlobalPathMaxLength> targetPath;
};
CHECK_OFFSET(TEMPDeviceInfo, 0x00, dirId);
CHECK_OFFSET(TEMPDeviceInfo, 0x08, targetPath);
CHECK_SIZE(TEMPDeviceInfo, 0x48);

#pragma pack(pop)

TEMPStatus
TEMPInit();

void
TEMPShutdown();

TEMPStatus
TEMPChangeDir(virt_ptr<coreinit::FSClient> client,
              virt_ptr<coreinit::FSCmdBlock> block,
              TEMPDirId dirId,
              virt_ptr<const char> path,
              coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPChangeDirAsync(virt_ptr<coreinit::FSClient> client,
                   virt_ptr<coreinit::FSCmdBlock> block,
                   TEMPDirId dirId,
                   virt_ptr<const char> path,
                   coreinit::FSErrorFlag errorMask,
                   virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         TEMPDevicePreference pref,
                         virt_ptr<TEMPDirId> outDirId);

TEMPStatus
TEMPGetDirPath(TEMPDirId dirId,
               virt_ptr<char> pathBuffer,
               uint32_t pathBufferSize);

TEMPStatus
TEMPGetDirGlobalPath(TEMPDirId dirId,
                     virt_ptr<char> pathBuffer,
                     uint32_t pathBufferSize);

TEMPStatus
TEMPGetFreeSpaceSize(virt_ptr<coreinit::FSClient> client,
                     virt_ptr<coreinit::FSCmdBlock> block,
                     TEMPDirId dirId,
                     virt_ptr<uint64_t> outFreeSize,
                     coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPGetFreeSpaceSizeAsync(virt_ptr<coreinit::FSClient> client,
                          virt_ptr<coreinit::FSCmdBlock> block,
                          TEMPDirId dirId,
                          virt_ptr<uint64_t> outFreeSize,
                          coreinit::FSErrorFlag errorMask,
                          virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPGetStat(virt_ptr<coreinit::FSClient> client,
            virt_ptr<coreinit::FSCmdBlock> block,
            TEMPDirId dirId,
            virt_ptr<const char> path,
            virt_ptr<coreinit::FSStat> outStat,
            coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPGetStatAsync(virt_ptr<coreinit::FSClient> client,
                 virt_ptr<coreinit::FSCmdBlock> block,
                 TEMPDirId dirId,
                 virt_ptr<const char> path,
                 virt_ptr<coreinit::FSStat> outStat,
                 coreinit::FSErrorFlag errorMask,
                 virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPMakeDir(virt_ptr<coreinit::FSClient> client,
            virt_ptr<coreinit::FSCmdBlock> block,
            TEMPDirId dirId,
            virt_ptr<const char> path,
            coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPMakeDirAsync(virt_ptr<coreinit::FSClient> client,
                 virt_ptr<coreinit::FSCmdBlock> block,
                 TEMPDirId dirId,
                 virt_ptr<const char> path,
                 coreinit::FSErrorFlag errorMask,
                 virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPMountTempDir(TEMPDirId dirId);

TEMPStatus
TEMPOpenDir(virt_ptr<coreinit::FSClient> client,
            virt_ptr<coreinit::FSCmdBlock> block,
            TEMPDirId dirId,
            virt_ptr<const char> path,
            virt_ptr<coreinit::FSDirHandle> outDirHandle,
            coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPOpenDirAsync(virt_ptr<coreinit::FSClient> client,
                 virt_ptr<coreinit::FSCmdBlock> block,
                 TEMPDirId dirId,
                 virt_ptr<const char> path,
                 virt_ptr<coreinit::FSDirHandle> outDirHandle,
                 coreinit::FSErrorFlag errorMask,
                 virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPOpenFile(virt_ptr<coreinit::FSClient> client,
             virt_ptr<coreinit::FSCmdBlock> block,
             TEMPDirId dirId,
             virt_ptr<const char> path,
             virt_ptr<const char> mode,
             virt_ptr<coreinit::FSFileHandle> outFileHandle,
             coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPOpenFileAsync(virt_ptr<coreinit::FSClient> client,
                  virt_ptr<coreinit::FSCmdBlock> block,
                  TEMPDirId dirId,
                  virt_ptr<const char> path,
                  virt_ptr<const char> mode,
                  virt_ptr<coreinit::FSFileHandle> outFileHandle,
                  coreinit::FSErrorFlag errorMask,
                  virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPOpenNewFile(virt_ptr<coreinit::FSClient> client,
                virt_ptr<coreinit::FSCmdBlock> block,
                TEMPDirId dirId,
                virt_ptr<const char> path,
                virt_ptr<const char> mode,
                virt_ptr<coreinit::FSFileHandle> outFileHandle,
                coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPOpenNewFileAsync(virt_ptr<coreinit::FSClient> client,
                     virt_ptr<coreinit::FSCmdBlock> block,
                     TEMPDirId dirId,
                     virt_ptr<const char> path,
                     virt_ptr<const char> mode,
                     virt_ptr<coreinit::FSFileHandle> outFileHandle,
                     coreinit::FSErrorFlag errorMask,
                     virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPRemove(virt_ptr<coreinit::FSClient> client,
           virt_ptr<coreinit::FSCmdBlock> block,
           TEMPDirId dirId,
           virt_ptr<const char> path,
           coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPRemoveAsync(virt_ptr<coreinit::FSClient> client,
                virt_ptr<coreinit::FSCmdBlock> block,
                TEMPDirId dirId,
                virt_ptr<const char> path,
                coreinit::FSErrorFlag errorMask,
                virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPRename(virt_ptr<coreinit::FSClient> client,
           virt_ptr<coreinit::FSCmdBlock> block,
           TEMPDirId dirId,
           virt_ptr<const char> src,
           virt_ptr<const char> dst,
           coreinit::FSErrorFlag errorMask);

TEMPStatus
TEMPRenameAsync(virt_ptr<coreinit::FSClient> client,
                virt_ptr<coreinit::FSCmdBlock> block,
                TEMPDirId dirId,
                virt_ptr<const char> src,
                virt_ptr<const char> dst,
                coreinit::FSErrorFlag errorMask,
                virt_ptr<const coreinit::FSAsyncData> asyncData);

TEMPStatus
TEMPShutdownTempDir(TEMPDirId id);

TEMPStatus
TEMPUnmountTempDir(TEMPDirId dirId);

} // namespace cafe::nn_temp
