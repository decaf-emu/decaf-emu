#include "nn_act.h"
#include "nn_act_core.h"
#include "nn_act_result.h"
#include <algorithm>

static const uint8_t
DeviceHash[] = { 0x2C, 0x10, 0xC1, 0x67, 0xEB, 0xC6 };

static const uint8_t
SystemId[] = { 0xBA, 0xAD, 0xF0, 0x0D, 0xDE, 0xAD, 0xBA, 0xBE };

struct Account
{
   uint8_t slot;
   uint32_t parentalId;
   uint32_t persistentId;
   uint32_t principalId;
   uint32_t transferrableId;
   uint32_t simpleAddressId;
   uint32_t transferableId;
};

static Account
sUserAccount = { 1, 1, 0x80000001u, 1, 1, 1, 1 };

static Account
sSystemAccount = { nn::act::SystemSlot, 0, 0, 0, 0, 0, 0 };

namespace nn
{

namespace act
{

nn::Result
Initialize()
{
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   return nn::Result::Success;
}

uint8_t
GetNumOfAccounts()
{
   return 1;
}

bool
IsSlotOccupied(uint8_t slot)
{
   if (slot == SystemSlot || slot == CurrentUserSlot || slot == sUserAccount.slot) {
      return true;
   } else {
      return false;
   }
}

nn::Result
Cancel()
{
   return nn::Result::Success;
}

uint8_t
GetSlotNo()
{
   return sUserAccount.slot;
}

nn::Result
GetUuid(UUID *uuid)
{
   return GetUuidEx(uuid, GetSlotNo());
}

nn::Result
GetUuidEx(UUID *uuid,
          uint8_t slot)
{
   // System account
   if (slot == SystemSlot) {
      uuid->fill('X');
      uuid->at(0) = 's';
      uuid->at(1) = 'y';
      uuid->at(2) = 's';
      return nn::Result::Success;
   }

   // User account
   if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      uuid->fill('A' + slot);
      uuid->at(0) = 'u';
      uuid->at(1) = 's';
      uuid->at(2) = 'r';
      return nn::Result::Success;
   }

