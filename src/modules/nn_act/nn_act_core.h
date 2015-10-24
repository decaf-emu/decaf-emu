#pragma once
#include "types.h"
#include "modules/nn_result.h"

namespace nn
{

namespace act
{

nn::Result
Initialize();

void
Finalize();

bool
IsSlotOccupied(uint8_t id);

nn::Result
Cancel();

uint8_t
GetSlotNo();

uint32_t
GetTransferableId(uint32_t unk1);

}  // namespace act

}  // namespace nn
