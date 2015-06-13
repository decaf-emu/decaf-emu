#include "coreinit.h"
#include "coreinit_fs.h"

void
FSInit()
{
}

void
FSShutdown()
{
}

FSError
FSAddClient(FSClient *client, uint32_t flags)
{
   return FSError::OK;
}

void
FSInitCmdBlock(FSCmdBlock *block)
{
}

FSError
FSGetStat(FSClient *client, FSCmdBlock *block, const char *filepath, FSStat *stat, uint32_t flags)
{
   return FSError::Generic;
}

void
FSSetStateChangeNotification(FSClient *client, FSStateChangeInfo *info)
{
}

FSError
FSOpenFile(FSClient *client, FSCmdBlock *block, const char *path, const char *mode, p32<FSFileHandle> *fileHandle, uint32_t flags)
{
   return FSError::Generic;
}

void
CoreInit::registerFileSystemFunctions()
{
   RegisterSystemFunction(FSInit);
   RegisterSystemFunction(FSShutdown);
   RegisterSystemFunction(FSAddClient);
   RegisterSystemFunction(FSInitCmdBlock);
   RegisterSystemFunction(FSGetStat);
   RegisterSystemFunction(FSSetStateChangeNotification);
   RegisterSystemFunction(FSOpenFile);
}
