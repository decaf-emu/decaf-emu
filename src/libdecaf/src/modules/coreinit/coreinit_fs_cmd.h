#pragma once
#include "coreinit_fs.h"

namespace coreinit
{

FSStatus
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            FSErrorFlag errorMask);

FSStatus
FSChangeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 const char *path,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData);

FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirHandle handle,
           FSErrorFlag errorMask);

FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirHandle handle,
                FSErrorFlag errorMask,
                const FSAsyncData *asyncData);

FSStatus
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData);

FSStatus
FSFlushFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSFlushFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData);

FSStatus
FSFlushQuota(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             FSErrorFlag errorMask);

FSStatus
FSFlushQuotaAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData);

FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *returnedPath,
         uint32_t bytes,
         FSErrorFlag errorMask);

FSStatus
FSGetCwdAsync(FSClient *client,
              FSCmdBlock *block,
              char *returnedPath,
              uint32_t bytes,
              FSErrorFlag errorMask,
              const FSAsyncData *asyncData);

FSStatus
FSGetDirSize(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             be_val<uint64_t> *returnedDirSize,
             FSErrorFlag errorMask);

FSStatus
FSGetDirSizeAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  be_val<uint64_t> *returnedDirSize,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData);

FSStatus
FSGetFreeSpaceSize(FSClient *client,
                   FSCmdBlock *block,
                   const char *path,
                   be_val<uint64_t> *returnedFreeSize,
                   FSErrorFlag errorMask);

FSStatus
FSGetFreeSpaceSizeAsync(FSClient *client,
                        FSCmdBlock *block,
                        const char *path,
                        be_val<uint64_t> *returnedFreeSize,
                        FSErrorFlag errorMask,
                        const FSAsyncData *asyncData);

FSStatus
FSGetMountSource(FSClient *client,
                 FSCmdBlock *block,
                 FSMountSourceType type,
                 FSMountSource *source,
                 FSErrorFlag errorMask);

FSStatus
FSGetMountSourceAsync(FSClient *client,
                      FSCmdBlock *block,
                      FSMountSourceType type,
                      FSMountSource *source,
                      FSErrorFlag errorMask,
                      const FSAsyncData *asyncData);

FSStatus
FSGetMountSourceNext(FSClient *client,
                     FSCmdBlock *block,
                     FSMountSource *source,
                     FSErrorFlag errorMask);

FSStatus
FSGetMountSourceNextAsync(FSClient *client,
                          FSCmdBlock *block,
                          FSMountSource *source,
                          FSErrorFlag errorMask,
                          const FSAsyncData *asyncData);

FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             be_val<FSFilePosition> *returnedFpos,
             FSErrorFlag errorMask);

FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  be_val<FSFilePosition> *returnedFpos,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData);

FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *returnedStat,
          FSErrorFlag errorMask);

FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *returnedStat,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData);

FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *returnedStat,
              FSErrorFlag errorMask);

FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *returnedStat,
                   FSErrorFlag errorMask,
                   const FSAsyncData *asyncData);

FSStatus
FSIsEof(FSClient *client,
        FSCmdBlock *block,
        FSFileHandle handle,
        FSErrorFlag errorMask);

FSStatus
FSIsEofAsync(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             FSErrorFlag errorMask,
             const FSAsyncData *asyncData);

FSStatus
FSMakeDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSErrorFlag errorMask);

FSStatus
FSMakeDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData);

FSStatus
FSMount(FSClient *client,
        FSCmdBlock *block,
        FSMountSource *source,
        char *target,
        uint32_t bytes,
        FSErrorFlag errorMask);

FSStatus
FSMountAsync(FSClient *client,
             FSCmdBlock *block,
             FSMountSource *source,
             char *target,
             uint32_t bytes,
             FSErrorFlag errorMask,
             const FSAsyncData *asyncData);

FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirHandle> *dirHandle,
          FSErrorFlag errorMask);

FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirHandle> *dirHandle,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData);

FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *path,
           const char *mode,
           be_val<FSFileHandle> *fileHandle,
           FSErrorFlag errorMask);

FSStatus
FSOpenFileAsync(FSClient *client,
                FSCmdBlock *block,
                const char *path,
                const char *mode,
                be_val<FSFileHandle> *fileHandle,
                FSErrorFlag errorMask,
                const FSAsyncData *asyncData);

