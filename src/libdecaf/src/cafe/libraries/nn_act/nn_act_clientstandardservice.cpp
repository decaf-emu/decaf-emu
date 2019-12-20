#include "nn_act.h"
#include "nn_act_client.h"
#include "nn_act_clientstandardservice.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/act/nn_act_result.h"
#include "nn/act/nn_act_clientstandardservice.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <chrono>

using namespace nn::act;
using namespace nn::ipc;

namespace cafe::nn_act
{

nn::Result
GetAccountId(virt_ptr<char> accountId)
{
   if (!accountId) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(CurrentUserSlot, accountId, AccountIdSize,
                                   InfoType::AccountId);
}

nn::Result
GetAccountIdEx(virt_ptr<char> accountId,
               SlotNo slotNo)
{
   if (!accountId) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slotNo, accountId, AccountIdSize,
                                   InfoType::AccountId);
}

nn::Result
GetBirthday(virt_ptr<uint16_t> year,
            virt_ptr<uint8_t> month,
            virt_ptr<uint8_t> day)
{
   return GetBirthdayEx(year, month, day, CurrentUserSlot);
}

nn::Result
GetBirthdayEx(virt_ptr<uint16_t> year,
              virt_ptr<uint8_t> month,
              virt_ptr<uint8_t> day,
              SlotNo slotNo)
{
   StackObject<Birthday> birthday;
   if (!year || !month || !day) {
      return ResultInvalidPointer;
   }

   auto result = internal::GetAccountInfo(CurrentUserSlot, birthday,
                                          sizeof(Birthday), InfoType::Birthday);
   if (result) {
      *year = birthday->year;
      *month = birthday->month;
      *day = birthday->day;
   }

   return result;
}

SlotNo
GetDefaultAccount()
{
   StackObject<SlotNo> slotNo;
   if (!internal::GetAccountInfo(CurrentUserSlot, slotNo, sizeof(SlotNo),
                                 InfoType::DefaultAccount)) {
      return 0;
   }

   return *slotNo;
}

nn::Result
GetDeviceHash(virt_ptr<uint64_t> hash)
{
   if (!hash) {
      return ResultInvalidPointer;
   }

   return internal::GetCommonInfo(hash, sizeof(uint64_t), InfoType::DeviceHash);
}

nn::Result
GetMii(virt_ptr<nn::ffl::FFLStoreData> mii)
{
   if (!mii) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(CurrentUserSlot, mii,
                                   sizeof(nn::ffl::FFLStoreData),
                                   InfoType::Mii);
}

nn::Result
GetMiiEx(virt_ptr<nn::ffl::FFLStoreData> mii,
         SlotNo slotNo)
{
   if (!mii) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slotNo, mii,
                                   sizeof(nn::ffl::FFLStoreData),
                                   InfoType::Mii);
}

nn::Result
GetMiiImageEx(virt_ptr<uint32_t> outImageSize,
              virt_ptr<void> buffer,
              uint32_t bufferSize,
              MiiImageType miiImageType,
              SlotNo slot)
{
   auto command = ClientCommand<services::ClientStandardService::GetMiiImage>{ internal::getAllocator() };
   command.setParameters(slot, { buffer, bufferSize }, miiImageType);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result) {
      auto imageSize = uint32_t { 0 };
      result = command.readResponse(imageSize);
      if (result) {
         *outImageSize = imageSize;
      }
   }

   return result;
}

nn::Result
GetMiiName(virt_ptr<char16_t> name)
{
   return GetMiiNameEx(name, CurrentUserSlot);
}

nn::Result
GetMiiNameEx(virt_ptr<char16_t> name,
             SlotNo slotNo)
{
   if (!name) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slotNo, name, MiiNameSize * sizeof(char16_t),
                                   InfoType::MiiName);
}

nn::Result
GetNfsPassword(virt_ptr<char> password)
{
   return GetNfsPasswordEx(password, CurrentUserSlot);
}

nn::Result
GetNfsPasswordEx(virt_ptr<char> password,
                 SlotNo slotNo)
{
   if (!password) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slotNo, password, 17, InfoType::NfsPassword);
}

