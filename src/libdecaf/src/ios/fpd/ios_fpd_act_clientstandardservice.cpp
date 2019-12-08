#include "ios_fpd_act_accountdata.h"
#include "ios_fpd_act_clientstandardservice.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_log.h"

#include "ios/ios_stackobject.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_timer.h"
#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>

using namespace nn::ipc;
using namespace nn::act;
using namespace nn::ffl;
using namespace ios::fs;
using namespace ios::kernel;

namespace ios::fpd::internal
{

static nn::Result
getCommonInfo(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetCommonInfo> { args };
   auto buffer = OutBuffer<void> { };
   auto type = InfoType { 0 };
   command.ReadRequest(buffer, type);

   switch (type) {
   case InfoType::NumOfAccounts:
   {
      if (buffer.totalSize() < sizeof(uint8_t)) {
         return ResultInvalidSize;
      }

      uint8_t numAccounts = getNumAccounts();
      buffer.writeOutput(&numAccounts, sizeof(uint8_t));
      break;
   }
   case InfoType::SlotNo:
   {
      if (buffer.totalSize() < sizeof(SlotNo)) {
         return ResultInvalidSize;
      }

      SlotNo slotNo = getSlotNoForAccount(getCurrentAccount());
      buffer.writeOutput(&slotNo, sizeof(slotNo));
      break;
   }
   case InfoType::DefaultAccount:
   {
      if (buffer.totalSize() < sizeof(SlotNo)) {
         return ResultInvalidSize;
      }

      SlotNo slotNo = getSlotNoForAccount(getDefaultAccount());
      buffer.writeOutput(&slotNo, sizeof(slotNo));
      break;
   }
   case InfoType::NetworkTimeDifference:
      return ResultNotImplemented;
   case InfoType::LocalFriendCode:
   {
      auto accountManager = getAccountManager();
      if (buffer.totalSize() < sizeof(accountManager->commonTransferableIdBase)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(accountManager->commonTransferableIdBase),
                         sizeof(accountManager->commonTransferableIdBase));
      break;
   }
   case InfoType::ApplicationUpdateRequired:
   {
      auto accountManager = getAccountManager();
      if (buffer.totalSize() < sizeof(accountManager->isApplicationUpdateRequired)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(accountManager->isApplicationUpdateRequired),
                         sizeof(accountManager->isApplicationUpdateRequired));
      break;
   }
   case InfoType::DefaultHostServerSettings:
      return ResultNotImplemented;
   case InfoType::DefaultHostServerSettingsEx:
      return ResultNotImplemented;
   case InfoType::DeviceHash:
   {
      auto hash = getDeviceHash();
      if (buffer.totalSize() < hash.size()) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(hash.data(), hash.size());
      break;
   }
   case InfoType::NetworkTime:
      return ResultNotImplemented;
   default:
      return ResultInvalidValue;
   }

   return nn::ResultSuccess;
}

static nn::Result
getAccountInfo(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetAccountInfo> { args };
   auto slotNo = InvalidSlot;
   auto buffer = OutBuffer<void> { };
   auto type = InfoType { 0 };
   command.ReadRequest(slotNo, buffer, type);

   auto account = getAccountBySlotNo(slotNo);
   if (!account) {
      return ResultACCOUNT_NOT_FOUND;
   }

   switch (type) {
   case InfoType::PersistentId:
      if (buffer.totalSize() < sizeof(account->persistentId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->persistentId),
                         sizeof(account->persistentId));
      break;
   case InfoType::LocalFriendCode:
      if (buffer.totalSize() < sizeof(account->transferableIdBase)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->transferableIdBase),
                         sizeof(account->transferableIdBase));
      break;
   case InfoType::Mii:
      if (buffer.totalSize() < sizeof(account->miiData)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->miiData),
                         sizeof(account->miiData));
      break;
   case InfoType::AccountId:
      if (buffer.totalSize() < sizeof(account->accountId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->accountId),
                         sizeof(account->accountId));
      break;
   case InfoType::EmailAddress:
      if (buffer.totalSize() < sizeof(account->emailAddress)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->emailAddress),
                         sizeof(account->emailAddress));
      break;
   case InfoType::Birthday:
   {
      StackObject<Birthday> birthday;
      if (buffer.totalSize() < sizeof(*birthday)) {
         return ResultInvalidSize;
      }

      birthday->year = account->birthYear;
      birthday->month = account->birthMonth;
      birthday->day = account->birthDay;
      buffer.writeOutput(birthday, sizeof(*birthday));
      break;
   }
   case InfoType::Country:
      return ResultNotImplemented;
   case InfoType::PrincipalId:
      if (buffer.totalSize() < sizeof(account->principalId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->principalId),
                         sizeof(account->principalId));
      break;
   case InfoType::IsPasswordCacheEnabled:
      if (buffer.totalSize() < sizeof(account->isPasswordCacheEnabled)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->isPasswordCacheEnabled),
                         sizeof(account->isPasswordCacheEnabled));
      break;
   case InfoType::AccountPasswordCache:
      if (buffer.totalSize() < sizeof(account->accountPasswordCache)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->accountPasswordCache),
                         sizeof(account->accountPasswordCache));
      break;
   case InfoType::AccountInfo:
   case InfoType::HostServerSettings:
      return ResultNotImplemented;
   case InfoType::Gender:
      if (buffer.totalSize() < sizeof(account->gender)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->gender),
                         sizeof(account->gender));
      break;
   case InfoType::LastAuthenticationResult:
      if (buffer.totalSize() < sizeof(account->lastAuthenticationResult)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->lastAuthenticationResult),
                         sizeof(account->lastAuthenticationResult));
      break;
   case InfoType::StickyAccountId:
      if (buffer.totalSize() < sizeof(account->stickyAccountId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->stickyAccountId),
                         sizeof(account->stickyAccountId));
      break;
   case InfoType::ParentalControlSlot:
      if (buffer.totalSize() < sizeof(account->parentalControlSlotNo)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->parentalControlSlotNo),
                         sizeof(account->parentalControlSlotNo));
      break;
   case InfoType::SimpleAddressId:
      if (buffer.totalSize() < sizeof(account->simpleAddressId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->simpleAddressId),
                         sizeof(account->simpleAddressId));
      break;
   case static_cast<InfoType>(25):
      return ResultNotImplemented;
   case InfoType::IsCommitted:
      if (buffer.totalSize() < sizeof(account->isCommitted)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->isCommitted),
                         sizeof(account->isCommitted));
      break;
   case InfoType::MiiName:
      if (buffer.totalSize() < sizeof(account->miiName)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->miiName),
                         sizeof(account->miiName));
      break;
   case InfoType::NfsPassword:
      if (buffer.totalSize() < sizeof(account->nfsPassword)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->nfsPassword),
                         sizeof(account->nfsPassword));
      break;
   case InfoType::HasEciVirtualAccount:
   {
      StackObject<uint8_t> hasEciVirtualAccount;
      if (buffer.totalSize() < sizeof(*hasEciVirtualAccount)) {
         return ResultInvalidSize;
      }

      *hasEciVirtualAccount = account->eciVirtualAccount[0] ? 1 : 0;
      buffer.writeOutput(hasEciVirtualAccount, sizeof(*hasEciVirtualAccount));
      break;
   }
   case InfoType::TimeZoneId:
      if (buffer.totalSize() < sizeof(account->timeZoneId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->timeZoneId),
                         sizeof(account->timeZoneId));
      break;
   case InfoType::IsMiiUpdated:
      if (buffer.totalSize() < sizeof(account->isMiiUpdated)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->isMiiUpdated),
                         sizeof(account->isMiiUpdated));
      break;
   case InfoType::IsMailAddressValidated:
      if (buffer.totalSize() < sizeof(account->isMailAddressValidated)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->isMailAddressValidated),
                         sizeof(account->isMailAddressValidated));
      break;
   case InfoType::NextAccountId:
      if (buffer.totalSize() < sizeof(account->nextAccountId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->nextAccountId),
                         sizeof(account->nextAccountId));
      break;
   case InfoType::Unk34:
      return ResultNotImplemented;
   case InfoType::IsServerAccountDeleted:
      if (buffer.totalSize() < sizeof(account->isServerAccountDeleted)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->isServerAccountDeleted),
                         sizeof(account->isServerAccountDeleted));
      break;
   case InfoType::MiiImageUrl:
      if (buffer.totalSize() < sizeof(account->miiImageUrl)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->miiImageUrl),
                         sizeof(account->miiImageUrl));
      break;
   case InfoType::StickyPrincipalId:
      if (buffer.totalSize() < sizeof(account->stickyPrincipalId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->stickyPrincipalId),
                         sizeof(account->stickyPrincipalId));
      break;
   case InfoType::Unk40:
   case InfoType::Unk41:
      return ResultNotImplemented;
   case InfoType::ServerAccountStatus:
      if (buffer.totalSize() < sizeof(account->serverAccountStatus)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->serverAccountStatus),
                         sizeof(account->serverAccountStatus));
      break;
   default:
      return ResultInvalidValue;
   }

   return nn::ResultSuccess;
}

