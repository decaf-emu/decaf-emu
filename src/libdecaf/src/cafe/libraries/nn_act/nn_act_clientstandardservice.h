#pragma once
#include "nn/nn_result.h"
#include "nn/act/nn_act_enum.h"
#include "nn/act/nn_act_types.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <cstdint>

namespace cafe::nn_act
{

using nn::act::MiiImageType;
using nn::act::PersistentId;
using nn::act::PrincipalId;
using nn::act::SlotNo;
using nn::act::SimpleAddressId;
using nn::act::TransferrableId;
using nn::act::Uuid;

nn::Result
GetAccountId(virt_ptr<char> accountId);

nn::Result
GetAccountIdEx(virt_ptr<char> accountId,
               SlotNo slotNo);

nn::Result
GetBirthday(virt_ptr<uint16_t> year,
            virt_ptr<uint8_t> month,
            virt_ptr<uint8_t> day);

nn::Result
GetBirthdayEx(virt_ptr<uint16_t> year,
              virt_ptr<uint8_t> month,
              virt_ptr<uint8_t> day,
              SlotNo slotNo);

SlotNo
GetDefaultAccount();

nn::Result
GetDeviceHash(virt_ptr<uint64_t> hash);

nn::Result
GetMii(virt_ptr<nn::ffl::FFLStoreData> mii);

nn::Result
GetMiiEx(virt_ptr<nn::ffl::FFLStoreData> mii,
         SlotNo slotNo);

nn::Result
GetMiiImageEx(virt_ptr<uint32_t> outImageSize,
              virt_ptr<void> buffer,
              uint32_t bufferSize,
              MiiImageType miiImageType,
              SlotNo slot);

nn::Result
GetMiiName(virt_ptr<char16_t> name);

nn::Result
GetMiiNameEx(virt_ptr<char16_t> name,
             SlotNo slotNo);

nn::Result
GetNfsPassword(virt_ptr<char> password);

nn::Result
GetNfsPasswordEx(virt_ptr<char> password,
                 SlotNo slotNo);

uint8_t
GetNumOfAccounts();

SlotNo
GetParentalControlSlotNo();

nn::Result
GetParentalControlSlotNoEx(virt_ptr<SlotNo> parentalSlotNo,
                           SlotNo slotNo);

PersistentId
GetPersistentId();

PersistentId
GetPersistentIdEx(SlotNo slotNo);

PrincipalId
GetPrincipalId();

nn::Result
GetPrincipalIdEx(virt_ptr<PrincipalId> id,
                 SlotNo slot);

SimpleAddressId
GetSimpleAddressId();

nn::Result
GetSimpleAddressIdEx(virt_ptr<SimpleAddressId> id,
                     SlotNo slot);

SlotNo
GetSlotNo();

uint64_t
GetTransferableId(uint32_t unk1);

nn::Result
GetTransferableIdEx(virt_ptr<TransferrableId> id,
                    uint32_t unk1,
                    SlotNo slotNo);

nn::Result
GetUuidEx(virt_ptr<Uuid> uuid,
          SlotNo slotNo,
          int32_t arg3);

nn::Result
GetUuidEx(virt_ptr<Uuid> uuid,
          SlotNo slot);

nn::Result
GetUuid(virt_ptr<Uuid> uuid,
        int32_t arg3);

nn::Result
GetUuid(virt_ptr<Uuid> uuid);

bool
HasNfsAccount();

bool
IsCommitted();

bool
IsCommittedEx(SlotNo slotNo);

bool
IsPasswordCacheEnabled();

bool
IsPasswordCacheEnabledEx(SlotNo slotNo);

bool
IsNetworkAccount();

bool
IsNetworkAccountEx(SlotNo slotNo);

bool
IsServerAccountActive();

bool
IsServerAccountActiveEx(SlotNo slotNo);

bool
IsServerAccountDeleted();

bool
IsServerAccountDeletedEx(SlotNo slotNo);

bool
IsSlotOccupied(SlotNo slot);

namespace internal
{

nn::Result
GetAccountInfo(nn::act::SlotNo slotNo,
               virt_ptr<void> buffer,
               uint32_t bufferSize,
               nn::act::InfoType type);

nn::Result
GetCommonInfo(virt_ptr<void> buffer,
              uint32_t bufferSize,
              nn::act::InfoType type);

} // namespace internal

}  // namespace cafe::nn_act