uint8_t
GetNumOfAccounts()
{
   StackObject<uint8_t> num;
   if (!internal::GetCommonInfo(num, sizeof(uint8_t), InfoType::NumOfAccounts)) {
      return 0;
   }

   return *num;
}

SlotNo
GetParentalControlSlotNo()
{
   StackObject<SlotNo> slotNo;
   if (!internal::GetAccountInfo(CurrentUserSlot, slotNo, sizeof(SlotNo),
                                 InfoType::ParentalControlSlot)) {
      return 0;
   }

   return *slotNo;
}

nn::Result
GetParentalControlSlotNoEx(virt_ptr<SlotNo> parentalSlotNo,
                           SlotNo slotNo)
{
   if (!parentalSlotNo) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slotNo, parentalSlotNo, sizeof(SlotNo),
                                   InfoType::ParentalControlSlot);
}

PersistentId
GetPersistentId()
{
   StackObject<PersistentId> id;
   if (!internal::GetAccountInfo(CurrentUserSlot, id, sizeof(PersistentId),
                                 InfoType::PersistentId)) {
      return 0;
   }

   return *id;
}

PersistentId
GetPersistentIdEx(SlotNo slotNo)
{
   StackObject<PersistentId> id;
   if (!internal::GetAccountInfo(slotNo, id, sizeof(PersistentId),
                                 InfoType::PersistentId)) {
      return 0;
   }

   return *id;
}

PrincipalId
GetPrincipalId()
{
   StackObject<PrincipalId> id;
   if (!internal::GetAccountInfo(CurrentUserSlot, id, sizeof(PrincipalId),
                                 InfoType::PrincipalId)) {
      return 0;
   }

   return *id;
}

nn::Result
GetPrincipalIdEx(virt_ptr<PrincipalId> id,
                 SlotNo slot)
{
   if (!id) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slot, id, sizeof(PrincipalId),
                                   InfoType::PrincipalId);
}

SimpleAddressId
GetSimpleAddressId()
{
   StackObject<SimpleAddressId> id;
   if (!internal::GetAccountInfo(CurrentUserSlot, id, sizeof(SimpleAddressId),
                                 InfoType::SimpleAddressId)) {
      return 0;
   }

   return *id;
}

nn::Result
GetSimpleAddressIdEx(virt_ptr<SimpleAddressId> id,
                     SlotNo slot)
{
   if (!id) {
      return ResultInvalidPointer;
   }

   return internal::GetAccountInfo(slot, id, sizeof(SimpleAddressId),
                                   InfoType::SimpleAddressId);
}

SlotNo
GetSlotNo()
{
   StackObject<SlotNo> slotNo;
   if (!internal::GetCommonInfo(slotNo, sizeof(SlotNo), InfoType::SlotNo)) {
      return 0;
   }

   return *slotNo;
}

uint64_t
GetTransferableId(uint32_t unk1)
{
   StackObject<uint64_t> id;
   GetTransferableIdEx(id, unk1, CurrentUserSlot);
   return *id;
}

nn::Result
GetTransferableIdEx(virt_ptr<TransferrableId> id,
                    uint32_t unk1,
                    SlotNo slotNo)
{
   if (!id) {
      return ResultInvalidPointer;
   }

   auto command = ClientCommand<services::ClientStandardService::GetTransferableId> { internal::getAllocator() };
   command.setParameters(slotNo, unk1);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      auto value = TransferrableId { };
      result = command.readResponse(value);
      if (result.ok()) {
         *id = value;
      }
   }

   return result;
}

nn::Result
GetUuidEx(virt_ptr<Uuid> uuid,
          SlotNo slotNo,
          int32_t arg3)
{
   auto command = ClientCommand<services::ClientStandardService::GetUuid> { internal::getAllocator() };
   command.setParameters(slotNo, uuid, arg3);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return result;
}

nn::Result
GetUuidEx(virt_ptr<Uuid> uuid,
          SlotNo slot)
{
   return GetUuidEx(uuid, slot, -2);
}

