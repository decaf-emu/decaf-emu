#include "nn_act.h"
#include "nn_act_lib.h"

#include "cafe/cafe_stackobject.h"
#include "nn/act/nn_act_result.h"

#include <algorithm>

using namespace nn::act;
using nn::ffl::FFLStoreData;

namespace cafe::nn_act
{

static const uint8_t DeviceHash[] = { 0x2C, 0x10, 0xC1, 0x67, 0xEB, 0xC6 };
static const uint8_t SystemId[] = { 0xBA, 0xAD, 0xF0, 0x0D, 0xDE, 0xAD, 0xBA, 0xBE };
static uint8_t MiiAuthorId[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
static uint8_t MiiId[] = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10 };
static std::u16string MiiCreatorsName = u"decafC";
static std::u16string MiiName = u"decafM";

struct Account
{
   SlotNo slot;
   uint32_t parentalId;
   uint32_t persistentId;
   uint32_t principalId;
   uint32_t transferrableId;
   uint32_t simpleAddressId;
   uint32_t transferableId;
};

static Account sUserAccount = { 1, 1, 0x80000001u, 1, 1, 1, 1 };
static Account sSystemAccount = { SystemSlot, 0, 0, 0, 0, 0, 0 };

nn::Result
Initialize()
{
   // TODO: This whole library is supposed to just be IOS calls to /dev/act
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   return nn::ResultSuccess;
}

uint8_t
GetNumOfAccounts()
{
   return 1;
}

bool
IsSlotOccupied(SlotNo slot)
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
   return nn::ResultSuccess;
}

SlotNo
GetSlotNo()
{
   return sUserAccount.slot;
}

nn::Result
GetUuid(virt_ptr<UUID> uuid)
{
   return GetUuidEx(uuid, GetSlotNo());
}

nn::Result
GetUuidEx(virt_ptr<UUID> uuid,
          SlotNo slot)
{
   // System account
   if (slot == SystemSlot) {
      uuid->fill('X');
      uuid->at(0) = 's';
      uuid->at(1) = 'y';
      uuid->at(2) = 's';
      return nn::ResultSuccess;
   }

   // User account
   if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      uuid->fill('A' + slot);
      uuid->at(0) = 'u';
      uuid->at(1) = 's';
      uuid->at(2) = 'r';
      return nn::ResultSuccess;
   }

   return ResultAccountNotFound;
}

nn::Result
GetAccountId(virt_ptr<char> accountId)
{
   return GetAccountIdEx(accountId, GetSlotNo());
}

nn::Result
GetAccountIdEx(virt_ptr<char> accountId,
               SlotNo slot)
{
   if (slot != SystemSlot && slot != CurrentUserSlot && slot != sUserAccount.slot) {
      return ResultAccountNotFound;
   }

   *accountId = '\0';
   return nn::ResultSuccess;
}

uint8_t
GetParentalControlSlotNo()
{
   auto parentSlot = StackObject<uint8_t> { };
   GetParentalControlSlotNoEx(parentSlot, GetSlotNo());
   return *parentSlot;
}

nn::Result
GetParentalControlSlotNoEx(virt_ptr<SlotNo> parentSlot,
                           SlotNo slot)
{
   if (slot == SystemSlot) {
      *parentSlot = SystemSlot;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *parentSlot = sUserAccount.slot;
   } else {
      return ResultAccountNotFound;
   }

   return nn::ResultSuccess;
}

uint32_t
GetPersistentId()
{
   return GetPersistentIdEx(CurrentUserSlot);
}

uint32_t
GetPersistentIdEx(SlotNo slot)
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
   auto id = StackObject<uint32_t> { };
   GetPrincipalIdEx(id, CurrentUserSlot);
   return *id;
}

nn::Result
GetPrincipalIdEx(virt_ptr<uint32_t> principalId,
                 SlotNo slot)
{
   if (slot == SystemSlot) {
      *principalId = sSystemAccount.principalId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *principalId = sUserAccount.principalId;
   } else {
      return ResultAccountNotFound;
   }

   return nn::ResultSuccess;
}

uint32_t
GetSimpleAddressId()
{
   auto id = StackObject<uint32_t> { };
   GetSimpleAddressIdEx(id, CurrentUserSlot);
   return *id;
}

