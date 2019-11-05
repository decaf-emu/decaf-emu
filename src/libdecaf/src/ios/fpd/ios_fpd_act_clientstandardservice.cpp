#include "ios_fpd_act_accountmanager.h"
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

//! TODO: This hash is actually read from /dev/mcp ioctl 0xD3
static constexpr std::array<uint8_t, 4> UnknownHash = { 0xDE, 0xCA, 0xF0, 0x0D };

//! TODO: Device Hash is actually calculated from UnkownHash using some sort of sha256
static constexpr std::array<uint8_t, 8> DeviceHash = { 0xDE, 0xCA, 0xFD, 0xEC, 0xAF, 0xDE, 0xCA, 0xF0 };

struct AccountData
{
   be2_struct<UuidManager> uuidManager;
   be2_struct<TransferableIdManager> transferableIdManager;
   be2_struct<PersistentIdManager> persistentIdManager;
   be2_struct<AccountManager> accountManager;
   be2_array<AccountInstance, NumSlots> accounts;
   be2_array<bool, NumSlots> accountUsed;

   be2_phys_ptr<AccountInstance> currentAccount;
   be2_phys_ptr<AccountInstance> defaultAccount;
};

static phys_ptr<AccountData> sAccountData = nullptr;

static phys_ptr<AccountInstance>
allocateAccount()
{
   for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
      if (!sAccountData->accountUsed[i]) {
         sAccountData->accountUsed[i] = true;
         return phys_addrof(sAccountData->accounts[i]);
      }
   }

   return nullptr;
}

static uint8_t
getNumAccounts()
{
   auto count = uint8_t { 0 };
   for (auto used : sAccountData->accountUsed) {
      if (used) {
         count++;
      }
   }

   return count;
}

static SlotNo
getSlotNoForAccount(phys_ptr<AccountInstance> account)
{
   auto index = account - phys_addrof(sAccountData->accounts[0]);
   if (index < 0 || index >= sAccountData->accounts.size()) {
      return InvalidSlot;
   }

   return static_cast<SlotNo>(index + 1);
}

static phys_ptr<AccountInstance>
getAccountBySlotNo(SlotNo slot)
{
   if (slot == CurrentUserSlot) {
      return sAccountData->currentAccount;
   } else if (slot == InvalidSlot) {
      return nullptr;
   }

   auto index = static_cast<unsigned>(slot - 1);
   if (index >= sAccountData->accounts.size()) {
      return nullptr;
   }

   return phys_addrof(sAccountData->accounts[index]);
}

static phys_ptr<AccountInstance>
getAccountByPersistentId(PersistentId id)
{
   for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
      if (sAccountData->accountUsed[i] &&
          sAccountData->accounts[i].persistentId == id) {
         return phys_addrof(sAccountData->accounts[i]);
      }
   }

   return nullptr;
}

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

      SlotNo slotNo = getSlotNoForAccount(sAccountData->currentAccount);
      buffer.writeOutput(&slotNo, sizeof(slotNo));
      break;
   }
   case InfoType::DefaultAccount:
   {
      if (buffer.totalSize() < sizeof(SlotNo)) {
         return ResultInvalidSize;
      }

      SlotNo slotNo = getSlotNoForAccount(sAccountData->defaultAccount);
      buffer.writeOutput(&slotNo, sizeof(slotNo));
      break;
   }
   case InfoType::NetworkTimeDifference:
      return ResultNotImplemented;
   case InfoType::LocalFriendCode:
      if (buffer.totalSize() < sizeof(sAccountData->accountManager.commonTransferableIdBase)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(sAccountData->accountManager.commonTransferableIdBase),
                         sizeof(sAccountData->accountManager.commonTransferableIdBase));
      break;
   case InfoType::ApplicationUpdateRequired:
   {
      if (buffer.totalSize() < sizeof(sAccountData->accountManager.isApplicationUpdateRequired)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(sAccountData->accountManager.isApplicationUpdateRequired),
                         sizeof(sAccountData->accountManager.isApplicationUpdateRequired));
      break;
   }
   case InfoType::DefaultHostServerSettings:
      return ResultNotImplemented;
   case InfoType::DefaultHostServerSettingsEx:
      return ResultNotImplemented;
   case InfoType::DeviceHash:
      if (buffer.totalSize() < DeviceHash.size()) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(DeviceHash.data(), DeviceHash.size());
      break;
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
      StackObject<uint32_t> birthday;
      if (buffer.totalSize() < sizeof(*birthday)) {
         return ResultInvalidSize;
      }

      *birthday = static_cast<uint32_t>(
         account->birthYear |
         (account->birthMonth << 16) |
         (account->birthDay << 24));
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
   case static_cast<InfoType>(34):
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
   case static_cast<InfoType>(40):
   case static_cast<InfoType>(41):
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

