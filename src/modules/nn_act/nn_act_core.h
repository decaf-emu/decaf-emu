#pragma once
#include <array>
#include "types.h"
#include "modules/nn_result.h"

namespace nn
{

namespace act
{

using UUID = std::array<uint8_t, 16>;

nn::Result
Initialize();

void
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
GetMii(void *data);

nn::Result
GetMiiEx(void *data, uint8_t slot);

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

}  // namespace act

}  // namespace nn
