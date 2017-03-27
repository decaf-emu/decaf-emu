#include "nn_save.h"
#include "nn_save_cmd.h"
#include "nn_save_core.h"
#include "modules/coreinit/coreinit_fs_cmd.h"

namespace nn
{

namespace save
{


SaveStatus
SAVEChangeGroupAndOthersMode(FSClient *client,
                             FSCmdBlock *block,
                             uint8_t account,
                             const char *path,
                             uint32_t mode,
                             FSErrorFlag errorMask)
{
   // TODO: SAVEChangeGroupAndOthersMode
   decaf_warn_stub();
   return SaveStatus::OK;
}

SaveStatus
SAVEFlushQuota(FSClient *client,
               FSCmdBlock *block,
               uint8_t account,
               FSErrorFlag errorMask)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSFlushQuota(client,
                       block,
                       fsPath.path().c_str(),
                       errorMask);
}


SaveStatus
SAVEFlushQuotaAsync(FSClient *client,
                    FSCmdBlock *block,
                    uint8_t account,
                    FSErrorFlag errorMask,
                    FSAsyncData *asyncData)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSFlushQuotaAsync(client,
                            block,
                            fsPath.path().c_str(),
                            errorMask,
                            asyncData);
}


SaveStatus
SAVEGetFreeSpaceSize(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t account,
                     be_val<uint64_t> *freeSpace,
                     FSErrorFlag errorMask)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSGetFreeSpaceSize(client,
                             block,
                             fsPath.path().c_str(),
                             freeSpace,
                             errorMask);
}


SaveStatus
SAVEGetFreeSpaceSizeAsync(FSClient *client,
                          FSCmdBlock *block,
                          uint8_t account,
                          be_val<uint64_t> *freeSpace,
                          FSErrorFlag errorMask,
                          FSAsyncData *asyncData)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSGetFreeSpaceSizeAsync(client,
                                  block,
                                  fsPath.path().c_str(),
                                  freeSpace,
                                  errorMask,
                                  asyncData);
}


SaveStatus
SAVEGetStat(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            FSStat *stat,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSGetStat(client,
                    block,
                    fsPath.path().c_str(),
                    stat,
                    errorMask);
}


SaveStatus
SAVEGetStatAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 FSStat *stat,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSGetStatAsync(client,
                         block,
                         fsPath.path().c_str(),
                         stat,
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEMakeDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSMakeDir(client,
                    block,
                    fsPath.path().c_str(),
                    errorMask);
}


SaveStatus
SAVEMakeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSMakeDirAsync(client,
                         block,
                         fsPath.path().c_str(),
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEOpenDir(FSClient *client,
            FSCmdBlock *block,
            uint8_t account,
            const char *path,
            be_val<FSDirHandle> *handle,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSOpenDir(client,
                    block,
                    fsPath.path().c_str(),
                    handle,
                    errorMask);
}


SaveStatus
SAVEOpenDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 uint8_t account,
                 const char *path,
                 be_val<FSDirHandle> *handle,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSOpenDirAsync(client,
                         block,
                         fsPath.path().c_str(),
                         handle,
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEOpenFile(FSClient *client,
             FSCmdBlock *block,
             uint8_t account,
             const char *path,
             const char *mode,
             be_val<FSFileHandle> *handle,
             FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSOpenFile(client,
                     block,
                     fsPath.path().c_str(),
                     mode,
                     handle,
                     errorMask);
}


SaveStatus
SAVEOpenFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t account,
                  const char *path,
                  const char *mode,
                  be_val<FSFileHandle> *handle,
                  FSErrorFlag errorMask,
                  FSAsyncData *asyncData)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSOpenFileAsync(client,
                          block,
                          fsPath.path().c_str(),
                          mode,
                          handle,
                          errorMask,
                          asyncData);
}


SaveStatus
SAVERemove(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *path,
           FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSRemove(client,
                   block,
                   fsPath.path().c_str(),
                   errorMask);
}


SaveStatus
SAVERemoveAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *path,
                FSErrorFlag errorMask,
                FSAsyncData *asyncData)
{
   auto fsPath = internal::getSavePath(account, path);

   return FSRemoveAsync(client,
                        block,
                        fsPath.path().c_str(),
                        errorMask,
                        asyncData);
}


SaveStatus
SAVERename(FSClient *client,
           FSCmdBlock *block,
           uint8_t account,
           const char *src,
           const char *dst,
           FSErrorFlag errorMask)
{
   auto srcPath = internal::getSavePath(account, src);
   auto dstPath = internal::getSavePath(account, dst);

   return FSRename(client,
                   block,
                   srcPath.path().c_str(),
                   dstPath.path().c_str(),
                   errorMask);
}


SaveStatus
SAVERenameAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t account,
                const char *src,
                const char *dst,
                FSErrorFlag errorMask,
                FSAsyncData *asyncData)
{
   auto srcPath = internal::getSavePath(account, src);
   auto dstPath = internal::getSavePath(account, dst);

   return FSRenameAsync(client,
                        block,
                        srcPath.path().c_str(),
                        dstPath.path().c_str(),
                        errorMask,
                        asyncData);
}


void
Module::registerCmdFunctions()
{
   RegisterKernelFunction(SAVEChangeGroupAndOthersMode);

   RegisterKernelFunction(SAVEFlushQuotaAsync);
   RegisterKernelFunction(SAVEFlushQuota);
   RegisterKernelFunction(SAVEGetFreeSpaceSizeAsync);
   RegisterKernelFunction(SAVEGetFreeSpaceSize);
   RegisterKernelFunction(SAVEGetStatAsync);
   RegisterKernelFunction(SAVEGetStat);
   RegisterKernelFunction(SAVEMakeDir);
   RegisterKernelFunction(SAVEMakeDirAsync);
   RegisterKernelFunction(SAVEOpenDir);
   RegisterKernelFunction(SAVEOpenDirAsync);
   RegisterKernelFunction(SAVEOpenFile);
   RegisterKernelFunction(SAVEOpenFileAsync);
   RegisterKernelFunction(SAVERemoveAsync);
   RegisterKernelFunction(SAVERemove);
   RegisterKernelFunction(SAVERenameAsync);
   RegisterKernelFunction(SAVERename);
}

} // namespace save

} // namespace nn
