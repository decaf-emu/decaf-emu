#include "ios_acp.h"
#include "ios_acp_nn_saveservice.h"

#include "ios/ios_stackobject.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/acp/nn_acp_result.h"

#include <fmt/core.h>

using namespace ios::fs;
using namespace nn::acp;
using namespace nn::ipc;

namespace ios::acp::internal
{

static nn::Result
translateError(FSAStatus status)
{
   switch (status) {
   case FSAStatus::OK:
      return nn::ResultSuccess;
   case FSAStatus::NotInit:
      return ResultFsNotInit;
   case FSAStatus::Busy:
      return ResultFsBusy;
   case FSAStatus::Cancelled:
      return ResultFsCancelled;
   case FSAStatus::EndOfDir:
   case FSAStatus::EndOfFile:
   case FSAStatus::MaxVolumes:
   case FSAStatus::MaxClients:
   case FSAStatus::MaxFiles:
   case FSAStatus::MaxDirs:
   case FSAStatus::MaxMountpoints:
   case FSAStatus::OutOfRange:
   case FSAStatus::OutOfResources:
      return ResultFsEnd;
   case FSAStatus::NotFound:
      return ResultFsNotFound;
   case FSAStatus::AlreadyOpen:
      return ResultFsAlreadyOpen;
   case FSAStatus::AlreadyExists:
      return ResultFsAlreadyExists;
   case FSAStatus::NotEmpty:
   case FSAStatus::FileTooBig:
      return ResultFsUnexpectedFileSize;
   case FSAStatus::DataCorrupted:
      return ResultDataCorrupted;
   case FSAStatus::PermissionError:
      return ResultFsPermissionError;
   case FSAStatus::StorageFull:
      return ResultStorageFull;
   case FSAStatus::JournalFull:
      return ResultJournalFull;
   case FSAStatus::AccessError:
      return ResultFsAccessError;
   case FSAStatus::UnavailableCmd:
   case FSAStatus::UnsupportedCmd:
   case FSAStatus::InvalidParam:
   case FSAStatus::InvalidPath:
   case FSAStatus::InvalidBuffer:
   case FSAStatus::InvalidAlignment:
   case FSAStatus::InvalidClientHandle:
   case FSAStatus::InvalidFileHandle:
   case FSAStatus::InvalidDirHandle:
   case FSAStatus::NotFile:
   case FSAStatus::NotDir:
      return ResultFsInvalid;
   case FSAStatus::MediaNotReady:
      return ResultMediaNotReady;
   case FSAStatus::MediaError:
      return ResultMediaError;
   case FSAStatus::WriteProtected:
      return ResultWriteProtected;
   default:
      return ResultFatalFilesystemError;
   }
}

static nn::Result
CreateDir(std::string_view path)
{
   auto status = FSAMakeDir(getFsaHandle(), path, 0x600);
   if (status == FSAStatus::OK || status == FSAStatus::AlreadyExists) {
      return nn::ResultSuccess;
   }

   return translateError(status);
}

static nn::Result
MakeQuota(std::string_view path,
          uint32_t mode,
          uint64_t size)
{
   auto status = FSAMakeQuota(getFsaHandle(), path, mode, size);
   if (status == FSAStatus::OK || status == FSAStatus::AlreadyExists) {
      return nn::ResultSuccess;
   }

   return translateError(status);
}

static std::string
GetAccountSaveDirPath(uint32_t persistentId,
                      TitleId titleId)
{
   auto titleHi = static_cast<uint32_t>(titleId >> 32);
   auto titleLo = static_cast<uint32_t>(titleId & 0xFFFFFFFF);

   if (persistentId == 0) {
      return fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user/common",
                         titleHi, titleLo);
   } else {
      return fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user/{:08X}",
                         titleHi, titleLo, persistentId);
   }
}

static std::string
GetAccountBossDirPath(uint32_t persistentId,
                      TitleId titleId)
{
   auto titleHi = static_cast<uint32_t>(titleId >> 32);
   auto titleLo = static_cast<uint32_t>(titleId & 0xFFFFFFFF);

   if (persistentId == 0) {
      return fmt::format("/vol/storage_mlc01/usr/boss/{:08x}/{:08x}/user/common",
                         titleHi, titleLo);
   } else {
      return fmt::format("/vol/storage_mlc01/usr/boss/{:08x}/{:08x}/user/{:08X}",
                         titleHi, titleLo, persistentId);
   }
}

