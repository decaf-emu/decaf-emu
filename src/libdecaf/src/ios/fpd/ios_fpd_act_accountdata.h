#pragma once
#include "nn/act/nn_act_types.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace ios::fpd::internal
{

using nn::act::SlotNo;
using nn::act::LocalFriendCode;
using nn::act::PersistentId;
using nn::act::PrincipalId;
using nn::act::SimpleAddressId;
using nn::act::TransferrableId;
using nn::act::Uuid;

using nn::act::NumSlots;
using nn::act::AccountIdSize;
using nn::act::NfsPasswordSize;
using nn::act::MiiNameSize;
using nn::act::UuidSize;

struct TransferableIdManager
{
   be2_val<uint32_t> counter;
};

struct UuidManager
{
   be2_val<int32_t> clockSequence;
   be2_val<int64_t> lastTime;
};

struct PersistentIdManager
{
   be2_val<uint32_t> persistentIdHead;
};

struct AccountManager
{
   be2_array<PersistentId, NumSlots> persistentIdList;
   be2_val<PersistentId> defaultAccountPersistentId;
   be2_val<TransferrableId> commonTransferableIdBase;
   be2_array<uint8_t, UuidSize> commonUuid;
   be2_val<uint8_t> isApplicationUpdateRequired;
   be2_val<uint32_t> defaultNnasType;
   be2_val<uint32_t> defaultNfsType;
   be2_val<uint8_t> defaultNfsNo;
   be2_array<char, 33> defaultNnasSubDomain;
   be2_array<char, 3> defaultNnasNfsEnv;
};

struct AccountInstance
{
   be2_val<PersistentId> persistentId;
   be2_val<TransferrableId> transferableIdBase;
   be2_array<uint8_t, UuidSize> uuid;
   be2_val<SlotNo> parentalControlSlotNo;
   be2_struct<nn::ffl::FFLStoreData> miiData;
   be2_array<char16_t, MiiNameSize> miiName;
   be2_val<uint8_t> isMiiUpdated;
   be2_array<char, AccountIdSize> accountId;
   be2_val<uint16_t> birthYear;
   be2_val<uint8_t> birthMonth;
   be2_val<uint8_t> birthDay;
   be2_val<uint32_t> gender;
   be2_val<uint8_t> isMailAddressValidated;
   be2_array<char, 257> emailAddress;
   be2_val<uint32_t> country;
   be2_val<SimpleAddressId> simpleAddressId;
   be2_array<char, 65> timeZoneId;
   be2_val<uint64_t> utcOffset; // TODO: Seconds? Nanoseconds? etc
   be2_val<PrincipalId> principalId;
   be2_array<char, 17> nfsPassword;
   be2_array<char, 32> eciVirtualAccount;
   be2_val<uint8_t> needsToDownloadMiiImage;
   be2_array<char, 257> miiImageUrl;
   be2_array<uint8_t, 32> accountPasswordHash;
   be2_val<uint8_t> isPasswordCacheEnabled;
   be2_array<uint8_t, 32> accountPasswordCache;
   be2_val<uint32_t> nnasType;
   be2_val<uint32_t> nfsType;
   be2_val<uint8_t> nfsNo;
   be2_array<char, 33> nnasSubDomain;
   be2_array<char, 3> nnasNfsEnv;
   be2_val<uint8_t> isPersistentIdUploaded;
   be2_val<uint8_t> isConsoleAccountInfoUploaded;
   be2_val<uint32_t> lastAuthenticationResult;
   be2_array<char, AccountIdSize> stickyAccountId;
   be2_array<char, AccountIdSize> nextAccountId;
   be2_val<PrincipalId> stickyPrincipalId;
   be2_val<uint8_t> isServerAccountDeleted;
   be2_val<uint32_t> serverAccountStatus;
   be2_array<char, 31> miiImageLastModifiedDate;
   be2_val<uint8_t> isCommitted;
};

phys_ptr<TransferableIdManager>
getTransferableIdManager();

phys_ptr<UuidManager>
getUuidManager();

phys_ptr<PersistentIdManager>
getPersistentIdManager();

phys_ptr<AccountManager>
getAccountManager();

phys_ptr<AccountInstance>
getCurrentAccount();

phys_ptr<AccountInstance>
getDefaultAccount();

std::array<uint8_t, 8>
getDeviceHash();

uint8_t
getNumAccounts();

SlotNo
getSlotNoForAccount(phys_ptr<AccountInstance> account);

phys_ptr<AccountInstance>
getAccountBySlotNo(SlotNo slot);

phys_ptr<AccountInstance>
getAccountByPersistentId(PersistentId id);

TransferrableId
calculateTransferableId(uint64_t transferableIdBase, uint16_t a3);

phys_ptr<AccountInstance>
createAccount();

void
initialiseAccounts();

void
initialiseStaticAccountData();

} // namespace ios::fpd::internal