static nn::Result
getTransferableId(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetTransferableId> { args };
   auto slotNo = InvalidSlot;
   auto unkArg2 = uint32_t { 0 };
   command.ReadRequest(slotNo, unkArg2);

   auto transferableIdBase = TransferrableId { 0 };
   if (slotNo == SystemSlot) {
      transferableIdBase = getAccountManager()->commonTransferableIdBase;
   } else {
      auto account = getAccountBySlotNo(slotNo);
      if (!account) {
         return ResultACCOUNT_NOT_FOUND;
      }

      transferableIdBase = account->transferableIdBase;
   }

   command.WriteResponse(calculateTransferableId(transferableIdBase,
                                                 static_cast<uint16_t>(unkArg2)));
   return nn::ResultSuccess;
}

static nn::Result
getMiiImage(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetMiiImage> { args };
   auto slotNo = InvalidSlot;
   auto buffer = OutBuffer<void> { };
   auto imageType = MiiImageType { 0 };
   command.ReadRequest(slotNo, buffer, imageType);

   auto account = getAccountBySlotNo(slotNo);
   if (!account) {
      return ResultACCOUNT_NOT_FOUND;
   }

   auto path = fmt::format(
      "/vol/storage_mlc01/usr/save/system/act/{:08x}/miiimg{:02}.dat",
      static_cast<uint32_t>(account->persistentId),
      static_cast<int>(imageType));

   // Check that the given buffer is large enough to read file into
   auto stat = StackObject<FSAStat> { };
   auto status = FSAGetInfoByQuery(getActFsaHandle(), path,
                                   FSAQueryInfoType::Stat, stat);
   if (status < FSAStatus::OK) {
      if (status == FSAStatus::NotFound) {
         return ResultFileNotFound;
      } else {
         return ResultFileIoError;
      }
   }

   if (buffer.totalSize() < stat->size) {
      return ResultInvalidSize;
   }

   // Read file split across buffer
   auto fileHandle = FSAFileHandle { -1 };
   status = FSAOpenFile(getActFsaHandle(), path, "r", &fileHandle);
   if (status < FSAStatus::OK) {
      if (status == FSAStatus::NotFound) {
         return ResultFileNotFound;
      } else {
         return ResultFileIoError;
      }
   }

   auto bytesRead = 0u;
   if (bytesRead < stat->size && buffer.unalignedBeforeBufferSize > 0) {
      auto readSize = std::min<uint32_t>(buffer.unalignedBeforeBufferSize,
                                         stat->size - bytesRead);
      status = FSAReadFile(getActFsaHandle(), buffer.unalignedBeforeBuffer,
                           1, readSize, fileHandle, FSAReadFlag::None);
      if (status < FSAStatus::OK || static_cast<uint32_t>(status) != readSize) {
         FSACloseFile(getActFsaHandle(), fileHandle);
         return ResultFileIoError;
      }

      bytesRead += readSize;
   }

   if (bytesRead < stat->size && buffer.alignedBufferSize > 0) {
      auto readSize = std::min<uint32_t>(buffer.alignedBufferSize,
                                         stat->size - bytesRead);
      status = FSAReadFile(getActFsaHandle(), buffer.alignedBuffer,
                           1, readSize, fileHandle, FSAReadFlag::None);
      if (status < FSAStatus::OK || static_cast<uint32_t>(status) != readSize) {
         FSACloseFile(getActFsaHandle(), fileHandle);
         return ResultFileIoError;
      }

      bytesRead += readSize;
   }

   if (bytesRead < stat->size && buffer.unalignedAfterBufferSize > 0) {
      auto readSize = std::min<uint32_t>(buffer.unalignedAfterBufferSize,
                                         stat->size - bytesRead);
      status = FSAReadFile(getActFsaHandle(), buffer.unalignedAfterBuffer,
                           1, readSize, fileHandle, FSAReadFlag::None);
      if (status < FSAStatus::OK || static_cast<uint32_t>(status) != readSize) {
         FSACloseFile(getActFsaHandle(), fileHandle);
         return ResultFileIoError;
      }

      bytesRead += readSize;
   }

   FSACloseFile(getActFsaHandle(), fileHandle);
   command.WriteResponse(bytesRead);
   return nn::ResultSuccess;
}

