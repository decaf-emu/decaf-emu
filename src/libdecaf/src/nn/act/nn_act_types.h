#pragma once
#include <array>
#include <cstdint>

namespace nn::act
{

using SlotNo = uint8_t;

static constexpr SlotNo InvalidSlot = 0;
static constexpr SlotNo MaxSlot = 12;
static constexpr SlotNo CurrentUserSlot = 254;
static constexpr SlotNo SystemSlot = 255;

} // namespace nn::act
