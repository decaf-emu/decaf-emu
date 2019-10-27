#include "ios_fpd_act_clientstandardservice.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <array>
#include <cstring>
#include <memory>
#include <string>

using namespace nn::ipc;
using namespace nn::act;
using namespace nn::ffl;

namespace ios::fpd::internal
{

static constexpr std::array<uint8_t, 8> DeviceHash = { 0x2C, 0x10, 0xC1, 0x67, 0xEB, 0xC6, 0x00, 0x00 };

struct Account
{
   bool used = false;

   be2_val<SlotNo> parentalControlSlot;
   be2_val<uint32_t> parentalId;
   be2_val<uint32_t> persistentId;
   be2_val<uint32_t> principalId;
   be2_val<uint32_t> simpleAddressId;
   be2_val<uint64_t> localFriendCode;
   be2_struct<FFLStoreData> mii;
   be2_val<uint32_t> birthday;
   be2_array<char, AccountIdSize> accountId;
   be2_val<uint8_t> gender;
   be2_array<char, NfsPasswordSize> nfsPassword;
   be2_val<uint8_t> isCommitted;
   be2_struct<Uuid> uuid;
};

struct AccountData
{
   be2_array<Account, NumSlots> accounts;
   be2_array<phys_ptr<Account>, NumSlots> accountSlots;
   be2_phys_ptr<Account> currentAccount;
   be2_phys_ptr<Account> defaultAccount;
   be2_val<LocalFriendCode> systemLocalFriendCode = 0x7379736C636C6672ull;
   be2_val<uint8_t> applicationUpdateRequired = uint8_t { 0 };
};

static phys_ptr<AccountData> sAccountData = nullptr;

static phys_ptr<Account>
allocateAccount()
{
   for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
      auto account = phys_addrof(sAccountData->accounts[i]);
      if (!account->used) {
         account->used = true;
         return account;
      }
   }

   return nullptr;
}

static SlotNo
addAccount(phys_ptr<Account> account)
{
   for (auto i = 0u; i < sAccountData->accountSlots.size(); ++i) {
      if (!sAccountData->accountSlots[i]) {
         sAccountData->accountSlots[i] = account;
         return i + 1;
      }
   }

   return InvalidSlot;
}

static uint8_t
getNumAccounts()
{
   auto count = uint8_t { 0 };
   for (auto account : sAccountData->accountSlots) {
      if (account) {
         count++;
      }
   }

   return count;
}

static SlotNo
getSlotNoForAccount(phys_ptr<Account> account)
{
   for (auto i = 0u; i < sAccountData->accountSlots.size(); ++i) {
      if (sAccountData->accountSlots[i] == account) {
         return i + 1;
      }
   }

   return InvalidSlot;
}

static phys_ptr<Account>
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

   return sAccountData->accountSlots[index];
}