static uint64_t
sub_E30BD3A0(uint32_t a, uint32_t b)
{
   uint32_t v2 = (b << 8) & 0xFF0000 | (b << 24) | (b >> 8) & 0xFF00 | (b >> 24);
   uint32_t v3 = (a << 8) & 0xFF0000 | (a << 24) | (a >> 8) & 0xFF00 | (a >> 24);
   uint32_t v4 = v2 ^ (v2 ^ (v2 >> 7)) & 0xAA00AA ^ (((v2 ^ (v2 >> 7)) & 0xAA00AA) << 7);
   uint32_t v5 = v3 ^ (v3 ^ (v3 >> 7)) & 0xAA00AA ^ (((v3 ^ (v3 >> 7)) & 0xAA00AA) << 7);
   uint32_t v6 = v5 ^ (v5 ^ (v5 >> 14)) & 0xCCCC ^ (((v5 ^ (v5 >> 14)) & 0xCCCC) << 14);
   uint32_t v7 = v4 ^ (v4 ^ (v4 >> 14)) & 0xCCCC ^ (((v4 ^ (v4 >> 14)) & 0xCCCC) << 14);

   uint32_t resultLo =
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) << 8) & 0xFF0000 |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) << 24) |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) >> 8) & 0xFF00 |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) >> 24);
   uint32_t resultHi =
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) << 8) & 0xFF0000 |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) << 24) |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) >> 8) & 0xFF00 |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) >> 24);

   return static_cast<uint64_t>(resultLo) | (static_cast<uint64_t>(resultHi) << 32);
}

static uint64_t
sub_E30BD4F0(uint32_t a1, uint32_t a2, uint16_t a3)
{
   uint32_t v3 = (a3 << 6) | a1;
   uint32_t v4 = (v3 >> 16) & 0x1C0 | 8 * (uint8_t)a2 & 0x38 | (v3 >> 6) & 7;
   return sub_E30BD3A0(
      v3 ^ ((v4 << 31) | (v4 << 22) | (v4 >> 5) | (v4 >> 23) | (v4 >> 14) | 16 * v4 | (v4 << 13)) & 0xFFFFFE3F,
      a2 ^ ((v4 << 27) | (v4 << 9) | (v4 << 18) | v4));
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
      transferableIdBase = sAccountData->accountManager.commonTransferableIdBase;
   } else {
      auto account = getAccountBySlotNo(slotNo);
      if (!account) {
         return ResultACCOUNT_NOT_FOUND;
      }

      transferableIdBase = account->transferableIdBase;
   }

   auto transferableId =
      sub_E30BD4F0(static_cast<uint32_t>(transferableIdBase >> 32),
                   static_cast<uint32_t>(transferableIdBase & 0xFFFFFFFF),
                   static_cast<uint16_t>(unkArg2));

   command.WriteResponse(transferableId);
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
   case GetUuid::command:
      return getUuid(args);
   default:
      return nn::ResultSuccess;
   }
}