nn::Result
GetSimpleAddressIdEx(virt_ptr<uint32_t> simpleAddressId,
                     SlotNo slot)
{
   if (slot == SystemSlot) {
      *simpleAddressId = sSystemAccount.simpleAddressId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *simpleAddressId = sUserAccount.simpleAddressId;
   } else {
      return ResultAccountNotFound;
   }

   return nn::ResultSuccess;
}

uint64_t
GetTransferableId(uint32_t unk1)
{
   auto id = StackObject<uint64_t> { };
   GetTransferableIdEx(id, unk1, CurrentUserSlot);
   return *id;
}

nn::Result
GetTransferableIdEx(virt_ptr<uint64_t> transferableId,
                    uint32_t unk1,
                    SlotNo slot)
{
   if (slot == SystemSlot) {
      *transferableId = sSystemAccount.transferableId;
   } else if (slot == CurrentUserSlot || slot == sUserAccount.slot) {
      *transferableId = sUserAccount.transferableId;
   } else {
      return ResultAccountNotFound;
   }

   return nn::ResultSuccess;
}

nn::Result
GetMii(virt_ptr<FFLStoreData> data)
{
   return GetMiiEx(data, CurrentUserSlot);
}

// This is taken from http://wiibrew.org/wiki/Mii_Data
static uint16_t
calculateMiiCRC(virt_ptr<const uint8_t> bytes,
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

nn::Result
GetMiiEx(virt_ptr<FFLStoreData> data,
         SlotNo slot)
{
   if (slot != SystemSlot &&
       slot != CurrentUserSlot &&
       slot != sUserAccount.slot) {
      return ResultAccountNotFound;
   }

   // Set our Mii Data!
   std::memset(data.get(), 0, sizeof(FFLStoreData));

   data->birth_platform = 3;
   data->font_region = 0;
   data->region_move = 0;
   data->copyable = 0;
   data->mii_version = 0x40;
   std::memcpy(data->author_id, MiiAuthorId, 8);
   std::memcpy(data->mii_id, MiiId, 10);
   data->color = 4;
   data->birth_day = 10;
   data->birth_month = 8;
   data->gender = 0;
   std::memcpy(data->mii_name, MiiName.c_str(), MiiName.size() * 2 + 2);
   data->size = 109;
   data->fatness = 65;
   data->blush_type = 0;
   data->face_style = 0;
   data->face_color = 0;
   data->face_type = 6;
   data->local_only = 1;
   data->hair_mirrored = 15;
   data->hair_color = 1;
   data->hair_type = 1;
   data->eye_thickness = 6;
   data->eye_scale = 0;
   data->eye_color = 2;
   data->eye_type = 26;
   data->eye_height = 68;
   data->eye_distance = 0;
   data->eye_rotation = 3;
   data->eyebrow_thickness = 12;
   data->eyebrow_scale = 2;
   data->eyebrow_color = 4;
   data->eyebrow_type = 6;
   data->eyebrow_height = 69;
   data->eyebrow_distance = 8;
   data->eyebrow_rotation = 2;
   data->nose_height = 1;
   data->nose_scale = 5;
   data->nose_type = 2;
   data->mouth_thickness = 6;
   data->mouth_scale = 2;
   data->mouth_color = 0;
   data->mouth_type = 26;
   data->unk_0x40 = 13;
   data->mustache_type = 0;
   data->mouth_height = 0;
   data->mustache_height = 8;
   data->mustache_scale = 4;
   data->beard_color = 2;
   data->beard_type = 1;
   data->glass_height = 0;
   data->glass_scale = 0;
   data->glass_color = 1;
   data->glass_type = 5;
   data->mole_ypos = 4;
   data->mole_xpos = 1;
   data->mole_scale = 10;
   data->mole_enabled = 0;

   std::memcpy(data->creator_name, MiiCreatorsName.c_str(), MiiCreatorsName.size() * 2 + 2);

   data->checksum = calculateMiiCRC(virt_cast<uint8_t *>(data),
                                    sizeof(FFLStoreData) - 2);
   return nn::ResultSuccess;
}

nn::Result
GetMiiName(virt_ptr<char16_t> name)
{
   return GetMiiNameEx(name, CurrentUserSlot);
}

nn::Result
GetMiiNameEx(virt_ptr<char16_t> name,
             SlotNo slot)
{
   if (slot != SystemSlot &&
       slot != CurrentUserSlot &&
       slot != sUserAccount.slot) {
      return ResultAccountNotFound;
   }

   std::copy(MiiName.begin(), MiiName.end(), name.get());
   name[MiiName.size()] = char16_t { 0 };

   return nn::ResultSuccess;
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
IsNetworkAccountEx(SlotNo slot)
{
   return false;
}

nn::Result
GetDeviceHash(virt_ptr<uint8_t> data)
{
   std::memcpy(data.get(), DeviceHash, 6);
   return nn::ResultSuccess;
}

nn::Result
IsCommittedEx(SlotNo slot)
{
   return nn::ResultSuccess;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3actFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3actFv",
                              Finalize);
   RegisterFunctionExportName("Cancel__Q2_2nn3actFv",
                              Cancel);
   RegisterFunctionExportName("IsSlotOccupied__Q2_2nn3actFUc",
                              IsSlotOccupied);
   RegisterFunctionExportName("GetSlotNo__Q2_2nn3actFv",
                              GetSlotNo);
   RegisterFunctionExportName("GetMii__Q2_2nn3actFP12FFLStoreData",
                              GetMii);
   RegisterFunctionExportName("GetMiiEx__Q2_2nn3actFP12FFLStoreDataUc",
                              GetMiiEx);
   RegisterFunctionExportName("GetMiiName__Q2_2nn3actFPw",
                              GetMiiName);
   RegisterFunctionExportName("GetMiiNameEx__Q2_2nn3actFPwUc",
                              GetMiiNameEx);
   RegisterFunctionExportName("IsParentalControlCheckEnabled__Q2_2nn3actFv",
                              IsParentalControlCheckEnabled);
   RegisterFunctionExportName("IsNetworkAccount__Q2_2nn3actFv",
                              IsNetworkAccount);
   RegisterFunctionExportName("IsNetworkAccountEx__Q2_2nn3actFUc",
                              IsNetworkAccountEx);
   RegisterFunctionExportName("GetNumOfAccounts__Q2_2nn3actFv",
                              GetNumOfAccounts);
   RegisterFunctionExportName("GetUuid__Q2_2nn3actFP7ACTUuid",
                              GetUuid);
   RegisterFunctionExportName("GetUuidEx__Q2_2nn3actFP7ACTUuidUc",
                              GetUuidEx);
   RegisterFunctionExportName("GetAccountId__Q2_2nn3actFPc",
                              GetAccountId);
   RegisterFunctionExportName("GetAccountIdEx__Q2_2nn3actFPcUc",
                              GetAccountIdEx);
   RegisterFunctionExportName("GetParentalControlSlotNo__Q2_2nn3actFv",
                              GetParentalControlSlotNo);
   RegisterFunctionExportName("GetParentalControlSlotNoEx__Q2_2nn3actFPUcUc",
                              GetParentalControlSlotNoEx);
   RegisterFunctionExportName("GetPersistentId__Q2_2nn3actFv",
                              GetPersistentId);
   RegisterFunctionExportName("GetPersistentIdEx__Q2_2nn3actFUc",
                              GetPersistentIdEx);
   RegisterFunctionExportName("GetPrincipalId__Q2_2nn3actFv",
                              GetPrincipalId);
   RegisterFunctionExportName("GetPrincipalIdEx__Q2_2nn3actFPUiUc",
                              GetPrincipalIdEx);
   RegisterFunctionExportName("GetSimpleAddressId__Q2_2nn3actFv",
                              GetSimpleAddressId);
   RegisterFunctionExportName("GetSimpleAddressIdEx__Q2_2nn3actFPUiUc",
                              GetSimpleAddressIdEx);
   RegisterFunctionExportName("GetTransferableId__Q2_2nn3actFUi",
                              GetTransferableId);
   RegisterFunctionExportName("GetTransferableIdEx__Q2_2nn3actFPULUiUc",
                              GetTransferableIdEx);
   RegisterFunctionExportName("GetDeviceHash__Q2_2nn3actFPUL",
                              GetDeviceHash);
   RegisterFunctionExportName("IsCommittedEx__Q2_2nn3actFUc",
                              IsCommittedEx);
}

} // namespace cafe::nn_act
