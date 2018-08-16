#pragma once
#include "coreinit_fs.h"
#include "coreinit_fsa.h"

namespace cafe::coreinit
{

FSStatus
FSChangeDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            virt_ptr<const char> path,
            FSErrorFlag errorMask);

FSStatus
FSChangeDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 virt_ptr<const char> path,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSCloseDir(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           FSDirHandle handle,
           FSErrorFlag errorMask);

FSStatus
FSCloseDirAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                FSDirHandle handle,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSCloseFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSFileHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSCloseFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSFlushFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSFileHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSFlushFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSFlushQuota(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             FSErrorFlag errorMask);

FSStatus
FSFlushQuotaAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetCwd(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<char> returnedPath,
         uint32_t bytes,
         FSErrorFlag errorMask);

FSStatus
FSGetCwdAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<char> returnedPath,
              uint32_t bytes,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetDirSize(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             virt_ptr<uint64_t> returnedDirSize,
             FSErrorFlag errorMask);

FSStatus
FSGetDirSizeAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  virt_ptr<uint64_t> returnedDirSize,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetFreeSpaceSize(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   virt_ptr<const char> path,
                   virt_ptr<uint64_t> returnedFreeSize,
                   FSErrorFlag errorMask);

FSStatus
FSGetFreeSpaceSizeAsync(virt_ptr<FSClient> client,
                        virt_ptr<FSCmdBlock> block,
                        virt_ptr<const char> path,
                        virt_ptr<uint64_t> returnedFreeSize,
                        FSErrorFlag errorMask,
                        virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetMountSource(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSMountSourceType type,
                 virt_ptr<FSMountSource> source,
                 FSErrorFlag errorMask);

FSStatus
FSGetMountSourceAsync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      FSMountSourceType type,
                      virt_ptr<FSMountSource> outSource,
                      FSErrorFlag errorMask,
                      virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetMountSourceNext(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     virt_ptr<FSMountSource> outSource,
                     FSErrorFlag errorMask);

FSStatus
FSGetMountSourceNextAsync(virt_ptr<FSClient> client,
                          virt_ptr<FSCmdBlock> block,
                          virt_ptr<FSMountSource> outSource,
                          FSErrorFlag errorMask,
                          virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetPosFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             virt_ptr<FSFilePosition> outPos,
             FSErrorFlag errorMask);

FSStatus
FSGetPosFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  FSFileHandle handle,
                  virt_ptr<FSFilePosition> outPos,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetStat(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          virt_ptr<FSStat> outStat,
          FSErrorFlag errorMask);

FSStatus
FSGetStatAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               virt_ptr<FSStat> outStat,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSGetStatFile(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              FSFileHandle handle,
              virt_ptr<FSStat> outStat,
              FSErrorFlag errorMask);

FSStatus
FSGetStatFileAsync(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   FSFileHandle handle,
                   virt_ptr<FSStat> outStat,
                   FSErrorFlag errorMask,
                   virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSIsEof(virt_ptr<FSClient> client,
        virt_ptr<FSCmdBlock> block,
        FSFileHandle handle,
        FSErrorFlag errorMask);

FSStatus
FSIsEofAsync(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             FSErrorFlag errorMask,
             virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSMakeDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          FSErrorFlag errorMask);

FSStatus
FSMakeDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSMount(virt_ptr<FSClient> client,
        virt_ptr<FSCmdBlock> block,
        virt_ptr<FSMountSource> source,
        virt_ptr<char> target,
        uint32_t bytes,
        FSErrorFlag errorMask);

FSStatus
FSMountAsync(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<FSMountSource> source,
             virt_ptr<char> target,
             uint32_t bytes,
             FSErrorFlag errorMask,
             virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSOpenDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          virt_ptr<FSDirHandle> outHandle,
          FSErrorFlag errorMask);

FSStatus
FSOpenDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               virt_ptr<FSDirHandle> outHandle,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSOpenFile(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           virt_ptr<const char> path,
           virt_ptr<const char> mode,
           virt_ptr<FSFileHandle> outHandle,
           FSErrorFlag errorMask);

FSStatus
FSOpenFileAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                virt_ptr<const char> path,
                virt_ptr<const char> mode,
                virt_ptr<FSFileHandle> outHandle,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSOpenFileEx(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             virt_ptr<const char> mode,
             uint32_t unk1,
             uint32_t unk2,
             uint32_t unk3,
             virt_ptr<FSFileHandle> outHandle,
             FSErrorFlag errorMask);

FSStatus
FSOpenFileExAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  virt_ptr<const char> mode,
                  uint32_t unk1,
                  uint32_t unk2,
                  uint32_t unk3,
                  virt_ptr<FSFileHandle> outHandle,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSReadDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          FSDirHandle handle,
          virt_ptr<FSDirEntry> outDirEntry,
          FSErrorFlag errorMask);

FSStatus
FSReadDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               FSDirHandle handle,
               virt_ptr<FSDirEntry> outDirEntry,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSReadFile(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           virt_ptr<uint8_t> buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           FSReadFlag readFlags,
           FSErrorFlag errorMask);

FSStatus
FSReadFileAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                virt_ptr<uint8_t> buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                FSReadFlag readFlags,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSReadFileWithPos(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<uint8_t> buffer,
                  uint32_t size,
                  uint32_t count,
                  FSFilePosition pos,
                  FSFileHandle handle,
                  FSReadFlag readFlags,
                  FSErrorFlag errorMask);

FSStatus
FSReadFileWithPosAsync(virt_ptr<FSClient> client,
                       virt_ptr<FSCmdBlock> block,
                       virt_ptr<uint8_t> buffer,
                       uint32_t size,
                       uint32_t count,
                       FSFilePosition pos,
                       FSFileHandle handle,
                       FSReadFlag readFlags,
                       FSErrorFlag errorMask,
                       virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSRemove(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<const char> path,
         FSErrorFlag errorMask);

FSStatus
FSRemoveAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<const char> path,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSRename(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<const char> oldPath,
         virt_ptr<const char> newPath,
         FSErrorFlag errorMask);

FSStatus
FSRenameAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<const char> oldPath,
              virt_ptr<const char> newPath,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSRewindDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSDirHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSRewindDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSDirHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSSetPosFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             FSFilePosition pos,
             FSErrorFlag errorMask);

FSStatus
FSSetPosFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  FSFileHandle handle,
                  FSFilePosition pos,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSTruncateFile(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               FSFileHandle handle,
               FSErrorFlag errorMask);

FSStatus
FSTruncateFileAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    FSFileHandle handle,
                    FSErrorFlag errorMask,
                    virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSUnmount(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> target,
          FSErrorFlag errorMask);

FSStatus
FSUnmountAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> target,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSWriteFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            virt_ptr<const uint8_t> buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            FSWriteFlag writeFlags,
            FSErrorFlag errorMask);

FSStatus
FSWriteFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 virt_ptr<const uint8_t> buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 FSWriteFlag writeFlags,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData);

FSStatus
FSWriteFileWithPos(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   virt_ptr<const uint8_t> buffer,
                   uint32_t size,
                   uint32_t count,
                   FSFilePosition pos,
                   FSFileHandle handle,
                   FSWriteFlag writeFlags,
                   FSErrorFlag errorMask);

FSStatus
FSWriteFileWithPosAsync(virt_ptr<FSClient> client,
                        virt_ptr<FSCmdBlock> block,
                        virt_ptr<const uint8_t> buffer,
                        uint32_t size,
                        uint32_t count,
                        FSFilePosition pos,
                        FSFileHandle handle,
                        FSWriteFlag writeFlags,
                        FSErrorFlag errorMask,
                        virt_ptr<const FSAsyncData> asyncData);

} // namespace cafe::coreinit
