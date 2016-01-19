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

uint32_t
GetTransferableId(uint32_t unk1);

nn::Result
GetMii(void *data);

nn::Result
GetMiiEx(void *data, uint8_t slot);

bool
IsNetworkAccount();

bool
IsNetworkAccountEx(uint8_t slot);

nn::Result
GetUuidEx(UUID *uuid,
          uint8_t slotNo);

nn::Result
GetPrincipalIdEx(be_val<uint32_t> *principalId,
                 uint8_t slotNo);

nn::Result
GetSimpleAddressIdEx(be_val<uint32_t> *simpleAddressId,
                     uint8_t slotNo);

}  // namespace act

}  // namespace nn
