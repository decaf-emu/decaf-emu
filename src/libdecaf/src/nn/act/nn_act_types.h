#pragma once
#include <array>
#include <cstdint>
#include <libcpu/be2_struct.h>

namespace nn::act
{

#pragma pack(push, 1)

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

struct Birthday
{
   be2_val<uint16_t> year;
   be2_val<uint8_t> month;
   be2_val<uint8_t> day;
};
CHECK_OFFSET(Birthday, 0x00, year);
CHECK_OFFSET(Birthday, 0x02, month);
CHECK_OFFSET(Birthday, 0x03, day);
CHECK_SIZE(Birthday, 0x4);

struct NexAuthenticationResult
{
   be2_array<char, 513> token;
   PADDING(3);
   be2_array<char, 65> password;
   PADDING(3);
   be2_array<char, 16> host;
   be2_val<uint16_t> port;
   PADDING(2);
};
CHECK_OFFSET(NexAuthenticationResult, 0x000, token);
CHECK_OFFSET(NexAuthenticationResult, 0x204, password);
CHECK_OFFSET(NexAuthenticationResult, 0x248, host);
CHECK_OFFSET(NexAuthenticationResult, 0x258, port);
CHECK_SIZE(NexAuthenticationResult, 0x25C);

#pragma pack(pop)

} // namespace nn::act
