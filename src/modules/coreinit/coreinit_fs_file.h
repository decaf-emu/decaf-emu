#pragma once
#include "coreinit_fs.h"

FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *path,
           const char *mode,
           be_val<FSFileHandle> *handle,
           uint32_t flags);

FSStatus
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            uint32_t flags);

FSStatus
FSReadFile(FSClient *client,
           FSCmdBlock *block,
           uint8_t *buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           uint32_t unk1,
           uint32_t flags);

FSStatus
FSReadFileWithPos(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t *buffer,
                  uint32_t size,
                  uint32_t count,
                  uint32_t pos,
                  FSFileHandle handle,
                  uint32_t unk1,
                  uint32_t flags);

FSStatus
FSWriteFile(FSClient *client,
            FSCmdBlock *block,
            uint8_t *buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            uint32_t unk1,
            uint32_t flags);

FSStatus
FSWriteFileWithPos(FSClient *client,
                   FSCmdBlock *block,
                   uint8_t *buffer,
                   uint32_t size,
                   uint32_t count,
                   uint32_t pos,
                   FSFileHandle handle,
                   uint32_t unk1,
                   uint32_t flags);

FSStatus
FSIsEof(FSClient *client,
        FSCmdBlock *block,
        FSFileHandle handle,
        uint32_t flags);

FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             be_val<uint32_t> *pos,
             uint32_t flags);

FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t pos,
             uint32_t flags);

FSStatus
FSTruncateFile(FSClient *client,
               FSCmdBlock *block,
               FSFileHandle handle,
               uint32_t flags);

FSStatus
FSOpenFileAsync(FSClient *client,
                FSCmdBlock *block,
                const char *path,
                const char *mode,
                be_val<FSFileHandle> *outHandle,
                uint32_t flags,
                FSAsyncData *asyncData);

FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
FSReadFileAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t *buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                uint32_t unk1,
                uint32_t flags,
                FSAsyncData *asyncData);

FSStatus
FSReadFileWithPosAsync(FSClient *client,
                       FSCmdBlock *block,
                       uint8_t *buffer,
                       uint32_t size,
                       uint32_t count,
                       uint32_t position,
                       FSFileHandle handle,
                       uint32_t unk1,
                       uint32_t flags,
                       FSAsyncData *asyncData);

FSStatus
FSWriteFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t *buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 uint32_t unk1,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
FSWriteFileWithPosAsync(FSClient *client,
                        FSCmdBlock *block,
                        uint8_t *buffer,
                        uint32_t size,
                        uint32_t count,
                        uint32_t position,
                        FSFileHandle handle,
                        uint32_t unk1,
                        uint32_t flags,
                        FSAsyncData *asyncData);

FSStatus
FSIsEofAsync(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t flags,
             FSAsyncData *asyncData);

FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle fileHandle,
                  be_val<uint32_t> *pos,
                  uint32_t flags,
                  FSAsyncData *asyncData);

FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  uint32_t pos,
                  uint32_t flags,
                  FSAsyncData *asyncData);

FSStatus
FSTruncateFileAsync(FSClient *client,
                    FSCmdBlock *block,
                    FSFileHandle handle,
                    uint32_t flags,
                    FSAsyncData *asyncData);
