#pragma once
#include <array>
#include <cstdint>

namespace nn::act
{

using SlotNo = uint8_t;
using LocalFriendCode = uint64_t;
using PersistentId = uint32_t;
using PrincipalId = uint32_t;
using SimpleAddressId = uint32_t;
using TransferrableId = uint64_t;
using Uuid = std::array<char, 0x10>;

static constexpr SlotNo InvalidSlot = 0;
static constexpr SlotNo NumSlots = 12;
static constexpr SlotNo CurrentUserSlot = 254;
static constexpr SlotNo SystemSlot = 255;

static constexpr uint32_t AccountIdSize = 17;
static constexpr uint32_t NfsPasswordSize = 17;
static constexpr uint32_t MiiNameSize = 11;
static constexpr uint32_t UuidSize = 16;

} // namespace nn::act
