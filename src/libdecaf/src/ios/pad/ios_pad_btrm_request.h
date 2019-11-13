#pragma once
#include "ios_pad_enum.h"

#include <libcpu/be2_struct.h>

namespace ios::pad
{

/**
 * \ingroup ios_pad
 * @{
 */

#pragma pack(push, 1)

struct BtrmRequest
{
   union {
      be2_array<uint8_t, 0x1000> data;
   };

   be2_val<BtrmCommand> command;
   be2_val<BtrmSubCommand> subcommand;
   be2_val<uint16_t> unk0x1002;
   be2_val<uint32_t> unk0x1004;
};
CHECK_OFFSET(BtrmRequest, 0x1000, command);
CHECK_OFFSET(BtrmRequest, 0x1001, subcommand);
CHECK_OFFSET(BtrmRequest, 0x1002, unk0x1002);
CHECK_OFFSET(BtrmRequest, 0x1004, unk0x1004);
CHECK_SIZE(BtrmRequest, 0x1008);

#pragma pack(pop)

/** @} */

} // namespace ios::pad
