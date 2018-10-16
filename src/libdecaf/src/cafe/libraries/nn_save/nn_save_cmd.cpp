#include "nn_save.h"
#include "nn_save_cmd.h"
#include "nn_save_path.h"
#include "cafe/libraries/coreinit/coreinit_fs_cmd.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/cafe_stackobject.h"

namespace cafe::nn_save
{

SaveStatus
SAVEChangeGroupAndOthersMode(virt_ptr<FSClient> client,
                             virt_ptr<FSCmdBlock> block,
                             uint8_t account,
                             virt_ptr<const char> path,
                             uint32_t mode,
                             FSErrorFlag errorMask)
{
   // TODO: SAVEChangeGroupAndOthersMode
   decaf_warn_stub();
   return SaveStatus::OK;
}

SaveStatus
SAVEFlushQuota(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               uint8_t account,
               FSErrorFlag errorMask)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSFlushQuota(client,
                       block,
                       make_stack_string(fsPath.path()),
                       errorMask);
}


SaveStatus
SAVEFlushQuotaAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    uint8_t account,
                    FSErrorFlag errorMask,
                    virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSFlushQuotaAsync(client,
                            block,
                            make_stack_string(fsPath.path()),
                            errorMask,
                            asyncData);
}


SaveStatus
SAVEGetFreeSpaceSize(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     uint8_t account,
                     virt_ptr<uint64_t> freeSpace,
                     FSErrorFlag errorMask)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSGetFreeSpaceSize(client,
                             block,
                             make_stack_string(fsPath.path()),
                             freeSpace,
                             errorMask);
}


SaveStatus
SAVEGetFreeSpaceSizeAsync(virt_ptr<FSClient> client,
                          virt_ptr<FSCmdBlock> block,
                          uint8_t account,
                          virt_ptr<uint64_t> freeSpace,
                          FSErrorFlag errorMask,
                          virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSaveDirectory(account);

   return FSGetFreeSpaceSizeAsync(client,
                                  block,
                                  make_stack_string(fsPath.path()),
                                  freeSpace,
                                  errorMask,
                                  asyncData);
}


SaveStatus
SAVEGetStat(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            virt_ptr<FSStat> stat,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSGetStat(client,
                    block,
                    make_stack_string(fsPath.path()),
                    stat,
                    errorMask);
}


SaveStatus
SAVEGetStatAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 virt_ptr<FSStat> stat,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSGetStatAsync(client,
                         block,
                         make_stack_string(fsPath.path()),
                         stat,
                         errorMask,
                         asyncData);
}

SaveStatus
SAVEGetStatOtherApplication(virt_ptr<FSClient> client,
                            virt_ptr<FSCmdBlock> block,
                            uint64_t titleId,
                            uint8_t account,
                            virt_ptr<const char> path,
                            virt_ptr<FSStat> stat,
                            FSErrorFlag errorMask)
{
   auto fsPath = internal::getTitleSavePath(titleId, account, path.get());

   return FSGetStat(client,
                    block,
                    make_stack_string(fsPath.path()),
                    stat,
                    errorMask);
}

SaveStatus
SAVEGetStatOtherApplicationAsync(virt_ptr<FSClient> client,
                                 virt_ptr<FSCmdBlock> block,
                                 uint64_t titleId,
                                 uint8_t account,
                                 virt_ptr<const char> path,
                                 virt_ptr<FSStat> stat,
                                 FSErrorFlag errorMask,
                                 virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getTitleSavePath(titleId, account, path.get());

   return FSGetStatAsync(client,
                         block,
                         make_stack_string(fsPath.path()),
                         stat,
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEMakeDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSMakeDir(client,
                    block,
                    make_stack_string(fsPath.path()),
                    errorMask);
}


SaveStatus
SAVEMakeDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSMakeDirAsync(client,
                         block,
                         make_stack_string(fsPath.path()),
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEOpenDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            uint8_t account,
            virt_ptr<const char> path,
            virt_ptr<FSDirHandle> handle,
            FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSOpenDir(client,
                    block,
                    make_stack_string(fsPath.path()),
                    handle,
                    errorMask);
}


SaveStatus
SAVEOpenDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 uint8_t account,
                 virt_ptr<const char> path,
                 virt_ptr<FSDirHandle> handle,
                 FSErrorFlag errorMask,
                 virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSOpenDirAsync(client,
                         block,
                         make_stack_string(fsPath.path()),
                         handle,
                         errorMask,
                         asyncData);
}


