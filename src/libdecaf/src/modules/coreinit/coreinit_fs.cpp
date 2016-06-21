#include "coreinit.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_dir.h"
#include "coreinit_fs_file.h"
#include "coreinit_fs_path.h"
#include "coreinit_fs_stat.h"
#include "filesystem/filesystem.h"

namespace coreinit
{

void
FSInit()
{
}


void
FSShutdown()
{
}


void
FSInitCmdBlock(FSCmdBlock *block)
{
}


FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 FSPriority priority)
{
   return FSStatus::OK;
}

void
Module::registerFileSystemFunctions()
{
   RegisterKernelFunction(FSInit);
   RegisterKernelFunction(FSShutdown);
   RegisterKernelFunction(FSAddClient);
   RegisterKernelFunction(FSAddClientEx);
   RegisterKernelFunction(FSDelClient);
   RegisterKernelFunction(FSGetClientNum);
   RegisterKernelFunction(FSInitCmdBlock);
   RegisterKernelFunction(FSSetCmdPriority);
   RegisterKernelFunction(FSSetStateChangeNotification);
   RegisterKernelFunction(FSGetVolumeState);
   RegisterKernelFunction(FSGetLastErrorCodeForViewer);
   RegisterKernelFunction(FSGetAsyncResult);

   // coreinit_fs_path
   RegisterKernelFunction(FSGetCwd);
   RegisterKernelFunction(FSChangeDir);
   RegisterKernelFunction(FSChangeDirAsync);

   // coreinit_fs_stat
   RegisterKernelFunction(FSGetStat);
   RegisterKernelFunction(FSGetStatAsync);
   RegisterKernelFunction(FSGetStatFile);
   RegisterKernelFunction(FSGetStatFileAsync);

   // coreinit_fs_file
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSWriteFile);
   RegisterKernelFunction(FSWriteFileAsync);
   RegisterKernelFunction(FSWriteFileWithPos);
   RegisterKernelFunction(FSWriteFileWithPosAsync);
   RegisterKernelFunction(FSIsEof);
   RegisterKernelFunction(FSIsEofAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSGetPosFileAsync);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSSetPosFileAsync);
   RegisterKernelFunction(FSTruncateFile);
   RegisterKernelFunction(FSTruncateFileAsync);

   // coreinit_fs_dir
   RegisterKernelFunction(FSOpenDir);
   RegisterKernelFunction(FSOpenDirAsync);
   RegisterKernelFunction(FSCloseDir);
   RegisterKernelFunction(FSCloseDirAsync);
   RegisterKernelFunction(FSMakeDir);
   RegisterKernelFunction(FSMakeDirAsync);
   RegisterKernelFunction(FSReadDir);
   RegisterKernelFunction(FSReadDirAsync);
   RegisterKernelFunction(FSRewindDir);
   RegisterKernelFunction(FSRewindDirAsync);
}

} // namespace coreinit