nn::Result
GetUuid(virt_ptr<Uuid> uuid,
        int32_t arg3)
{
   return GetUuidEx(uuid, CurrentUserSlot, arg3);
}

nn::Result
GetUuid(virt_ptr<Uuid> uuid)
{
   return GetUuidEx(uuid, CurrentUserSlot, -2);
}

bool
HasNfsAccount()
{
   StackArray<char, 17> nfsPassword;
   if (!GetNfsPassword(nfsPassword)) {
      return false;
   }

   return nfsPassword[0] != '\0';
}

bool
IsCommitted()
{
   return IsCommittedEx(CurrentUserSlot);
}

bool
IsCommittedEx(SlotNo slotNo)
{
   StackObject<uint8_t> value;
   if (!internal::GetAccountInfo(slotNo, value, sizeof(uint8_t),
                                 InfoType::IsCommitted)) {
      return 0;
   }

   return *value != 0;
}

bool
IsPasswordCacheEnabled()
{
   return IsPasswordCacheEnabledEx(GetSlotNo());
}

bool
IsPasswordCacheEnabledEx(SlotNo slotNo)
{
   StackObject<uint8_t> value;
   if (!internal::GetAccountInfo(slotNo, value, sizeof(uint8_t),
                                 InfoType::IsPasswordCacheEnabled)) {
      return 0;
   }

   return *value != 0;
}

bool
IsNetworkAccount()
{
   StackArray<char, AccountIdSize> accountId;
   if (!GetAccountId(accountId)) {
      return false;
   }

   return accountId[0] != 0;
}

bool
IsNetworkAccountEx(SlotNo slotNo)
{
   StackArray<char, AccountIdSize> accountId;
   if (!GetAccountIdEx(accountId, slotNo)) {
      return false;
   }

   return accountId[0] != 0;
}

bool
IsServerAccountActive()
{
   return IsServerAccountActiveEx(CurrentUserSlot);
}

bool
IsServerAccountActiveEx(SlotNo slotNo)
{
   StackObject<uint32_t> value;
   if (!internal::GetAccountInfo(slotNo, value, sizeof(uint32_t),
                                 InfoType::ServerAccountStatus)) {
      return 0;
   }

   return *value == 0;
}

bool
IsServerAccountDeleted()
{
   return IsServerAccountDeletedEx(CurrentUserSlot);
}

bool
IsServerAccountDeletedEx(SlotNo slotNo)
{
   StackObject<uint8_t> value;
   if (!internal::GetAccountInfo(slotNo, value, sizeof(uint8_t),
                                 InfoType::IsServerAccountDeleted)) {
      return 0;
   }

   return !!*value;
}

bool
IsSlotOccupied(SlotNo slot)
{
   return GetPersistentIdEx(slot) != 0;
}

namespace internal
{

nn::Result
GetAccountInfo(SlotNo slotNo,
               virt_ptr<void> buffer,
               uint32_t bufferSize,
               InfoType type)
{
   auto command = ClientCommand<services::ClientStandardService::GetAccountInfo> { internal::getAllocator() };
   command.setParameters(slotNo, { buffer, bufferSize }, type);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return result;
}

nn::Result
GetCommonInfo(virt_ptr<void> buffer,
              uint32_t bufferSize,
              InfoType type)
{
   auto command = ClientCommand<services::ClientStandardService::GetCommonInfo> { internal::getAllocator() };
   command.setParameters({ buffer, bufferSize }, type);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return result;
}

} // namespace internal