static uint16_t
FFLiGetCRC16(phys_ptr<const uint8_t> bytes,
             uint32_t length)
{
   auto crc = uint32_t { 0 };

   for (auto byteIndex = 0u; byteIndex < length; byteIndex++) {
      for (auto bitIndex = 7; bitIndex >= 0; bitIndex--) {
         crc = (((crc << 1) | ((bytes[byteIndex] >> bitIndex) & 0x1)) ^
               (((crc & 0x8000) != 0) ? 0x1021 : 0));
      }
   }

   for (auto counter = 16; counter > 0u; counter--) {
      crc = ((crc << 1) ^ (((crc & 0x8000) != 0) ? 0x1021 : 0));
   }

   return static_cast<uint16_t>(crc & 0xFFFF);
}

static void
FFLiSetAuthorID(uint64_t *authorId)
{
   *authorId = sub_E30BD4F0(
      static_cast<uint32_t>(sAccountData->accountManager.commonTransferableIdBase >> 32),
      static_cast<uint32_t>(sAccountData->accountManager.commonTransferableIdBase),
      0x4A0);
}

static void
FFLiSetCreateID(FFLCreateID *createId)
{
   static constexpr tm MiiEpoch = { 0, 0, 0, 1, 0, 2010 - 1900, 0, 0, 0 };
   auto epoch = std::chrono::system_clock::from_time_t(platform::make_gm_time(MiiEpoch));
   auto secondsSinceMiiEpoch =
      std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now() - epoch);

   createId->flags = FFLCreateIDFlags::IsNormalMii | FFLCreateIDFlags::IsWiiUMii;
   createId->timestamp = static_cast<uint32_t>(secondsSinceMiiEpoch.count() & 0x0FFFFFFF);
   std::memcpy(createId->deviceHash, DeviceHash.data(), 6);
}

static long long
getUuidTime()
{
   auto start = std::chrono::system_clock::from_time_t(-12219292800);
   auto diff = std::chrono::system_clock::now() - start;
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
   return ns / 100;
}

static std::array<uint8_t, UuidSize>
generateUuid()
{
   auto time = getUuidTime();
   sAccountData->uuidManager.lastTime = time;

   auto time_low = static_cast<uint32_t>(time);
   auto time_mid = static_cast<uint16_t>((time >> 32) & 0xFFFF);
   auto time_hi_and_version = static_cast<uint16_t>(
      static_cast<uint16_t>((time >> 48) & 0x0FFF) | 0x1000);

   auto clock_seq = static_cast<uint16_t>(
      (++sAccountData->uuidManager.clockSequence & 0x3FFF) | 0x8000);
   if (sAccountData->uuidManager.clockSequence >= 0x4000) {
      sAccountData->uuidManager.clockSequence = 0;
   }

   auto node = std::array<uint8_t, 6> { };
   node.fill(0);
   node[0] = 1;
   node[1] = 1;
   std::memcpy(node.data() + 2, UnknownHash.data(), UnknownHash.size());

   auto uuid = std::array<uint8_t, UuidSize> { };
   std::memcpy(uuid.data() + 0, &time_low, 4);
   std::memcpy(uuid.data() + 4, &time_mid, 2);
   std::memcpy(uuid.data() + 6, &time_hi_and_version, 2);
   std::memcpy(uuid.data() + 8, &clock_seq, 2);
   std::memcpy(uuid.data() + 10, node.data(), 6);
   return uuid;
}

static PersistentId
generatePersistentId()
{
   return ++sAccountData->persistentIdManager.persistentIdHead;
}

static TransferrableId
generateTransferrableIdBase()
{
   auto valueLo = *reinterpret_cast<const uint32_t *>(UnknownHash.data());
   auto valueHi = (sAccountData->transferableIdManager.counter << 22) | 4;
   sAccountData->transferableIdManager.counter =
      static_cast<uint32_t>((sAccountData->transferableIdManager.counter + 1) & 0x3FF);

   return static_cast<uint64_t>(valueLo) | (static_cast<uint64_t>(valueHi) << 32);
}

