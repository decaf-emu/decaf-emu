#pragma once
#include <array>
#include "common/types.h"
#include "modules/nn_ffl.h"
#include "modules/nn_result.h"

namespace nn
{

namespace act
{

using UUID = std::array<uint8_t, 16>;

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
GetMii(FFLStoreData *data);

nn::Result
GetMiiEx(FFLStoreData *data,
         uint8_t slot);

nn::Result
GetMiiName(be_val<char16_t> *name);

nn::Result
GetMiiNameEx(be_val<char16_t> *name,
             uint8_t slot);

bool
IsNetworkAccount();

bool
IsNetworkAccountEx(uint8_t slot);

nn::Result
GetAccountId(char *accountId);

nn::Result
GetAccountIdEx(char *accountId,
               uint8_t slot);

nn::Result
GetUuid(UUID *uuid);

nn::Result
GetUuidEx(UUID *uuid,
          uint8_t slotNo);

uint8_t
GetParentalControlSlotNo();

nn::Result
GetParentalControlSlotNoEx(uint8_t *parentSlot, uint8_t slot);

uint32_t
GetPersistentId();

uint32_t
GetPersistentIdEx(uint8_t slot);

uint32_t
GetPrincipalId();

nn::Result
GetPrincipalIdEx(be_val<uint32_t> *principalId,
                 uint8_t slotNo);

uint32_t
GetSimpleAddressId();

nn::Result
GetSimpleAddressIdEx(be_val<uint32_t> *simpleAddressId,
                     uint8_t slotNo);
uint64_t
GetTransferableId(uint32_t unk1);

nn::Result
GetTransferableIdEx(be_val<uint64_t> *transferableId,
                    uint32_t unk1,
                    uint8_t slot);

bool
IsParentalControlCheckEnabled();

}  // namespace act

}  // namespace nn
