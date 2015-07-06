#pragma once
#include "systemtypes.h"

namespace nn
{

namespace act
{

enum class Result : uint32_t
{
   OK = 0,
   Error = 0x80000000,
};

Result
Initialize();

void
Finalize();

Result
Cancel();

uint8_t
GetSlotNo();

uint32_t
GetTransferableId(uint32_t unk1);

}  // namespace act

}  // namespace nn