   return nn::act::AccountNotFound;
}

nn::Result
GetAccountId(char *accountId)
{
   return GetAccountIdEx(accountId, GetSlotNo());
}

nn::Result
GetAccountIdEx(char *accountId,
               uint8_t slot)
{
   if (slot != SystemSlot && slot != CurrentUserSlot && slot != sUserAccount.slot) {
      return nn::act::AccountNotFound;
   }

   *accountId = '\0';
   return nn::Result::Success;
}

uint8_t
GetParentalControlSlotNo()
{
   uint8_t parentSlot = 0;
   GetParentalControlSlotNoEx(&parentSlot, GetSlotNo());
   return parentSlot;
}

nn::Result
GetParentalControlSlotNoEx(uint8_t *parentSlot, uint8_t slot)
{
   if (slot == SystemSlot) {
      *parentSlot = SystemSlot;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *parentSlot = sUserAccount.slot;
   } else {
      return nn::act::AccountNotFound;
   }

   return nn::Result::Success;
}

uint32_t
GetPersistentId()
{
   return GetPersistentIdEx(CurrentUserSlot);
}

uint32_t
GetPersistentIdEx(uint8_t slot)
{
   if (slot == SystemSlot) {
      return sSystemAccount.persistentId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      return sUserAccount.persistentId;
   } else {
      return 0;
   }
}

uint32_t
GetPrincipalId()
{
   be_val<uint32_t> id;
   GetPrincipalIdEx(&id, CurrentUserSlot);
   return id;
}

nn::Result
GetPrincipalIdEx(be_val<uint32_t> *principalId,
                 uint8_t slot)
{
   if (slot == SystemSlot) {
      *principalId = sSystemAccount.principalId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *principalId = sUserAccount.principalId;
   } else {
      return nn::act::AccountNotFound;
   }

   return nn::Result::Success;
}

uint32_t
GetSimpleAddressId()
{
   be_val<uint32_t> id;
   GetSimpleAddressIdEx(&id, CurrentUserSlot);
   return id;
}

nn::Result
GetSimpleAddressIdEx(be_val<uint32_t> *simpleAddressId,
                     uint8_t slot)
{
   if (slot == SystemSlot) {
      *simpleAddressId = sSystemAccount.simpleAddressId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *simpleAddressId = sUserAccount.simpleAddressId;
   } else {
      return nn::act::AccountNotFound;
   }

   return nn::Result::Success;
}

uint64_t
GetTransferableId(uint32_t unk1)
{
   be_val<uint64_t> id;
   GetTransferableIdEx(&id, unk1, CurrentUserSlot);
   return id;
}

nn::Result
GetTransferableIdEx(be_val<uint64_t> *transferableId,
                    uint32_t unk1,
                    uint8_t slot)
{
   if (slot == SystemSlot) {
      *transferableId = sSystemAccount.transferableId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *transferableId = sUserAccount.transferableId;
   } else {
      return nn::act::AccountNotFound;
   }

   return nn::Result::Success;
}

nn::Result
GetMii(FFLStoreData *data)
{
   return GetMiiEx(data, CurrentUserSlot);
}

// This is taken from http://wiibrew.org/wiki/Mii_Data
static uint16_t
calculateMiiCRC(const uint8_t *bytes, uint32_t length)
{
   uint32_t crc = 0x0000;

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

nn::Result
GetMiiEx(FFLStoreData *data,
         uint8_t slot)
{
   if (slot != SystemSlot && slot != CurrentUserSlot && slot != sUserAccount.slot) {
      return nn::act::AccountNotFound;
   }

   // Set our Mii Data!
   std::memset(data, 0, sizeof(FFLStoreData));
   std::copy(std::begin(DeviceHash), std::end(DeviceHash), std::begin(data->deviceHash));
   std::copy(std::begin(SystemId), std::end(SystemId), std::begin(data->systemId));
   data->miiId = 0x10000000;

   // Unknown values, but game will assert if not set
   data->importantUnk1 = 0x6D60E291;
   data->importantUnk2 = 0x16271329;

   // Mii Name
   data->name[0] = 'd';
   data->name[1] = 'e';
   data->name[2] = 'c';
   data->name[3] = 'a';
   data->name[4] = 'f';
   data->name[5] = 'M';

   // Creator's Name
   data->creator[0] = 'd';
   data->creator[1] = 'e';
   data->creator[2] = 'c';
   data->creator[3] = 'a';
   data->creator[4] = 'f';
   data->creator[5] = 'C';

   data->crc = calculateMiiCRC(reinterpret_cast<uint8_t *>(data), sizeof(FFLStoreData) - 2);
   return nn::Result::Success;
}

nn::Result
GetMiiName(be_val<char16_t> *name)
{
   return GetMiiNameEx(name, CurrentUserSlot);
}

nn::Result
GetMiiNameEx(be_val<char16_t> *name,
             uint8_t slot)
{
   if (slot != SystemSlot && slot != CurrentUserSlot && slot != sUserAccount.slot) {
      return nn::act::AccountNotFound;
   }

   auto miiName = u"decafM";
   size_t i = 0;
   do {
      name[i] = miiName[i];
   } while (miiName[i++] != 0);
   return nn::Result::Success;
}

bool
IsParentalControlCheckEnabled()
{
   return false;
}

bool
IsNetworkAccount()
{
   return false;
}

bool
IsNetworkAccountEx(uint8_t slot)
{
   return false;
}

nn::Result
GetDeviceHash(uint8_t *data)
{
   memcpy(data, DeviceHash, 6);
   return nn::Result::Success;
}

nn::Result
IsCommittedEx(uint8_t slot)
{
   return nn::Result::Success;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3actFv", nn::act::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3actFv", nn::act::Finalize);
   RegisterKernelFunctionName("Cancel__Q2_2nn3actFv", nn::act::Cancel);
   RegisterKernelFunctionName("IsSlotOccupied__Q2_2nn3actFUc", nn::act::IsSlotOccupied);
   RegisterKernelFunctionName("GetSlotNo__Q2_2nn3actFv", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetMii__Q2_2nn3actFP12FFLStoreData", nn::act::GetMii);
   RegisterKernelFunctionName("GetMiiEx__Q2_2nn3actFP12FFLStoreDataUc", nn::act::GetMiiEx);
   RegisterKernelFunctionName("GetMiiName__Q2_2nn3actFPw", nn::act::GetMiiName);
   RegisterKernelFunctionName("GetMiiNameEx__Q2_2nn3actFPwUc", nn::act::GetMiiNameEx);
   RegisterKernelFunctionName("IsParentalControlCheckEnabled__Q2_2nn3actFv", nn::act::IsParentalControlCheckEnabled);
   RegisterKernelFunctionName("IsNetworkAccount__Q2_2nn3actFv", nn::act::IsNetworkAccount);
   RegisterKernelFunctionName("IsNetworkAccountEx__Q2_2nn3actFUc", nn::act::IsNetworkAccountEx);
   RegisterKernelFunctionName("GetNumOfAccounts__Q2_2nn3actFv", nn::act::GetNumOfAccounts);
   RegisterKernelFunctionName("GetUuid__Q2_2nn3actFP7ACTUuid", nn::act::GetUuid);
   RegisterKernelFunctionName("GetUuidEx__Q2_2nn3actFP7ACTUuidUc", nn::act::GetUuidEx);
   RegisterKernelFunctionName("GetAccountId__Q2_2nn3actFPc", nn::act::GetAccountId);
   RegisterKernelFunctionName("GetAccountIdEx__Q2_2nn3actFPcUc", nn::act::GetAccountIdEx);
   RegisterKernelFunctionName("GetParentalControlSlotNo__Q2_2nn3actFv", nn::act::GetParentalControlSlotNo);
   RegisterKernelFunctionName("GetParentalControlSlotNoEx__Q2_2nn3actFPUcUc", nn::act::GetParentalControlSlotNoEx);
   RegisterKernelFunctionName("GetPersistentId__Q2_2nn3actFv", nn::act::GetPersistentId);
   RegisterKernelFunctionName("GetPersistentIdEx__Q2_2nn3actFUc", nn::act::GetPersistentIdEx);
   RegisterKernelFunctionName("GetPrincipalId__Q2_2nn3actFv", nn::act::GetPrincipalId);
   RegisterKernelFunctionName("GetPrincipalIdEx__Q2_2nn3actFPUiUc", nn::act::GetPrincipalIdEx);
   RegisterKernelFunctionName("GetSimpleAddressId__Q2_2nn3actFv", nn::act::GetSimpleAddressId);
   RegisterKernelFunctionName("GetSimpleAddressIdEx__Q2_2nn3actFPUiUc", nn::act::GetSimpleAddressIdEx);
   RegisterKernelFunctionName("GetTransferableId__Q2_2nn3actFUi", nn::act::GetTransferableId);
   RegisterKernelFunctionName("GetTransferableIdEx__Q2_2nn3actFPULUiUc", nn::act::GetTransferableIdEx);
   RegisterKernelFunctionName("GetDeviceHash__Q2_2nn3actFPUL", nn::act::GetDeviceHash);
   RegisterKernelFunctionName("IsCommittedEx__Q2_2nn3actFUc", nn::act::IsCommittedEx);
}

} // namespace act

} // namespace nn