static phys_ptr<AccountInstance>
createAccount()
{
   auto account = allocateAccount();
   account->persistentId = generatePersistentId();
   account->parentalControlSlotNo = uint8_t { 1u };
   account->principalId = 1u;
   account->simpleAddressId = 1u;
   account->transferableIdBase = generateTransferrableIdBase();
   account->accountId = "DonaldTrump420";
   account->nfsPassword = "NfsPassword";
   account->birthDay = uint8_t { 4 };
   account->birthMonth = uint8_t { 6 };
   account->birthYear = uint16_t { 1989 };
   account->gender = uint8_t { 1 };
   account->uuid = generateUuid();

   // Default Mii from IOS
   static uint8_t DefaultMii[] = {
      0x00, 0x01, 0x00, 0x40, 0x80, 0xF3, 0x41, 0x80, 0x02, 0x65, 0xA0, 0x92,
      0xD2, 0x3B, 0x13, 0x36, 0xA4, 0xC0, 0xE1, 0xF8, 0x2D, 0x06, 0x00, 0x00,
      0x00, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40,
      0x00, 0x00, 0x21, 0x01, 0x02, 0x68, 0x44, 0x18, 0x26, 0x34, 0x46, 0x14,
      0x81, 0x12, 0x17, 0x68, 0x0D, 0x00, 0x00, 0x29, 0x00, 0x52, 0x48, 0x50,
      0x00, 0x00, 0x61, 0x00, 0x72, 0x00, 0x61, 0x00, 0x68, 0x00, 0x00, 0x00,
      0x67, 0x00, 0x68, 0x00, 0x69, 0x00, 0x6A, 0x00, 0x00, 0x00, 0xE4, 0x62,
   };

   static_assert(sizeof(DefaultMii) == sizeof(account->miiData));
   std::memcpy(std::addressof(account->miiData), DefaultMii, sizeof(DefaultMii));
   FFLiSetAuthorID(&account->miiData.author_id);
   FFLiSetCreateID(&account->miiData.mii_id);
   account->miiData.checksum = FFLiGetCRC16(phys_cast<uint8_t *>(phys_addrof(account->miiData)),
      sizeof(FFLStoreData) - 2);
   account->miiName = u"???";

   return account;
}

void
loadAccounts()
{
   loadTransferableIdManager(phys_addrof(sAccountData->transferableIdManager));
   loadUuidManager(phys_addrof(sAccountData->uuidManager));
   loadPersistentIdManager(phys_addrof(sAccountData->persistentIdManager));

   loadAccountManager(phys_addrof(sAccountData->accountManager));
   if (!sAccountData->accountManager.commonUuid[0]) {
      sAccountData->accountManager.commonUuid = generateUuid();
   }

   if (!sAccountData->accountManager.commonTransferableIdBase) {
      sAccountData->accountManager.commonTransferableIdBase = generateTransferrableIdBase();
   }

   for (auto slot = 1; slot <= sAccountData->accountManager.persistentIdList.size(); ++slot) {
      auto persistentId = sAccountData->accountManager.persistentIdList[slot - 1];
      if (persistentId) {
         sAccountData->accountUsed[slot - 1] =
            loadAccountInstance(phys_addrof(sAccountData->accounts[slot - 1]),
                                persistentId);
      }
   }

   // Try find the default account
   auto account = getAccountByPersistentId(sAccountData->accountManager.defaultAccountPersistentId);
   if (!account) {
      // Try find any account
      for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
         if (sAccountData->accountUsed[i]) {
            account = phys_addrof(sAccountData->accounts[i]);
            break;
         }
      }

      if (!account) {
         // No account found, we can create one instead!
         account = createAccount();
         sAccountData->accountManager.persistentIdList[getSlotNoForAccount(account)] = account->persistentId;
         sAccountData->accountManager.defaultAccountPersistentId = account->persistentId;
      }
   }

   sAccountData->currentAccount = account;
   sAccountData->defaultAccount = account;
}

void
initialiseStaticClientStandardServiceData()
{
   sAccountData = kernel::allocProcessStatic<AccountData>();
}

} // namespace ios::fpd::internal