SaveStatus
SAVEOpenFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             uint8_t account,
             virt_ptr<const char> path,
             virt_ptr<const char> mode,
             virt_ptr<FSFileHandle> handle,
             FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSOpenFile(client,
                     block,
                     make_stack_string(fsPath.path()),
                     mode,
                     handle,
                     errorMask);
}


SaveStatus
SAVEOpenFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  uint8_t account,
                  virt_ptr<const char> path,
                  virt_ptr<const char> mode,
                  virt_ptr<FSFileHandle> handle,
                  FSErrorFlag errorMask,
                  virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSOpenFileAsync(client,
                          block,
                          make_stack_string(fsPath.path()),
                          mode,
                          handle,
                          errorMask,
                          asyncData);
}


SaveStatus
SAVERemove(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           uint8_t account,
           virt_ptr<const char> path,
           FSErrorFlag errorMask)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSRemove(client,
                   block,
                   make_stack_string(fsPath.path()),
                   errorMask);
}


SaveStatus
SAVERemoveAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                uint8_t account,
                virt_ptr<const char> path,
                FSErrorFlag errorMask,
                virt_ptr<FSAsyncData> asyncData)
{
   auto fsPath = internal::getSavePath(account, path.get());

   return FSRemoveAsync(client,
                        block,
                        make_stack_string(fsPath.path()),
                        errorMask,
                        asyncData);
}


SaveStatus
SAVERename(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           uint8_t account,
           virt_ptr<const char> src,
           virt_ptr<const char> dst,
           FSErrorFlag errorMask)
{
   auto srcPath = internal::getSavePath(account, src.get());
   auto dstPath = internal::getSavePath(account, dst.get());

   return FSRename(client,
                   block,
                   make_stack_string(srcPath.path()),
                   make_stack_string(dstPath.path()),
                   errorMask);
}


SaveStatus
SAVERenameAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                uint8_t account,
                virt_ptr<const char> src,
                virt_ptr<const char> dst,
                FSErrorFlag errorMask,
                virt_ptr<FSAsyncData> asyncData)
{
   auto srcPath = internal::getSavePath(account, src.get());
   auto dstPath = internal::getSavePath(account, dst.get());

   return FSRenameAsync(client,
                        block,
                        make_stack_string(srcPath.path()),
                        make_stack_string(dstPath.path()),
                        errorMask,
                        asyncData);
}

void
Library::registerCmdSymbols()
{
   RegisterFunctionExport(SAVEChangeGroupAndOthersMode);
   RegisterFunctionExport(SAVEFlushQuotaAsync);
   RegisterFunctionExport(SAVEFlushQuota);
   RegisterFunctionExport(SAVEGetFreeSpaceSizeAsync);
   RegisterFunctionExport(SAVEGetFreeSpaceSize);
   RegisterFunctionExport(SAVEGetStatAsync);
   RegisterFunctionExport(SAVEGetStat);
   RegisterFunctionExport(SAVEGetStatOtherApplication);
   RegisterFunctionExport(SAVEGetStatOtherApplicationAsync);
   RegisterFunctionExport(SAVEMakeDir);
   RegisterFunctionExport(SAVEMakeDirAsync);
   RegisterFunctionExport(SAVEOpenDir);
   RegisterFunctionExport(SAVEOpenDirAsync);
   RegisterFunctionExport(SAVEOpenFile);
   RegisterFunctionExport(SAVEOpenFileAsync);
   RegisterFunctionExport(SAVERemoveAsync);
   RegisterFunctionExport(SAVERemove);
   RegisterFunctionExport(SAVERenameAsync);
   RegisterFunctionExport(SAVERename);
}

} // namespace cafe::nn_save
