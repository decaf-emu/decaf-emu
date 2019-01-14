#pragma once
#include "nn/nn_result.h"
#include "nn/act/nn_act_types.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_act
{

using UUID = be2_array<char, 16>;
using SlotNo = nn::act::SlotNo;

nn::Result
Initialize();

nn::Result
Finalize();

uint8_t
GetNumOfAccounts();

bool
IsSlotOccupied(SlotNo slot);

nn::Result
Cancel();

uint8_t
GetSlotNo();

nn::Result
GetMii(virt_ptr<nn::ffl::FFLStoreData> data);

nn::Result
GetMiiEx(virt_ptr<nn::ffl::FFLStoreData> data,
         SlotNo slot);

nn::Result
GetMiiName(virt_ptr<char16_t> name);

nn::Result
GetMiiNameEx(virt_ptr<char16_t> name,
             SlotNo slot);

bool
IsNetworkAccount();

bool
IsNetworkAccountEx(SlotNo slot);

nn::Result
GetAccountId(virt_ptr<char> accountId);

nn::Result
GetAccountIdEx(virt_ptr<char> accountId,
               SlotNo slot);

nn::Result
GetUuid(virt_ptr<UUID> uuid);

nn::Result
GetUuidEx(virt_ptr<UUID> uuid,
          SlotNo slotNo);

SlotNo
GetParentalControlSlotNo();

nn::Result
GetParentalControlSlotNoEx(virt_ptr<SlotNo> parentSlot,
                           SlotNo slot);

uint32_t
GetPersistentId();

uint32_t
GetPersistentIdEx(SlotNo slot);

uint32_t
GetPrincipalId();

nn::Result
GetPrincipalIdEx(virt_ptr<uint32_t> principalId,
                 SlotNo slotNo);

uint32_t
GetSimpleAddressId();

nn::Result
GetSimpleAddressIdEx(virt_ptr<uint32_t> simpleAddressId,
                     SlotNo slotNo);
uint64_t
GetTransferableId(uint32_t unk1);

nn::Result
GetTransferableIdEx(virt_ptr<uint64_t> transferableId,
                    uint32_t unk1,
                    SlotNo slot);

bool
IsParentalControlCheckEnabled();

}  // namespace cafe::nn_act
