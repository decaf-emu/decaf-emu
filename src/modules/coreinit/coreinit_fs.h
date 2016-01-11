#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

using FSDirectoryHandle = uint32_t;
using FSFileHandle = uint32_t;
using FSPriority = uint32_t;

#pragma pack(push, 1)

class FSClient; // Size = 0x1700

struct FSCmdBlock
{
   UNKNOWN(0xa80); // memset in FSInitCmdBlock
};

struct FSStat
{
   enum Flags
   {
      Directory = 0x80000000,
   };

   be_val<uint32_t> flags;
   UNKNOWN(0xc);
   be_val<uint32_t> size;
   UNKNOWN(0x50);
};
CHECK_OFFSET(FSStat, 0x00, flags);
CHECK_OFFSET(FSStat, 0x10, size);
CHECK_SIZE(FSStat, 0x64); // 0x64 from memmove in FSAGetStatFile

struct FSStateChangeInfo
{
   UNKNOWN(0xc); // Copy loop at FSSetStateChangeNotification
};

using FSAsyncCallback = wfunc_ptr<void, FSClient *, FSCmdBlock *, FSStatus, uint32_t>;

struct FSAsyncData
{
   be_val<uint32_t> callback;
   be_val<uint32_t> param;
   be_val<uint32_t> unk1;
};
CHECK_OFFSET(FSAsyncData, 0x00, callback);
CHECK_OFFSET(FSAsyncData, 0x04, param);
CHECK_SIZE(FSAsyncData, 0xC);

struct FSDirectoryEntry
{
   FSStat info;
   char name[256];
};
CHECK_OFFSET(FSDirectoryEntry, 0x64, name);
CHECK_SIZE(FSDirectoryEntry, 0x164);

#pragma pack(pop)

void
FSInit();

void
FSShutdown();

FSStatus
FSAddClient(FSClient *client,
            uint32_t flags);

FSStatus
FSAddClientEx(FSClient *client,
              uint32_t unk,
              uint32_t flags);

FSStatus
FSDelClient(FSClient *client,
            uint32_t flags);

uint32_t
FSGetClientNum();

void
FSInitCmdBlock(FSCmdBlock *block);

FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 FSPriority priority);

void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info);

FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *buffer,
         uint32_t bufferSize,
         uint32_t flags);

FSStatus
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            uint32_t flags);

FSStatus
FSChangeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 const char *path,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *stat,
          uint32_t flags);

FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *stat,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *path,
           const char *mode,
           be_val<FSFileHandle> *handle,
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
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            uint32_t flags);

FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 uint32_t flags,
                 FSAsyncData *asyncData);

FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirectoryHandle> *handle,
          uint32_t flags);

FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirectoryHandle> *handle,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirectoryHandle handle,
          FSDirectoryEntry *entry,
          uint32_t flags);

FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirectoryHandle handle,
               FSDirectoryEntry *entry,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirectoryHandle handle,
           uint32_t flags);

FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirectoryHandle handle,
                uint32_t flags,
                FSAsyncData *asyncData);

FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *stat,
              uint32_t flags);

FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *stat,
                   uint32_t flags,
                   FSAsyncData *asyncData);

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
FSReadFileWithPosAsync(FSClient *client,
                       FSCmdBlock *block,
                       uint8_t *buffer,
                       uint32_t size,
                       uint32_t count,
                       uint32_t pos,
                       FSFileHandle handle,
                       uint32_t unk1,
                       uint32_t flags,
                       FSAsyncData *asyncData);

FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle fileHandle,
             be_val<uint32_t> *pos,
             uint32_t flags);

FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle fileHandle,
                  be_val<uint32_t> *pos,
                  uint32_t flags,
                  FSAsyncData *asyncData);

FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             uint32_t pos,
             uint32_t flags);

FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  uint32_t pos,
                  uint32_t flags,
                  FSAsyncData *asyncData);

FSVolumeState
FSGetVolumeState(FSClient *client);

FSError
FSGetLastErrorCodeForViewer(FSClient *client);
