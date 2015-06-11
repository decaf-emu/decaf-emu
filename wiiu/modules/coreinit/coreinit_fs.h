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
   UNKNOWN(0x64); // memmove in FSAGetStatFile
};

struct FSStateChangeInfo
{
   UNKNOWN(0xc); // Copy loop at FSSetStateChangeNotification
};

#pragma pack(pop)

enum class FSError : int32_t
{
   OK       = 0,
   Generic  = -0x400
};

void
FSInit();

void
FSShutdown();

FSError
FSAddClient(FSClient *client, uint32_t flags);

void
FSInitCmdBlock(FSCmdBlock *block);

FSError
FSGetStat(FSClient *client, FSCmdBlock *block, const char *filepath, FSStat *stat, uint32_t flags);

void
FSSetStateChangeNotification(FSClient *client, FSStateChangeInfo *info);
