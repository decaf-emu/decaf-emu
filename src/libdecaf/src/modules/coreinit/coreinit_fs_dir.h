#pragma once
#include "coreinit_fs.h"

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 */

FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirectoryHandle> *handle,
          uint32_t flags);

FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirectoryHandle handle,
           uint32_t flags);

FSStatus
FSMakeDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          uint32_t flags);

FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirectoryHandle handle,
          FSDirectoryEntry *entry,
          uint32_t flags);

FSStatus
FSRewindDir(FSClient *client,
            FSCmdBlock *block,
            FSDirectoryHandle handle,
            uint32_t flags);

FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirectoryHandle> *handle,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirectoryHandle handle,
                uint32_t flags,
                FSAsyncData *asyncData);

FSStatus
FSMakeDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirectoryHandle handle,
               FSDirectoryEntry *entry,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSRewindDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSDirectoryHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
FSRemoveAsync(FSClient *client,
              FSCmdBlock *block,
              const char *path,
              uint32_t flags,
              FSAsyncData *asyncData);

FSStatus
FSRemove(FSClient *client,
         FSCmdBlock *block,
         const char *path,
         uint32_t flags);

FSStatus
FSGetFreeSpaceSizeAsync(FSClient *client,
                        FSCmdBlock *block,
                        const char *path,
                        uint64_t *freeSpace,
                        uint32_t flags,
                        FSAsyncData *asyncData);

FSStatus
FSGetFreeSpaceSize(FSClient *client,
                   FSCmdBlock *block,
                   const char *path,
                   uint64_t *freeSpace,
                   uint32_t flags);

FSStatus
FSFlushQuotaAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  uint32_t flags,
                  FSAsyncData *asyncData);

FSStatus
FSFlushQuota(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             uint32_t flags);

/** @} */

} // namespace coreinit