static nn::Result
getCommonInfo(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetCommonInfo> { args };
   auto buffer = InOutBuffer<void> { };
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
      if (buffer.totalSize() < sizeof(sAccountData->systemLocalFriendCode)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(sAccountData->systemLocalFriendCode),
                         sizeof(sAccountData->systemLocalFriendCode));
      break;
   case InfoType::ApplicationUpdateRequired:
   {
      if (buffer.totalSize() < sizeof(sAccountData->applicationUpdateRequired)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(sAccountData->applicationUpdateRequired),
                         sizeof(sAccountData->applicationUpdateRequired));
      break;
   }
   case static_cast<InfoType>(36):
      return ResultNotImplemented;
   case static_cast<InfoType>(42):
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
   auto buffer = InOutBuffer<void> { };
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
      if (buffer.totalSize() < sizeof(account->localFriendCode)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->localFriendCode),
                         sizeof(account->localFriendCode));
      break;
   case InfoType::Mii:
      if (buffer.totalSize() < sizeof(account->mii)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->mii),
                         sizeof(account->mii));
      break;
   case InfoType::AccountId:
      if (buffer.totalSize() < sizeof(account->accountId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->accountId),
                         sizeof(account->accountId));
      break;
   case static_cast<InfoType>(9):
      return ResultNotImplemented;
   case InfoType::Birthday:
      if (buffer.totalSize() < sizeof(account->birthday)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->birthday),
                         sizeof(account->birthday));
      break;
   case static_cast<InfoType>(11):
      return ResultNotImplemented;
   case InfoType::PrincipalId:
      if (buffer.totalSize() < sizeof(account->principalId)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->principalId),
                         sizeof(account->principalId));
      break;
   case static_cast<InfoType>(14):
   case static_cast<InfoType>(15):
   case static_cast<InfoType>(17):
   case static_cast<InfoType>(18):
      return ResultNotImplemented;
   case InfoType::Gender:
      if (buffer.totalSize() < sizeof(account->gender)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->gender),
                         sizeof(account->gender));
      break;
   case static_cast<InfoType>(20):
   case static_cast<InfoType>(21):
      return ResultNotImplemented;
   case InfoType::ParentalControlSlot:
      if (buffer.totalSize() < sizeof(account->parentalControlSlot)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->parentalControlSlot),
                         sizeof(account->parentalControlSlot));
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
   case static_cast<InfoType>(27):
      return ResultNotImplemented;
   case InfoType::NfsPassword:
      if (buffer.totalSize() < sizeof(account->nfsPassword)) {
         return ResultInvalidSize;
      }

      buffer.writeOutput(phys_addrof(account->nfsPassword),
                         sizeof(account->nfsPassword));
      break;
   case static_cast<InfoType>(29):
   case static_cast<InfoType>(30):
   case static_cast<InfoType>(31):
   case static_cast<InfoType>(32):
   case static_cast<InfoType>(33):
   case static_cast<InfoType>(34):
   case static_cast<InfoType>(37):
   case static_cast<InfoType>(38):
   case static_cast<InfoType>(39):
   case static_cast<InfoType>(40):
   case static_cast<InfoType>(41):
   case static_cast<InfoType>(44):
      return ResultNotImplemented;
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

   auto localFriendCode = LocalFriendCode { 0 };
   if (slotNo == SystemSlot) {
      localFriendCode = sAccountData->systemLocalFriendCode;
   } else {
      auto account = getAccountBySlotNo(slotNo);
      if (!account) {
         return ResultACCOUNT_NOT_FOUND;
      }

      localFriendCode = account->localFriendCode;
   }

   auto transferableId =
      sub_E30BD4F0(static_cast<uint32_t>(localFriendCode >> 32),
                   static_cast<uint32_t>(localFriendCode & 0xFFFFFFFF),
                   static_cast<uint16_t>(unkArg2));

   command.WriteResponse(transferableId);
   return nn::ResultSuccess;
}