FSStatus
FSOpenFileEx(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             const char *mode,
             uint32_t unk1,
             uint32_t unk2,
             uint32_t unk3,
             be_val<FSFileHandle> *fileHandle,
             FSErrorFlag errorMask);

FSStatus
FSOpenFileExAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  const char *mode,
                  uint32_t unk1,
                  uint32_t unk2,
                  uint32_t unk3,
                  be_val<FSFileHandle> *fileHandle,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData);

FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirHandle handle,
          FSDirEntry *returnedDirEntry,
          FSErrorFlag errorMask);

FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirHandle handle,
               FSDirEntry *returnedDirEntry,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData);

FSStatus
FSReadFile(FSClient *client,
           FSCmdBlock *block,
           uint8_t *buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           FSReadFlag readFlags,
           FSErrorFlag errorMask);

FSStatus
FSReadFileAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t *buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                FSReadFlag readFlags,
                FSErrorFlag errorMask,
                const FSAsyncData *asyncData);

FSStatus
FSReadFileWithPos(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t *buffer,
                  uint32_t size,
                  uint32_t count,
                  FSFilePosition pos,
                  FSFileHandle handle,
                  FSReadFlag readFlags,
                  FSErrorFlag errorMask);

FSStatus
FSReadFileWithPosAsync(FSClient *client,
                       FSCmdBlock *block,
                       uint8_t *buffer,
                       uint32_t size,
                       uint32_t count,
                       FSFilePosition pos,
                       FSFileHandle handle,
                       FSReadFlag readFlags,
                       FSErrorFlag errorMask,
                       const FSAsyncData *asyncData);

FSStatus
FSRemove(FSClient *client,
         FSCmdBlock *block,
         const char *path,
         FSErrorFlag errorMask);

FSStatus
FSRemoveAsync(FSClient *client,
              FSCmdBlock *block,
              const char *path,
              FSErrorFlag errorMask,
              const FSAsyncData *asyncData);

FSStatus
FSRename(FSClient *client,
         FSCmdBlock *block,
         const char *oldPath,
         const char *newPath,
         FSErrorFlag errorMask);

FSStatus
FSRenameAsync(FSClient *client,
              FSCmdBlock *block,
              const char *oldPath,
              const char *newPath,
              FSErrorFlag errorMask,
              const FSAsyncData *asyncData);

FSStatus
FSRewindDir(FSClient *client,
            FSCmdBlock *block,
            FSDirHandle handle,
            FSErrorFlag errorMask);

FSStatus
FSRewindDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSDirHandle handle,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData);

FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             FSFilePosition pos,
             FSErrorFlag errorMask);

FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  FSFilePosition pos,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData);

FSStatus
FSTruncateFile(FSClient *client,
               FSCmdBlock *block,
               FSFileHandle handle,
               FSErrorFlag errorMask);

FSStatus
FSTruncateFileAsync(FSClient *client,
                    FSCmdBlock *block,
                    FSFileHandle handle,
                    FSErrorFlag errorMask,
                    const FSAsyncData *asyncData);

FSStatus
FSUnmount(FSClient *client,
          FSCmdBlock *block,
          const char *target,
          FSErrorFlag errorMask);

FSStatus
FSUnmountAsync(FSClient *client,
               FSCmdBlock *block,
               const char *target,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData);

FSStatus
FSWriteFile(FSClient *client,
            FSCmdBlock *block,
            const uint8_t *buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            FSWriteFlag writeFlags,
            FSErrorFlag errorMask);

FSStatus
FSWriteFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 const uint8_t *buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 FSWriteFlag writeFlags,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData);

FSStatus
FSWriteFileWithPos(FSClient *client,
                   FSCmdBlock *block,
                   const uint8_t *buffer,
                   uint32_t size,
                   uint32_t count,
                   FSFilePosition pos,
                   FSFileHandle handle,
                   FSWriteFlag writeFlags,
                   FSErrorFlag errorMask);

FSStatus
FSWriteFileWithPosAsync(FSClient *client,
                        FSCmdBlock *block,
                        const uint8_t *buffer,
                        uint32_t size,
                        uint32_t count,
                        FSFilePosition pos,
                        FSFileHandle handle,
                        FSWriteFlag writeFlags,
                        FSErrorFlag errorMask,
                        const FSAsyncData *asyncData);

} // namespace coreinit
