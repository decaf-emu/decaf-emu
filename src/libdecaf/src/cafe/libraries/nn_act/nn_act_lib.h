#pragma once
#include "cafe/libraries/nn_ffl.h"
#include "cafe/libraries/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::act
{

using UUID = be2_array<char, 16>;

static const uint8_t
InvalidSlot = 0;

static const uint8_t
CurrentUserSlot = 254;

static const uint8_t
SystemSlot = 255;

nn::Result
Initialize();

nn::Result
Finalize();

uint8_t
GetNumOfAccounts();

bool
IsSlotOccupied(uint8_t slot);

nn::Result
Cancel();

uint8_t
GetSlotNo();

nn::Result
GetMii(virt_ptr<FFLStoreData> data);

nn::Result
GetMiiEx(virt_ptr<FFLStoreData> data,
         uint8_t slot);

nn::Result
GetMiiName(virt_ptr<char16_t> name);

nn::Result
GetMiiNameEx(virt_ptr<char16_t> name,
             uint8_t slot);

bool
IsNetworkAccount();

bool
IsNetworkAccountEx(uint8_t slot);

nn::Result
GetAccountId(virt_ptr<char> accountId);

nn::Result
GetAccountIdEx(virt_ptr<char> accountId,
               uint8_t slot);

nn::Result
GetUuid(virt_ptr<UUID> uuid);

nn::Result
GetUuidEx(virt_ptr<UUID> uuid,
          uint8_t slotNo);

uint8_t
GetParentalControlSlotNo();

nn::Result
GetParentalControlSlotNoEx(virt_ptr<uint8_t> parentSlot,
                           uint8_t slot);

uint32_t
GetPersistentId();

uint32_t
GetPersistentIdEx(uint8_t slot);

uint32_t
GetPrincipalId();

nn::Result
GetPrincipalIdEx(virt_ptr<uint32_t> principalId,
                 uint8_t slotNo);

uint32_t
GetSimpleAddressId();

nn::Result
GetSimpleAddressIdEx(virt_ptr<uint32_t> simpleAddressId,
                     uint8_t slotNo);
uint64_t
GetTransferableId(uint32_t unk1);

nn::Result
GetTransferableIdEx(virt_ptr<uint64_t> transferableId,
                    uint32_t unk1,
                    uint8_t slot);

bool
IsParentalControlCheckEnabled();

}  // namespace cafe::nn::act