void
Library::registerClientStandardServiceSymbols()
{
   RegisterFunctionExportName("GetAccountId__Q2_2nn3actFPc",
                              GetAccountId);
   RegisterFunctionExportName("GetAccountIdEx__Q2_2nn3actFPcUc",
                              GetAccountIdEx);
   RegisterFunctionExportName("GetBirthday__Q2_2nn3actFPUsPUcT2",
                              GetBirthday);
   RegisterFunctionExportName("GetBirthdayEx__Q2_2nn3actFPUsPUcT2Uc",
                              GetBirthdayEx);
   RegisterFunctionExportName("GetDefaultAccount__Q2_2nn3actFv",
                              GetDefaultAccount);
   RegisterFunctionExportName("GetDeviceHash__Q2_2nn3actFPUL",
                              GetDeviceHash);
   RegisterFunctionExportName("GetMii__Q2_2nn3actFP12FFLStoreData",
                              GetMii);
   RegisterFunctionExportName("GetMiiEx__Q2_2nn3actFP12FFLStoreDataUc",
                              GetMiiEx);
   RegisterFunctionExportName("GetMiiImageEx__Q2_2nn3actFPUiPvUi15ACTMiiImageTypeUc",
                              GetMiiImageEx);
   RegisterFunctionExportName("GetMiiName__Q2_2nn3actFPw",
                              GetMiiName);
   RegisterFunctionExportName("GetMiiNameEx__Q2_2nn3actFPwUc",
                              GetMiiNameEx);
   RegisterFunctionExportName("GetNfsPassword__Q2_2nn3actFPc",
                              GetNfsPassword);
   RegisterFunctionExportName("GetNfsPasswordEx__Q2_2nn3actFPcUc",
                              GetNfsPasswordEx);
   RegisterFunctionExportName("GetNumOfAccounts__Q2_2nn3actFv",
                              GetNumOfAccounts);
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
   RegisterFunctionExportName("GetSlotNo__Q2_2nn3actFv",
                              GetSlotNo);
   RegisterFunctionExportName("GetTransferableId__Q2_2nn3actFUi",
                              GetTransferableId);
   RegisterFunctionExportName("GetTransferableIdEx__Q2_2nn3actFPULUiUc",
                              GetTransferableIdEx);
   RegisterFunctionExportName("GetUuid__Q2_2nn3actFP7ACTUuid",
                              static_cast<nn::Result(*)(virt_ptr<Uuid>)>(GetUuid));
   RegisterFunctionExportName("GetUuid__Q2_2nn3actFP7ACTUuidUi",
                              static_cast<nn::Result(*)(virt_ptr<Uuid>, int32_t)>(GetUuid));
   RegisterFunctionExportName("GetUuidEx__Q2_2nn3actFP7ACTUuidUc",
                              static_cast<nn::Result(*)(virt_ptr<Uuid>, SlotNo)>(GetUuidEx));
   RegisterFunctionExportName("GetUuidEx__Q2_2nn3actFP7ACTUuidUcUi",
                              static_cast<nn::Result(*)(virt_ptr<Uuid>, SlotNo, int32_t)>(GetUuidEx));
   RegisterFunctionExportName("HasNfsAccount__Q2_2nn3actFv",
                              HasNfsAccount);
   RegisterFunctionExportName("IsCommitted__Q2_2nn3actFv",
                              IsCommitted);
   RegisterFunctionExportName("IsCommittedEx__Q2_2nn3actFUc",
                              IsCommittedEx);
   RegisterFunctionExportName("IsPasswordCacheEnabled__Q2_2nn3actFv",
                              IsPasswordCacheEnabled);
   RegisterFunctionExportName("IsPasswordCacheEnabledEx__Q2_2nn3actFUc",
                              IsPasswordCacheEnabledEx);
   RegisterFunctionExportName("IsNetworkAccount__Q2_2nn3actFv",
                              IsNetworkAccount);
   RegisterFunctionExportName("IsNetworkAccountEx__Q2_2nn3actFUc",
                              IsNetworkAccountEx);
   RegisterFunctionExportName("IsServerAccountActive__Q2_2nn3actFv",
                              IsServerAccountActive);
   RegisterFunctionExportName("IsServerAccountActiveEx__Q2_2nn3actFUc",
                              IsServerAccountActiveEx);
   RegisterFunctionExportName("IsServerAccountDeleted__Q2_2nn3actFv",
                              IsServerAccountDeleted);
   RegisterFunctionExportName("IsServerAccountDeletedEx__Q2_2nn3actFUc",
                              IsServerAccountDeletedEx);
   RegisterFunctionExportName("IsSlotOccupied__Q2_2nn3actFUc",
                              IsSlotOccupied);
}

}  // namespace cafe::nn_act