static nn::Result
CreateSaveUserDir(TitleId titleId)
{
   auto titleHi = static_cast<uint32_t>(titleId >> 32);
   auto titleLo = static_cast<uint32_t>(titleId & 0xFFFFFFFF);

   auto result = CreateDir(fmt::format("/vol/storage_mlc01/usr/save/{:08x}", titleHi));
   if (!result) {
      return result;
   }

   result = CreateDir(fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}", titleHi, titleLo));
   if (!result) {
      return result;
   }

   result = CreateDir(fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user", titleHi, titleLo));
   if (!result) {
      return result;
   }

   return nn::ResultSuccess;
}

static nn::Result
CreateSaveMetaFiles(TitleId titleId)
{
   auto titleHi = static_cast<uint32_t>(titleId >> 32);
   auto titleLo = static_cast<uint32_t>(titleId & 0xFFFFFFFF);

   auto result = CreateDir(fmt::format("/vol/storage_mlc01/usr/save/{:08x}", titleHi));
   if (!result) {
      return result;
   }

   result = CreateDir(fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}", titleHi, titleLo));
   if (!result) {
      return result;
   }

   result = MakeQuota(fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/meta", titleHi, titleLo), 0x640, 0x80000);
   if (!result) {
      return result;
   }

   // TODO: FSAChangeMode(path, 0x640)
   // TODO: Copy current title meta.xml to save meta
   // TODO: Copy current title iconTex.tga to save meta
   return nn::ResultSuccess;
}

static nn::Result
CreateBossStorage(uint32_t persistentId,
                  TitleId titleId)
{
   // TODO: Only create if meta->common_boss_size || meta->account_boss_size > 0
   auto titleHi = static_cast<uint32_t>(titleId >> 32);
   auto titleLo = static_cast<uint32_t>(titleId & 0xFFFFFFFF);
   auto result = CreateDir(fmt::format("/vol/storage_mlc01/usr/boss/{:08x}", titleHi));
   if (!result) {
      return result;
   }

   result = CreateDir(fmt::format("/vol/storage_mlc01/usr/boss/{:08x}/{:08x}", titleHi, titleLo));
   if (!result) {
      return result;
   }

   result = CreateDir(fmt::format("/vol/storage_mlc01/usr/boss/{:08x}/{:08x}/user", titleHi, titleLo));
   if (!result) {
      return result;
   }

   // TODO: Use meta->account_boss_size, only create if > 0
   result = MakeQuota(GetAccountBossDirPath(persistentId, titleId), 0x600, 0x40000);
   if (!result) {
      return result;
   }

   // TODO: Use meta->common_boss_size, only create if > 0
   result = MakeQuota(GetAccountBossDirPath(0, titleId), 0x600, 0x40000);
   if (!result) {
      return result;
   }

   // TODO: Change owner to bossOwner
   auto bossOwner = FSAProcessInfo { };
   bossOwner.titleId = 0x100000F3ull;
   bossOwner.processId = ProcessId::NIM;
   bossOwner.groupId = 0x400u;

   return nn::ResultSuccess;
}

static nn::Result
CreateSaveDirAndBossStorageImpl(uint32_t persistentId,
                                TitleId titleId)
{
   auto result = CreateSaveUserDir(titleId);
   if (!result) {
      return result;
   }

   // TODO: Use meta->account_save_size, only create if > 0
   result = MakeQuota(GetAccountSaveDirPath(persistentId, titleId), 0x660, 0x40000);
   if (!result) {
      return result;
   }

   // TODO: Use meta->common_save_size, only create if > 0
   result = MakeQuota(GetAccountSaveDirPath(0, titleId), 0x660, 0x40000);
   if (!result) {
      return result;
   }

   return CreateBossStorage(persistentId, titleId);
}

static nn::Result
CreateSaveDirAndBossStorage(uint32_t persistentId,
                            TitleId titleId)
{
   auto result = CreateSaveMetaFiles(titleId);
   if (!result) {
      return result;
   }

   result = CreateSaveDirAndBossStorageImpl(persistentId, titleId);
   if (!result) {
      return result;
   }

   /*
   TODO: UpdateSaveTimeStamp
      Sends a message to the save timestamp thread which will do the work.
      Thread updates saveinfo.xml with:
      <?xml version="1.0" encoding="utf-8"?>
      <info>
         <account persistentId="00000000">
            <timestamp>0000000023cc86be</timestamp>
         </account>
      </info>
   */

   return nn::ResultSuccess;
}

nn::Result
createSaveDirInternalEx(uint32_t persistentId,
                        TitleId titleId,
                        ProcessId processId,
                        GroupId groupId)
{
   return CreateSaveDirAndBossStorage(persistentId, titleId);
}

static nn::Result
createSaveDir(CommandHandlerArgs &args)
{
   auto command = ServerCommand<SaveService::CreateSaveDir> { args };
   auto deviceType = ACPDeviceType { };
   auto persistentId = uint32_t { 0 };
   command.ReadRequest(persistentId, deviceType);

   // TODO: Check that account with persistentId exists
   // 0xA0335200 = AccountNotExist
   return createSaveDirInternalEx(persistentId,
                                  args.resourceRequest->requestData.titleId,
                                  args.resourceRequest->requestData.processId,
                                  args.resourceRequest->requestData.groupId);
}

static nn::Result
repairSaveMetaDir(CommandHandlerArgs &args)
{
   // TODO: repairSaveMetaDir
   return nn::ResultSuccess;
}

static nn::Result
mountSaveDir(CommandHandlerArgs &args)
{
   auto titleHi = static_cast<uint32_t>(args.resourceRequest->requestData.titleId >> 32);
   auto titleLo = static_cast<uint32_t>(args.resourceRequest->requestData.titleId & 0xFFFFFFFF);

   /* Create the save user and common dir just incase it does not exist.
    * Note: The real MountSaveDir handler in IOS does not create these paths,
    * only mounts /vol/save, this must mean the initial path creation is done
    * somewhere else - I do not know where yet.
    */
   CreateSaveUserDir(args.resourceRequest->requestData.titleId);
   CreateDir(GetAccountSaveDirPath(0, args.resourceRequest->requestData.titleId));

   auto processInfo = FSAProcessInfo { };
   processInfo.processId = args.resourceRequest->requestData.processId;
   processInfo.titleId = args.resourceRequest->requestData.titleId;
   processInfo.groupId = args.resourceRequest->requestData.groupId;

   auto fsaStatus =
      FSAMountWithProcess(getFsaHandle(),
                          fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user", titleHi, titleLo),
                          "/vol/save", FSAMountPriority::Base,
                          &processInfo, nullptr, 0u);
   if (fsaStatus != FSAStatus::OK) {
      return translateError(fsaStatus);
   }

   return nn::ResultSuccess;
}

static nn::Result
unmountSaveDir(CommandHandlerArgs &args)
{
   auto processInfo = FSAProcessInfo { };
   processInfo.processId = args.resourceRequest->requestData.processId;
   processInfo.titleId = args.resourceRequest->requestData.titleId;
   processInfo.groupId = args.resourceRequest->requestData.groupId;

   auto fsaStatus = FSAUnmountWithProcess(getFsaHandle(), "/vol/save",
                                          FSAMountPriority::UnmountAll,
                                          &processInfo);
   if (fsaStatus != FSAStatus::OK) {
      return translateError(fsaStatus);
   }

   return nn::ResultSuccess;
}

static nn::Result
isExternalStorageRequired(CommandHandlerArgs &args)
{
   auto command = ServerCommand<SaveService::IsExternalStorageRequired> { args };
   command.WriteResponse(0);
   return nn::ResultSuccess;
}

static nn::Result
mountExternalStorage(CommandHandlerArgs &args)
{
   return nn::ResultSuccess;
}

static nn::Result
unmountExternalStorage(CommandHandlerArgs &args)
{
   return nn::ResultSuccess;
}

nn::Result
SaveService::commandHandler(uint32_t unk1,
                            CommandId command,
                            CommandHandlerArgs &args)
{
   switch (command) {
   case CreateSaveDir::command:
      return createSaveDir(args);
   case IsExternalStorageRequired::command:
      return isExternalStorageRequired(args);
   case MountExternalStorage::command:
      return mountExternalStorage(args);
   case UnmountExternalStorage::command:
      return unmountExternalStorage(args);
   case MountSaveDir::command:
      return mountSaveDir(args);
   case UnmountSaveDir::command:
      return unmountSaveDir(args);
   case RepairSaveMetaDir::command:
      return repairSaveMetaDir(args);
   default:
      return ResultSuccess;
   }
}

} // namespace ios::acp::internal
