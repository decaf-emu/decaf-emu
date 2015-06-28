#pragma once
#include "systemtypes.h"

#pragma pack(push, 1)

struct FSClient
{
   UNKNOWN(0x1700); // memset in FSAddClient
};

struct FSCmdBlock
{
   UNKNOWN(0xa80); // memset in FSInitCmdBlock
};

struct FSStat
{
   UNKNOWN(0x10);
   be_val<uint32_t> size;
   UNKNOWN(0x50);
};
CHECK_SIZE(FSStat, 0x64); // 0x64 from memmove in FSAGetStatFile

struct FSStateChangeInfo
{
   UNKNOWN(0xc); // Copy loop at FSSetStateChangeNotification
};

#pragma pack(pop)

// Thank you cafe_tank for FS_STATUS_x
enum class FSStatus : int32_t
{
   OK = 0,
   Cancelled = -1,
   End = -2,
   Max = -3,
   AlreadyOpen = -4,
   Exists = -5,
   NotFound = -6,
   NotFile = -7,
   NotDir = -8,
   AccessError = -9,
   PermissionError = -10,
   FileTooBig = -11,
   StorageFull = -12,
   JournalFull = -13,
   UnsupportedCmd = -14,
   MediaNotReady = -15,
   MediaError = -17,
   Corrupted = -18,
   FatalError = -0x400,
};

// Thank you cafe_tank for FS_ERROR_x
enum class FSError : int32_t
{
   // FS_ERROR_x
   NotInit = -0x30001,
   Busy = -0x30002,
   Cancelled = -0x30003,
   EndOfDir = -0x30004,
   EndOfFile = -0x30005,

   MaxMountpoints = -0x30010,
   MaxVolumes = -0x30011,
   MaxClients = -0x30012,
   MaxFiles = -0x30013,
   MaxDirs = -0x30014,
   AlreadyOpen = -0x30015,
   AlreadyExists = -0x30016,
   NotFound = -0x30017,
   NotEmpty = -0x30018,
   AcessError = -0x30019,
   PermissionError = -0x3001a,
   DataCorrupted = -0x3001b,
   StorageFull = -0x3001c,
   JournalFull = -0x3001d,

   UnavailableCommand = -0x3001f,
   UnsupportedCommand = -0x30020,
   InvalidParam = -0x30021,
   InvalidPath = -0x30022,
   InvalidBuffer = -0x30023,
   InvalidAlignment = -0x30024,
   InvalidClientHandle = -0x30025,
   InvalidFileHandle = -0x30026,
   InvalidDirHandle = -0x30027,
   NotFile = -0x30028,
   NotDir = -0x30029,
   FileTooBig = -0x3002a,
   OutOfRange = -0x3002b,
   OutOfResources = -0x3002c,

   MediaNotReady = -0x30030,
   MediaError = -0x30031,
   WriteProtected = -0x30032,
   InvalidMedia = -0x30033,
};

using FSFileHandle = uint32_t;
using FSPriority = uint32_t;

void
FSInit();

void
FSShutdown();

FSStatus
FSAddClient(FSClient *client, uint32_t flags);

void
FSInitCmdBlock(FSCmdBlock *block);

FSStatus
FSSetCmdPriority(FSCmdBlock *block, FSPriority priority);

FSStatus
FSGetStat(FSClient *client, FSCmdBlock *block, const char *filepath, FSStat *stat, uint32_t flags);

void
FSSetStateChangeNotification(FSClient *client, FSStateChangeInfo *info);

FSStatus
FSOpenFile(FSClient *client, FSCmdBlock *block, const char *path, const char *mode, be_val<FSFileHandle> *fileHandle, uint32_t flags);

FSStatus
FSGetStatFile(FSClient *client, FSCmdBlock *block, FSFileHandle fileHandle, FSStat *stat, uint32_t flags);

FSStatus
FSCloseFile(FSClient *client, FSCmdBlock *block, FSFileHandle fileHandle, uint32_t flags);

FSStatus
FSReadFile(FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle fileHandle, uint32_t unk1, uint32_t flags);