static nn::Result
getUuid(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActClientStandardService::GetUuid> { args };
   auto slotNo = InvalidSlot;
   auto buffer = InOutBuffer<Uuid> { nullptr };
   auto unkArg3 = int32_t { 0 };
   command.ReadRequest(slotNo, buffer, unkArg3);

   if (slotNo == SystemSlot) {
      auto uuid = Uuid { };
      uuid.fill('X');
      uuid[0] = 's';
      uuid[1] = 'y';
      uuid[2] = 's';
      buffer.writeOutput(uuid.data(), uuid.size());
      return nn::ResultSuccess;
   }

   auto account = getAccountBySlotNo(slotNo);
   if (!account) {
      return ResultACCOUNT_NOT_FOUND;
   }

   buffer.writeOutput(account->uuid.data(), account->uuid.size());
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

// This is taken from http://wiibrew.org/wiki/Mii_Data
static uint16_t
calculateMiiCRC(phys_ptr<const uint8_t> bytes,
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

void
initialiseStaticClientStandardServiceData()
{
   sAccountData = kernel::allocProcessStatic<AccountData>();
}

void
initialiseAccounts()
{
   auto account = allocateAccount();
   account->parentalId = 1u;
   account->persistentId = 1u;
   account->principalId = 1u;
   account->simpleAddressId = 1u;
   account->localFriendCode = 0x7573726C636C6672ull;
   account->accountId = "DecafAccountId";
   account->nfsPassword = "NfsPassword69";
   account->birthday = 1337u;
   account->gender = uint8_t { 1 };
   account->parentalControlSlot = InvalidSlot;
   account->uuid.fill('A');
   account->uuid[0] = 'u';
   account->uuid[1] = 's';
   account->uuid[2] = 'r';

   const uint8_t miiAuthorId[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
   const uint8_t miiId[] = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10 };
   const std::u16string miiCreatorsName = u"decafC";
   const std::u16string miiName = u"decafM";

   std::memset(std::addressof(account->mii), 0, sizeof(FFLStoreData));

   std::memcpy(account->mii.mii_name, miiName.c_str(), miiName.size() * 2 + 2);
   std::memcpy(account->mii.creator_name, miiCreatorsName.c_str(), miiCreatorsName.size() * 2 + 2);
   std::memcpy(account->mii.author_id, miiAuthorId, 8);
   std::memcpy(account->mii.mii_id, miiId, 10);

   account->mii.birth_platform = 3;
   account->mii.font_region = 0;
   account->mii.region_move = 0;
   account->mii.copyable = 0;
   account->mii.mii_version = 0x40;
   account->mii.color = 4;
   account->mii.birth_day = 10;
   account->mii.birth_month = 8;
   account->mii.gender = 0;
   account->mii.size = 109;
   account->mii.fatness = 65;
   account->mii.blush_type = 0;
   account->mii.face_style = 0;
   account->mii.face_color = 0;
   account->mii.face_type = 6;
   account->mii.local_only = 1;
   account->mii.hair_mirrored = 15;
   account->mii.hair_color = 1;
   account->mii.hair_type = 1;
   account->mii.eye_thickness = 6;
   account->mii.eye_scale = 0;
   account->mii.eye_color = 2;
   account->mii.eye_type = 26;
   account->mii.eye_height = 68;
   account->mii.eye_distance = 0;
   account->mii.eye_rotation = 3;
   account->mii.eyebrow_thickness = 12;
   account->mii.eyebrow_scale = 2;
   account->mii.eyebrow_color = 4;
   account->mii.eyebrow_type = 6;
   account->mii.eyebrow_height = 69;
   account->mii.eyebrow_distance = 8;
   account->mii.eyebrow_rotation = 2;
   account->mii.nose_height = 1;
   account->mii.nose_scale = 5;
   account->mii.nose_type = 2;
   account->mii.mouth_thickness = 6;
   account->mii.mouth_scale = 2;
   account->mii.mouth_color = 0;
   account->mii.mouth_type = 26;
   account->mii.unk_0x40 = 13;
   account->mii.mustache_type = 0;
   account->mii.mouth_height = 0;
   account->mii.mustache_height = 8;
   account->mii.mustache_scale = 4;
   account->mii.beard_color = 2;
   account->mii.beard_type = 1;
   account->mii.glass_height = 0;
   account->mii.glass_scale = 0;
   account->mii.glass_color = 1;
   account->mii.glass_type = 5;
   account->mii.mole_ypos = 4;
   account->mii.mole_xpos = 1;
   account->mii.mole_scale = 10;
   account->mii.mole_enabled = 0;
   account->mii.checksum = calculateMiiCRC(phys_cast<uint8_t *>(phys_addrof(account->mii)),
                                           sizeof(FFLStoreData) - 2);

   addAccount(account);
   sAccountData->currentAccount = account;
   sAccountData->defaultAccount = account;
}

} // namespace ios::fpd::internal