static nn::Result
getUuid(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetUuid> { args };
   auto slotNo = InvalidSlot;
   auto buffer = OutBuffer<Uuid> { };
   auto unkArg3 = int32_t { 0 };
   command.ReadRequest(slotNo, buffer, unkArg3);

   if (slotNo == SystemSlot) {
      auto uuid = Uuid { };
      uuid.fill('b');
      uuid[0] = 'd';
      uuid[1] = 'e';
      uuid[2] = 'c';
      uuid[3] = 'a';
      uuid[4] = 'f';
      buffer.writeOutput(uuid.data(), uuid.size());
      return nn::ResultSuccess;
   }

   auto account = getAccountBySlotNo(slotNo);
   if (!account) {
      return ResultACCOUNT_NOT_FOUND;
   }

   buffer.writeOutput(phys_addrof(account->uuid), account->uuid.size());
   return nn::ResultSuccess;
}

nn::Result
ActClientStandardService::commandHandler(uint32_t unk1,
                                         CommandId command,
                                         CommandHandlerArgs &args)
{
   switch (command) {
   case GetCommonInfo::command:
      return getCommonInfo(args);
   case GetAccountInfo::command:
      return getAccountInfo(args);
   case GetTransferableId::command:
      return getTransferableId(args);
   case GetMiiImage::command:
      return getMiiImage(args);
   case GetUuid::command:
      return getUuid(args);
   default:
      return nn::ResultSuccess;
   }
}

} // namespace ios::fpd::internal
